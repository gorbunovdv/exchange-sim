#pragma once

#include "domain/types.h"

#include "exchange/domain/types.h"

#include <optional>
#include <type_traits>

namespace exchange_sim {

template<typename OrderType>
concept BookRecord = requires(OrderType order) {
  order.id();
  { order.price } -> std::same_as<Price&>;
  { order.quantity } -> std::same_as<Amount&>;
  { order.direction } -> std::same_as<Direction&>;
};

template<typename BookLevelType, typename OrderType>
concept BookLevel = BookRecord<OrderType> &&
    requires(
        std::remove_cvref_t<BookLevelType> level, OrderType order, std::remove_cvref_t<BookLevelType>::iterator it) {
      { level.empty() } -> std::same_as<bool>;
      { level.front() } -> std::same_as<OrderType&>;
      { level.push_back(order) } -> std::same_as<typename std::remove_cvref_t<BookLevelType>::iterator>;
      { level.erase_front() };
      { level.erase(it) };
      { level.begin() } -> std::same_as<typename std::remove_cvref_t<BookLevelType>::iterator>;
      { level.end() } -> std::same_as<typename std::remove_cvref_t<BookLevelType>::iterator>;
    };

template<typename HalfBookType, typename OrderType>
concept HalfBook = BookRecord<OrderType> && requires(std::remove_cvref_t<HalfBookType> half_book) {
  { half_book.empty() } -> std::same_as<bool>;
  { half_book.key_comp()(Price{0}, Price{0}) } -> std::same_as<bool>;
  { half_book.begin()->first } -> std::same_as<const Price&>;
  { half_book.begin()->second } -> BookLevel<OrderType>;
  { half_book.erase(half_book.begin()) };
};

template<typename BookContainerType>
concept BookContainer = BookLevel<typename std::remove_cvref_t<BookContainerType>::BookLevelType,
                            typename std::remove_cvref_t<BookContainerType>::OrderType> &&
    requires(std::remove_cvref_t<BookContainerType> book_container,
        std::remove_cvref_t<BookContainerType>::OrderType order) {
      { book_container.contains(order.id()) } -> std::same_as<bool>;
      { book_container.find(order.id()) } -> std::same_as<typename std::remove_cvref_t<BookContainerType>::OrderType*>;
      {
        book_container.template half<Direction::BUY>()
      } -> HalfBook<typename std::remove_cvref_t<BookContainerType>::OrderType>;
      {
        book_container.template half<Direction::SELL>()
      } -> HalfBook<typename std::remove_cvref_t<BookContainerType>::OrderType>;
      {
        book_container.template opposite_half<Direction::BUY>()
      } -> HalfBook<typename std::remove_cvref_t<BookContainerType>::OrderType>;
      {
        book_container.template opposite_half<Direction::SELL>()
      } -> HalfBook<typename std::remove_cvref_t<BookContainerType>::OrderType>;
      { book_container.template push_back<Direction::BUY>(order) };
      { book_container.template push_back<Direction::SELL>(order) };
      { book_container.template erase_front<Direction::BUY>() };
      { book_container.template erase_front<Direction::SELL>() };
      {
        book_container.extract_order(order.id())
      } -> std::same_as<std::optional<typename std::remove_cvref_t<BookContainerType>::OrderType>>;
    };

}  // namespace exchange_sim
