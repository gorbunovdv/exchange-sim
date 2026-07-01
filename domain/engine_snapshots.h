#pragma once

#include "base/processor.h"
#include "base/sink.h"
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

enum class FilledType : std::uint8_t { PASSIVE_FILL, AGGRESSIVE_FILL };

struct OrderFilled {
  ExchangeOrderID order_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  SecurityID security_id;
  Direction direction;
  Price price;
  FilledType type;
};

struct OrderReduced {
  ExchangeOrderID order_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  SecurityID security_id;
  Direction direction;
  Price price;
  Amount remaining_quantity;
};

enum class ExpirationReason : std::uint8_t { IOC_REMAINDER_EXPIRED };

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

enum class OrderRejectedReason : std::uint8_t {
  SECURITY_NOT_FOUND,
  INSUFFICIENT_FUNDS,
  INVALID_QUANTITY,
  PRICE_OUT_OF_RANGE,
  DUPLICATE_CLIENT_ORDER_ID
};

struct OrderRejected {
  SecurityID security_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  OrderRejectedReason reason;
};

enum class CancellationRejectedReason : std::uint8_t { SECURITY_NOT_FOUND, ORDER_NOT_FOUND };

struct CancellationRejected {
  ClientID client_id;
  ClientOrderID client_order_id;
  ClientOrderID client_cancellation_id;
  SecurityID security_id;
  CancellationRejectedReason reason;
};

enum class ReplacementRejectedReason : std::uint8_t {
  SECURITY_NOT_FOUND,
  ORDER_NOT_FOUND,
  DUPLICATE_CLIENT_ORDER_ID,
  INSUFFICIENT_FUNDS,
  INVALID_QUANTITY,
  PRICE_OUT_OF_RANGE
};

struct ReplacementAccepted {
  ExchangeOrderID order_id;
  ExchangeOrderID new_order_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  ClientOrderID client_replacement_id;
  SecurityID security_id;
  Direction direction;
  Price price;
  Amount quantity;
};

struct ReplacementRejected {
  ClientID client_id;
  ClientOrderID client_order_id;
  ClientOrderID client_replacement_id;
  SecurityID security_id;
  ReplacementRejectedReason reason;
};

using SnapshotType = std::variant<OrderAccepted,
    Trade,
    OrderBookEntry,
    OrderFilled,
    OrderReduced,
    OrderExpired,
    OrderCancelled,
    OrderRejected,
    CancellationRejected,
    ReplacementAccepted,
    ReplacementRejected>;

template<typename SinkType>
concept Sink = SinkOf<SinkType, SnapshotType>;

template<typename ProcessorType>
concept Processor = ProcessorOf<ProcessorType, SnapshotType>;

}  // namespace exchange_sim::engine_snapshot