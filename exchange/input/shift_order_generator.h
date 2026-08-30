#pragma once

#include "base/logging/loggable_class.h"
#include "base/random.h"
#include "base/traits.h"

#include "exchange/input/maintain_book_skill.h"
#include "exchange/input/store_orders_skill.h"

#include <boost/container/static_vector.hpp>
#include <glaze/glaze.hpp>

namespace exchange_sim {

template<template<typename> typename BookContainerType, typename GeneratorInterface>
  requires(derived_from_all<GeneratorInterface,
              generator_interface_skills::StoreOrders,
              generator_interface_skills::MaintainBook<BookContainerType>>)
class ShiftOrderGenerator : public LoggableClass, NonCopyable, NonMoveable {
public:
  explicit ShiftOrderGenerator(GeneratorInterface& generator_interface, const glz::generic& config):
      LoggableClass("ShiftOrderGenerator"),
      interface_(generator_interface),
      max_pending_transactions_(config["max_pending_transactions"].as<std::uint32_t>()),
      random_(config["seed"].as<std::uint32_t>()) {
    uint32_t min_active_orders_by_security_ = config["min_active_orders_by_security"].as<std::uint32_t>();
    uint32_t max_active_orders_by_security_ = config["max_active_orders_by_security"].as<std::uint32_t>();
    uint32_t max_quiet_state_sent_orders_count_ = config["max_quiet_state_sent_orders_count"].as<std::uint32_t>();
    uint32_t max_beat_filled_orders_count_ = config["max_beat_filled_orders_count"].as<std::uint32_t>();
    CHECK(min_active_orders_by_security_ <= max_active_orders_by_security_,
        "Min active orders must be less than max, min_active_orders_by_security={} max_active_orders_by_security={}",
        min_active_orders_by_security_,
        max_active_orders_by_security_);
    CHECK(min_active_orders_by_security_ > 0,
        "Min active orders must be greater than 0, min_active_orders_by_security={}",
        min_active_orders_by_security_);
    generators_.reserve(interface_.security_storage().size());
    for (const Security& security : interface_.security_storage().securities()) {
      generators_.emplace_back(interface_,
          security,
          min_active_orders_by_security_,
          max_active_orders_by_security_,
          max_quiet_state_sent_orders_count_,
          max_beat_filled_orders_count_,
          random_);
    }
  }

  void generate_event() {
    if (interface_.pending_transactions() >= max_pending_transactions_) {
      return;
    }
    SecurityID security_id = random_.rand_int_strong_type<SecurityID>(0, interface_.security_storage().size() - 1);
    generators_[security_id].generate_event();
  }

  void process_market_event(const auto&) {}
  void process_report(const auto&) {}

private:
  class PerSecurityGenerator : public LoggableClass {
  public:
    explicit PerSecurityGenerator(GeneratorInterface& interface,
        const Security& security,
        const uint32_t min_active_orders_by_security,
        const uint32_t max_active_orders_by_security,
        const uint32_t max_quiet_state_sent_orders_count,
        const uint32_t max_beat_filled_orders_count,
        Random& random):
        LoggableClass("PerSecurityGenerator_" + std::string(security.symbol.value())),
        interface_(interface),
        security_(security),
        security_id_(security.security_id),
        order_storage_(interface_.order_storage()[security_id_]),
        book_storage_(interface_.book_storage()[security_id_]),
        min_active_orders_by_security_(min_active_orders_by_security),
        max_active_orders_by_security_(max_active_orders_by_security),
        max_quiet_state_sent_orders_count_(max_quiet_state_sent_orders_count),
        max_beat_filled_orders_count_(max_beat_filled_orders_count),
        max_book_order_amount_([](const Security& security, uint32_t max_beat_filled_orders_count) {
          Amount current(ceil(security.max_amount.value() / (max_beat_filled_orders_count + 1)));
          return current - (current % security.lot_size);
        }(security_, max_beat_filled_orders_count_)),
        random_(random),
        middle_price_(security_.low_price) {
      CHECK(max_book_order_amount_ > security_.lot_size,
          "max_book_order_amount must be greater than lot_size,"
          " max_book_order_amount_={}"
          " lot_size={}",
          static_cast<double>(max_book_order_amount_.value()),
          static_cast<double>(security_.lot_size.value()));

      switch_to_shift_state();
    }

