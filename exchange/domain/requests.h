#pragma once

#include "base/sink.h"
#include "base/processor.h"
#include "domain/security.h"
#include "domain/types.h"
#include "exchange/domain/types.h"
#include <variant>

namespace exchange_sim::request {

struct OrderAddRequest {
  ClientID client_id;
  ClientOrderID client_order_id;
  SecurityID security_id;
  OrderTimeInForce time_in_force;
  OrderType order_type;
  Direction direction;
  Price price;
  Amount quantity;
};

struct OrderReplaceRequest {
  ClientID client_id;
  ClientOrderID client_order_id;
  ClientOrderID client_replacement_id;
  SecurityID security_id;
  Direction direction;
  Price price;
  Amount quantity;
};

struct OrderCancelRequest {
  ClientID client_id;
  ClientOrderID client_order_id;
  ClientOrderID client_cancellation_id;
  SecurityID security_id;
};

using RequestType = std::variant<OrderAddRequest, OrderReplaceRequest, OrderCancelRequest>;

template<typename SinkType>
concept Sink = SinkOf<SinkType, RequestType>;

template<typename ProcessorType>
concept Processor = ProcessorOf<ProcessorType, RequestType>;

}  // namespace exchange_sim::request