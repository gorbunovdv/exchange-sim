#pragma once

#include "base/processor.h"
#include "base/sink.h"
#include "domain/types.h"
#include "exchange/domain/types.h"
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

  bool operator==(const OrderAccepted& other) const = default;
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

  bool operator==(const Trade& other) const = default;
};

struct OrderBookEntry {
  ExchangeOrderID order_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  SecurityID security_id;
  Direction direction;
  Price price;
  Amount quantity;

  bool operator==(const OrderBookEntry& other) const = default;
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

  bool operator==(const OrderFilled& other) const = default;
};

struct OrderReduced {
  ExchangeOrderID order_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  SecurityID security_id;
  Direction direction;
  Price price;
  Amount remaining_quantity;

  bool operator==(const OrderReduced& other) const = default;
};

enum class ExpirationReason : std::uint8_t { IOC_REMAINDER_EXPIRED };

struct OrderExpired {
  ExchangeOrderID order_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  SecurityID security_id;
  ExpirationReason reason;

  bool operator==(const OrderExpired& other) const = default;
};

struct OrderCancelled {
  ExchangeOrderID order_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  ClientOrderID client_cancellation_id;
  SecurityID security_id;

  bool operator==(const OrderCancelled& other) const = default;
};

enum class OrderRejectedReason : std::uint8_t {
  SECURITY_NOT_FOUND,
  INSUFFICIENT_FUNDS,
  INVALID_QUANTITY,
  PRICE_OUT_OF_RANGE,
  PRICE_NOT_MULTIPLE_OF_TICK_SIZE,
  DUPLICATE_CLIENT_ORDER_ID
};

struct OrderRejected {
  SecurityID security_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  OrderRejectedReason reason;

  bool operator==(const OrderRejected& other) const = default;
};

enum class CancellationRejectedReason : std::uint8_t { SECURITY_NOT_FOUND, ORDER_NOT_FOUND };

struct CancellationRejected {
  ClientID client_id;
  ClientOrderID client_order_id;
  ClientOrderID client_cancellation_id;
  SecurityID security_id;
  CancellationRejectedReason reason;

  bool operator==(const CancellationRejected& other) const = default;
};

enum class ReplacementRejectedReason : std::uint8_t {
  SECURITY_NOT_FOUND,
  ORDER_NOT_FOUND,
  DUPLICATE_CLIENT_ORDER_ID,
  INSUFFICIENT_FUNDS,
  INVALID_QUANTITY,
  PRICE_NOT_MULTIPLE_OF_TICK_SIZE,
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

  bool operator==(const ReplacementAccepted& other) const = default;
};

struct ReplacementRejected {
  ClientID client_id;
  ClientOrderID client_order_id;
  ClientOrderID client_replacement_id;
  SecurityID security_id;
  ReplacementRejectedReason reason;

  bool operator==(const ReplacementRejected& other) const = default;
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