    void generate_event() {
      switch (state_) {
        case State::QUIET: {
          generate_event_quiet_state();
          break;
        }
        case State::SHIFT: {
          generate_event_shift_state();
          break;
        }
        case State::REPLENISH: {
          generate_event_replenish_state();
          break;
        }
        default: {
          CRITICAL("Unknown state occurred, state={}", static_cast<uint8_t>(state_));
        }
      }
    }

  private:
    enum class State : uint8_t { SHIFT, QUIET, REPLENISH };
    enum class EventType : uint8_t { CANCEL, ADD, REPLACE, BEAT, SMALL_BEAT };

    // EventType should be instantiated for magic_enum to work
    static_assert(std::to_underlying(EventType::CANCEL) == 0);
    using EventTypeStorage = boost::container::static_vector<EventType, magic_enum::enum_count<EventType>()>;

    void generate_event_quiet_state() {
      EventTypeStorage order_types;
      size_t active_orders_count = order_storage_.active_orders().size();
      if (active_orders_count > min_active_orders_by_security_) {
        order_types.push_back(EventType::CANCEL);
        order_types.push_back(EventType::SMALL_BEAT);
      }
      if (active_orders_count < max_active_orders_by_security_) {
        order_types.push_back(EventType::ADD);
      }
      if (active_orders_count > 0) {
        order_types.push_back(EventType::REPLACE);
      }
      CHECK(order_types.size() > 0, "It's impossible to generate any order type");
      EventType order_type = order_types[random_.rand_int<size_t>(0, order_types.size() - 1)];
      generate_event_by_order_type(order_type);

      ++current_state_sent_orders_count_;
      if (current_state_sent_orders_count_ >= max_quiet_state_sent_orders_count_) {
        switch_to_shift_state();
      }
    }

    void generate_event_shift_state() {
      if (order_storage_.active_orders().empty() ||
          (book_storage_.container().template half<Direction::BUY>().best_price() <= next_middle_price_ &&
              book_storage_.container().template half<Direction::SELL>().best_price() > next_middle_price_)) {
        switch_to_replenish_state();
        generate_event_replenish_state();
        return;
      }

      CHECK(order_storage_.active_orders().size() > 0, "Invalid state, no active orders during shift state");

      EventTypeStorage order_types{EventType::CANCEL, EventType::BEAT};

      EventType order_type = order_types[random_.rand_int<size_t>(0, order_types.size() - 1)];
      generate_event_by_order_type(order_type);

      ++current_state_sent_orders_count_;
    }

    void generate_event_replenish_state() {
      if (order_storage_.active_orders().size() >= active_orders_to_replenish_) {
        switch_to_quiet_state();
        generate_event_quiet_state();
        return;
      }
      generate_event_by_order_type(EventType::ADD);
    }

    void switch_to_shift_state() {
      state_ = State::SHIFT;

      enum class ShiftDirection : std::uint8_t { UP, DOWN };

      boost::container::static_vector<ShiftDirection, 2> shift_direction;
      if (security_.low_price < middle_price_) {
        shift_direction.push_back(ShiftDirection::DOWN);
      }
      if (middle_price_ < security_.high_price - security_.tick_size) {
        shift_direction.push_back(ShiftDirection::UP);
      }
      CHECK(!shift_direction.empty(), "Can't shift to either direction");

      ShiftDirection direction = shift_direction[random_.rand_int<size_t>(0, shift_direction.size() - 1)];
      if (direction == ShiftDirection::DOWN) {
        next_middle_price_ =
            random_.rand_decimal<Price>(security_.low_price, middle_price_ - security_.tick_size, security_.tick_size);
      } else {
        next_middle_price_ = random_.rand_decimal<Price>(
            middle_price_ + security_.tick_size, security_.high_price - security_.tick_size, security_.tick_size);
      }

      current_state_sent_orders_count_ = 0;
    }

    void switch_to_replenish_state() {
      state_ = State::REPLENISH;
      middle_price_ = next_middle_price_;
      next_middle_price_ = Price(0);
      current_state_sent_orders_count_ = 0;
      active_orders_to_replenish_ =
          random_.rand_int<size_t>(min_active_orders_by_security_, max_active_orders_by_security_);
    }

    void switch_to_quiet_state() {
      state_ = State::QUIET;
      current_state_sent_orders_count_ = 0;
      active_orders_to_replenish_ = 0;
    }

