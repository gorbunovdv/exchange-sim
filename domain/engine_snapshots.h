#pragma once

#include "domain/types.h"
#include <variant>

namespace exchange_sim::engine_snapshot {

struct OrderAccepted {
  ExchangeOrderID order_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  SecurityID security_id;
  OrderTimeInForce time_in_force;
  Direction direction;
  Price price;
  Amount quantity;
};

struct Trade {
  SecurityID security_id;
  Direction aggressor_direction;
  Price price;
  Amount quantity;
  ExchangeTradeID trade_id;
  ExchangeOrderID aggressor_order_id;
  ClientID aggressor_client_id;
  ClientOrderID aggressor_client_order_id;
  ExchangeOrderID recipient_order_id;
  ClientID recipient_client_id;
  ClientOrderID recipient_client_order_id;
};

struct OrderBookEntry {
  ExchangeOrderID order_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  SecurityID security_id;
  Direction direction;
  Price price;
  Amount quantity;
};

enum class ExpirationReason : std::uint8_t { PASSIVE_FILL, AGGRESSIVE_FILL, IOC_REMAINDER_EXPIRED, MODIFIED_EXPIRED };

struct OrderExpired {
  ExchangeOrderID order_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  SecurityID security_id;
  ExpirationReason reason;
};

struct OrderCancelled {
  ExchangeOrderID order_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  ClientOrderID client_cancellation_id;
  SecurityID security_id;
};

enum class OrderRejectedReason : std::uint8_t { INSUFFICIENT_FUNDS, PRICE_OUT_OF_RANGE, DUPLICATE_CLIENT_ORDER_ID };

struct OrderRejected {
  SecurityID security_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  OrderRejectedReason reason;
};

enum class CancellationRejectedReason : std::uint8_t { ORDER_NOT_FOUND };

struct CancellationRejected {
  ClientID client_id;
  ClientOrderID client_order_id;
  ClientOrderID client_cancellation_id;
  SecurityID security_id;
  CancellationRejectedReason reason;
};

enum class ModificationRejectedReason : std::uint8_t {
  ORDER_NOT_FOUND,
  DUPLICATE_CLIENT_ORDER_ID,
  INSUFFICIENT_FUNDS,
  PRICE_OUT_OF_RANGE
};

struct ModificationAccepted {
  ExchangeOrderID order_id;
  ExchangeOrderID new_order_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  ClientOrderID client_modification_id;
  SecurityID security_id;
  Direction direction;
  Price price;
  Amount quantity;
};

struct ModificationRejected {
  ClientID client_id;
  ClientOrderID client_order_id;
  ClientOrderID client_modification_id;
  SecurityID security_id;
  ModificationRejectedReason reason;
};

using SnapshotType = std::variant<OrderAccepted,
    Trade,
    OrderBookEntry,
    OrderExpired,
    OrderCancelled,
    OrderRejected,
    CancellationRejected,
    ModificationAccepted,
    ModificationRejected>;

}  // namespace exchange_sim::engine_snapshot