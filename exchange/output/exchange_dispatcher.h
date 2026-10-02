#pragma once

#include "exchange/domain/engine_events.h"
#include "exchange/domain/market_events.h"
#include "exchange/domain/reports.h"

#include <glaze/glaze.hpp>

namespace exchange_sim {

template<market_events::Sink MDSinkType, report::Sink ORSinkType>
class ExchangeDispatcher {
public:
  using OutputMDSinkType = MDSinkType;
  using OutputORSinkType = ORSinkType;

  explicit ExchangeDispatcher(
      const glz::generic&, OutputMDSinkType& market_data_sink, OutputORSinkType& report_generator_sink):
      market_data_sink_(market_data_sink), report_generator_sink_(report_generator_sink) {
    static_assert(engine_events::Processor<ExchangeDispatcher>);
  }

  void process(const engine_events::EventType& event) {
    std::visit([this](const auto& event) { this->process_event(event); }, event);
  }

private:
  void process_event(const engine_events::OrderAccepted& event) { report_generator_sink_.push(event); }

  void process_event(const engine_events::Trade& event) {
    market_data_sink_.push(market_events::Trade{
        .origin_time = event.origin_time,
        .security_id = event.security_id,
        .trade_id = event.trade_id,
        .aggressor_order_id = event.aggressor_order_id,
        .recipient_order_id = event.recipient_order_id,
        .price = event.price,
        .quantity = event.quantity,
    });
    report_generator_sink_.push(report::Trade{
        .origin_time = event.origin_time,
        .security_id = event.security_id,
        .client_id = event.aggressor_client_id,
        .client_order_id = event.aggressor_client_order_id,
        .trade_id = event.trade_id,
        .aggressor_order_id = event.aggressor_order_id,
        .recipient_order_id = event.recipient_order_id,
        .price = event.price,
        .quantity = event.quantity,
        .aggressor_direction = event.aggressor_direction,
    });
    report_generator_sink_.push(report::Trade{
        .origin_time = event.origin_time,
        .security_id = event.security_id,
        .client_id = event.recipient_client_id,
        .client_order_id = event.recipient_client_order_id,
        .trade_id = event.trade_id,
        .aggressor_order_id = event.aggressor_order_id,
        .recipient_order_id = event.recipient_order_id,
        .price = event.price,
        .quantity = event.quantity,
        .aggressor_direction = event.aggressor_direction,
    });
  }

  void process_event(const engine_events::OrderBookEntry& event) {
    market_data_sink_.push(market_events::BookOrderEntry{
        .origin_time = event.origin_time,
        .order_id = event.order_id,
        .security_id = event.security_id,
        .price = event.price,
        .quantity = event.quantity,
        .direction = event.direction,
    });
    report_generator_sink_.push(event);
  }

  void process_event(const engine_events::OrderFilled& event) {
    if (event.type == engine_events::FilledType::PASSIVE_FILL) {
      market_data_sink_.push(market_events::BookOrderDeletion{
          .origin_time = event.origin_time,
          .order_id = event.order_id,
          .security_id = event.security_id,
      });
    }
    report_generator_sink_.push(event);
  }

  void process_event(const engine_events::OrderReduced& event) {
    market_data_sink_.push(market_events::BookOrderChange{
        .origin_time = event.origin_time,
        .order_id = event.order_id,
        .security_id = event.security_id,
        .price = event.price,
        .new_quantity = event.remaining_quantity,
        .direction = event.direction,
    });
  }

  void process_event(const engine_events::OrderExpired& event) { report_generator_sink_.push(event); }

  void process_event(const engine_events::OrderCancelled& event) {
    market_data_sink_.push(market_events::BookOrderDeletion{
        .origin_time = event.origin_time,
        .order_id = event.order_id,
        .security_id = event.security_id,
    });
    report_generator_sink_.push(event);
  }

  void process_event(const engine_events::OrderRejected& event) { report_generator_sink_.push(event); }

  void process_event(const engine_events::CancellationRejected& event) { report_generator_sink_.push(event); }

  void process_event(const engine_events::ReplacementAccepted& event) {
    market_data_sink_.push(market_events::BookOrderDeletion{
        .origin_time = event.origin_time,
        .order_id = event.order_id,
        .security_id = event.security_id,
    });
    report_generator_sink_.push(event);
  }

  void process_event(const engine_events::ReplacementRejected& event) { report_generator_sink_.push(event); }

  OutputMDSinkType& market_data_sink_;
  OutputORSinkType& report_generator_sink_;
};

}  // namespace exchange_sim