    void generate_event_by_order_type(EventType order_type) {
      switch (order_type) {
        case EventType::ADD: {
          generate_add_request();
          break;
        }
        case EventType::CANCEL: {
          generate_cancel_request();
          break;
        }
        case EventType::REPLACE: {
          generate_replace_request();
          break;
        }
        case EventType::BEAT: {
          generate_beat_request(max_beat_filled_orders_count_);
          break;
        }
        case EventType::SMALL_BEAT: {
          generate_beat_request(1);
          break;
        }
        default: {
          CRITICAL("Unknown order type, type={}", static_cast<uint8_t>(order_type));
        }
      }
    }

    std::tuple<Direction, Price> generate_new_order_price_direction() {
      Price left_side = middle_price_ - security_.low_price + security_.tick_size;
      Price right_side = security_.high_price - middle_price_;
      Price diff = random_.rand_decimal<Price>(Price{0}, left_side + right_side, security_.tick_size);
      if (diff < left_side) {
        return {Direction::BUY,
            middle_price_ - std::min(diff, random_.rand_decimal<Price>(Price{0}, left_side, security_.tick_size))};
      } else {
        return {Direction::SELL,
            middle_price_ +
                std::min(diff - left_side, random_.rand_decimal<Price>(Price{0}, right_side, security_.tick_size))};
      }
    }

    void generate_add_request() {
      auto [direction, price] = generate_new_order_price_direction();
      Amount amount = random_.rand_decimal<Amount>(security_.lot_size, max_book_order_amount_, security_.lot_size);
      interface_.send_order_add_request(
          ClientID{0}, security_id_, OrderTimeInForce::LIMIT, OrderType::LIMIT, direction, price, amount);
    }

    void generate_cancel_request() {
      CHECK(!order_storage_.active_orders().empty(),
          "Active orders must not be empty for cancellation query, security id: {}",
          security_id_.value());
      const auto& active_orders = order_storage_.active_orders();
      auto order_to_cancel_index = random_.rand_int<size_t>(0, active_orders.size() - 1);

      interface_.send_order_cancel_request(active_orders[order_to_cancel_index].client_id,
          active_orders[order_to_cancel_index].client_order_id,
          security_id_);
    }

    void generate_replace_request() {
      CHECK(!order_storage_.active_orders().empty(),
          "Active orders must not be empty for replacement query, security_id={}",
          security_id_.value());
      const auto& active_orders = order_storage_.active_orders();
      auto order_to_replace_index = random_.rand_int<size_t>(0, active_orders.size() - 1);

      auto [direction, price] = generate_new_order_price_direction();
      Amount amount = random_.rand_decimal<Amount>(security_.lot_size, max_book_order_amount_, security_.lot_size);

      interface_.send_order_replace_request(active_orders[order_to_replace_index].client_id,
          active_orders[order_to_replace_index].client_order_id,
          security_id_,
          direction,
          price,
          amount);
    }

    template<Direction direction>
    std::tuple<Price, Amount> generate_beat_price_amount(size_t max_filled_orders_count) {
      size_t filled_orders_count = random_.rand_int<size_t>(0, max_filled_orders_count);
      auto& half_book = book_storage_.container().template half<direction>();

      Amount amount{0};
      std::optional<Price> price;
      auto level_iterator = half_book.begin();
      CHECK(level_iterator != half_book.end(),
          "Empty book occurred while preparing for beat, security={} direction={}",
          security_.symbol.value().c_str(),
          format_as(direction));
      auto order_iterator = level_iterator->second.begin();
      while (filled_orders_count > 0 && level_iterator != half_book.end()) {
        if (order_iterator == level_iterator->second.end()) {
          level_iterator++;
          if (level_iterator != half_book.end()) {
            order_iterator = level_iterator->second.begin();
          }
          continue;
        }
        if (amount + order_iterator->quantity > security_.max_amount) {
          break;
        }
        price = order_iterator->price;
        amount += order_iterator->quantity;
        filled_orders_count--;
        order_iterator++;
      }
      while (level_iterator != half_book.end() && order_iterator == level_iterator->second.end()) {
        level_iterator++;
        if (level_iterator != half_book.end()) {
          order_iterator = level_iterator->second.begin();
        }
      }
      if (level_iterator != half_book.end() && order_iterator != level_iterator->second.end()) {
        price = order_iterator->price;
        if (amount == Amount{0}) {
          amount += random_.rand_decimal(security_.lot_size, order_iterator->quantity, security_.lot_size);
        } else {
          Amount max_amount = security_.max_amount - amount;
          max_amount = std::min(max_amount, order_iterator->quantity - security_.lot_size);
          amount += random_.rand_decimal(Amount{0}, max_amount, security_.lot_size);
        }
      }
      CHECK(price.has_value(), "Nullopt price during generation");
      CHECK(amount > Amount{0}, "Zero amount during generation");
      return {price.value(), amount};
    }

