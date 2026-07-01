#include "simple_l3book.h"

namespace exchange_sim {

SimpleL3Book::SimpleL3Book(
    const Security& security, Counter<ExchangeOrderID>& order_id_counter, Counter<ExchangeTradeID>& trade_id_counter):
    security_(security),
    risk_checker_(security),
    order_id_counter_(order_id_counter),
    trade_id_counter_(trade_id_counter) {}

std::optional<SimpleL3Book::ActiveOrder> SimpleL3Book::extract_order(
    ClientID client_id, ClientOrderID client_order_id) {
  if (auto active_order_map_iterator = active_orders_.find(std::make_pair(client_id, client_order_id));
      active_order_map_iterator != active_orders_.end()) {
    auto active_order_iterator = active_order_map_iterator->second;
    active_orders_.erase(active_order_map_iterator);
    ActiveOrder order = *active_order_iterator;
    book_[order.direction][order.price].erase(active_order_iterator);
    if (book_[order.direction][order.price].empty()) {
      book_[order.direction].erase(order.price);
    }
    return order;
  }
  return std::nullopt;
}

}  // namespace exchange_sim