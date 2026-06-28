#pragma once

#include "application/types.h"
#include <variant>

namespace exchange_sim {

struct OrderAddRequest {
  ClientID client_id;
  ClientOrderID client_order_id;
  SecurityID symbol_id;
  OrderTimeInForce time_in_force;
  OrderType order_type;
  Direction direction;
  Price price;
  Quantity quantity;
};

struct OrderModifyRequest {
  ClientID client_id;
  ClientOrderID client_order_id;
  SecurityID symbol_id;
  Direction direction;
  Price price;
  Quantity quantity;
};

struct OrderCancelRequest {
  ClientID client_id;
  ClientOrderID client_order_id;
  SecurityID symbol_id;
};

using RequestType = std::variant<OrderAddRequest, OrderModifyRequest, OrderCancelRequest>;

}  // namespace exchange_sim