    Direction generate_beat_direction() {
      boost::container::static_vector<Direction, 2> possible_directions;
      if (state_ == State::SHIFT) {
        CHECK(middle_price_ != next_middle_price_, "Middle price is equal to next middle price");
        possible_directions = {middle_price_ < next_middle_price_ ? Direction::BUY : Direction::SELL};
      } else {
        if (!book_storage_.container().template half<Direction::SELL>().empty()) {
          possible_directions.push_back(Direction::BUY);
        }
        if (!book_storage_.container().template half<Direction::BUY>().empty()) {
          possible_directions.push_back(Direction::SELL);
        }
      }
      CHECK(!possible_directions.empty(), "No possible directions to beat");
      return possible_directions[random_.rand_int<size_t>(0, possible_directions.size() - 1)];
    }

    void generate_beat_request(uint32_t max_beat_filled_orders_count) {
      Direction direction = generate_beat_direction();
      auto [price, amount] = direction == Direction::BUY ?
          generate_beat_price_amount<Direction::SELL>(max_beat_filled_orders_count) :
          generate_beat_price_amount<Direction::BUY>(max_beat_filled_orders_count);
      generate_exact_beat_request(direction, price, amount);
    }

    void generate_exact_beat_request(Direction direction, Price price, Amount amount) {
      enum class BeatOrderType : uint8_t { ADD, REPLACE };

      boost::container::static_vector<BeatOrderType, 2> order_types{BeatOrderType::ADD};
      /* todo(gorbunovdv): Rewrite the whole replace generation logic in both Generators
       * Currently OrderStorage does not separate orders by Direction
       * In order to maintain the book, we must select the order to replace with the same direction as the beat
       * direction, otherwise the quantity will disappear from the other side of the book
       */

      // if (interface_.order_storage()[security_id_].active_orders().size() > min_active_orders_by_security_) {
      //   order_types.push_back(BeatOrderType::REPLACE);
      // }

      BeatOrderType type = order_types[random_.rand_int<size_t>(0, order_types.size() - 1)];
      switch (type) {
        case BeatOrderType::ADD: {
          interface_.send_order_add_request(
              ClientID{0}, security_id_, OrderTimeInForce::IOC, OrderType::LIMIT, direction, price, amount);
          break;
        }
        case BeatOrderType::REPLACE: {
          CHECK(!order_storage_.active_orders().empty(),
              "Active orders must not be empty for replacement query, security_id={}",
              security_id_.value());
          const auto& active_orders = order_storage_.active_orders();
          auto order_to_replace_index = random_.rand_int<size_t>(0, active_orders.size() - 1);

          interface_.send_order_replace_request(active_orders[order_to_replace_index].client_id,
              active_orders[order_to_replace_index].client_order_id,
              security_id_,
              direction,
              price,
              amount);
          break;
        }
        default: {
          CRITICAL("Unknown BeatOrderType, type={}", static_cast<uint8_t>(type));
        }
      }
    }

    GeneratorInterface& interface_;

    const Security& security_;
    const SecurityID security_id_;

    using OrderStorage = GeneratorInterface::OrderStorage;
    using BookStorage = GeneratorInterface::BookStorage;

    const OrderStorage& order_storage_;
    const BookStorage& book_storage_;

    const uint32_t min_active_orders_by_security_;
    const uint32_t max_active_orders_by_security_;
    const uint32_t max_quiet_state_sent_orders_count_;
    const uint32_t max_beat_filled_orders_count_;

    const Amount max_book_order_amount_;

    Random& random_;

    State state_ = State::REPLENISH;

    Price middle_price_;
    Price next_middle_price_{0};

    size_t current_state_sent_orders_count_ = 0;
    size_t active_orders_to_replenish_ = 0;
  };

  GeneratorInterface& interface_;

  const uint32_t max_pending_transactions_;

  Random random_;

  StrongTypeIndexedVector<SecurityID, PerSecurityGenerator> generators_;
};

}  // namespace exchange_sim