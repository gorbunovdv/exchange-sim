#pragma once

#include "domain/security_storage.h"

#include "exchange/book_containers/book_container_concept.h"
#include "exchange/domain/market_events.h"

namespace exchange_sim::generator_interface_skills {

template<template<typename> typename BookContainerType>
class BookStorage {
public:
  void handle(const market_events::BookOrderEntry& event) {
    ActiveOrder order{event.order_id, event.price, event.quantity, event.direction};
    if (event.direction == Direction::BUY) {
      book_container_.template push_back<Direction::BUY>(order);
    } else {
      book_container_.template push_back<Direction::SELL>(order);
    }
  }

  void handle(const market_events::BookOrderChange& event) {
    auto* order = book_container_.find(event.order_id);
    CHECK(order != nullptr, "Order has not been found in L3Book, order_id={}", event.order_id.value());
    order->quantity = event.new_quantity;
  }

  void handle(const market_events::BookOrderDeletion& event) {
    CHECK(book_container_.extract_order(event.order_id).has_value(),
        "Order has not been found in L3Book, order_id={}",
        event.order_id.value());
  }

  const auto& container() const { return book_container_; }

private:
  struct ActiveOrder {
    ExchangeOrderID order_id;
    Price price;
    Amount quantity;
    Direction direction;

    auto id() const { return order_id; }
  };

  static_assert(BookContainer<BookContainerType<ActiveOrder>>);

  BookContainerType<ActiveOrder> book_container_;
};

template<template<typename> typename BookContainerType>
class MaintainBook {
public:
  explicit MaintainBook(const SecurityStorage& security_storage): security_storage_(security_storage) {}

  template<typename EventType>
  void handle_market_event(const EventType& event) {
    if constexpr (!std::is_same_v<EventType, market_events::Trade>) {
      if (security_storage_.has_security(event.security_id)) {
        book_storage_[event.security_id].handle(event);
      }
    }
  }

  void handle_request(const auto&) {}
  void handle_report(const auto&) {}

  const auto& book_storage() const { return book_storage_; }

private:
  const SecurityStorage& security_storage_;
  StrongTypeIndexedVector<SecurityID, BookStorage<BookContainerType>> book_storage_;
};

}  // namespace exchange_sim::generator_interface_skills
