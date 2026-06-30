#include "simple_l3book.h"

namespace exchange_sim {

std::optional<SimpleL3Book::ActiveOrder> SimpleL3Book::extract_order(
    ClientID client_id, ClientOrderID client_order_id) {
  if (auto active_order_map_iterator = active_orders_.find(std::make_pair(client_id, client_order_id));
      active_order_map_iterator != active_orders_.end()) {
    auto active_order_iterator = active_order_map_iterator->second;
    active_orders_.erase(active_order_map_iterator);
    ActiveOrder order = *active_order_iterator;
    book_[magic_enum::enum_integer(order.direction)][order.price].erase(active_order_iterator);
    if (book_[magic_enum::enum_integer(order.direction)][order.price].empty()) {
      book_[magic_enum::enum_integer(order.direction)].erase(order.price);
    }
    return order;
  }
  return std::nullopt;
}

}  // namespace exchange_sim