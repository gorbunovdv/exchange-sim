#pragma once

#include "domain/security_storage.h"

#include "exchange/book_containers/book_container_concept.h"
#include "exchange/domain/market_events.h"

namespace exchange_sim::generator_interface_skills {

template<template<typename, typename> typename BookContainerType>
class BookStorage {
public:
  BookStorage() { static_assert(BookContainer<BookContainerType<ActiveOrder, UniqueIDGetter>>); }

  void handle(const market_events::BookOrderEntry& event) {
    ActiveOrder order{event.order_id, event.quantity};
    if (event.direction == Direction::BUY) {
      book_container_.template push_back<Direction::BUY>(order);
    } else {
      book_container_.template push_back<Direction::SELL>(order);
    }
  }

  void handle(const market_events::BookOrderChange& event) {
    CHECK(book_container_.extract_order(event.order_id).has_value(),
        "Order has not been found in L3Book, order_id={}",
        event.order_id);
    if (event.direction == Direction::BUY) {
      book_container_.template push_back<Direction::BUY>(ActiveOrder{event.order_id, event.new_quantity});
    } else {
      book_container_.template erase_front<Direction::SELL>(ActiveOrder{event.order_id, event.new_quantity});
    }
  }

  void handle(const market_events::BookOrderDeletion& event) {
    CHECK(book_container_.extract_order(event.order_id).has_value(),
        "Order has not been found in L3Book, order_id={}",
        event.order_id);
  }

  const auto& container() const { return book_container_; }

private:
  struct ActiveOrder {
    ExchangeOrderID order_id;
    Amount quantity;
  };

  using UniqueIDGetter = decltype([](const ActiveOrder& active_order) { return active_order.order_id; });

  BookContainerType<ActiveOrder, UniqueIDGetter> book_container_;
};

template<template<typename, typename> typename BookContainerType>
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
