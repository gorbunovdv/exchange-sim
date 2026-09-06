#pragma once

#include "base/journal.h"

#include "exchange/domain/requests.h"

#include <boost/hof/match.hpp>
#include <glaze/glaze.hpp>

#include <concepts>

namespace exchange_sim {

template<typename GeneratorInterface>
class ReplayOrderGenerator {
public:
  explicit ReplayOrderGenerator(GeneratorInterface& interface, const glz::generic& config):
      interface_(interface),
      max_pending_transactions_(config["max_pending_transactions"].as<std::uint32_t>()),
      replay_journal_("replay_journal", config["generated_replay"].as<std::string>(), JournalMode::READ),
      current_request_(replay_journal_.begin()) {}

  void process_market_event(const auto&) {}
  void process_report(const auto&) {}

  void generate_event() {
    if (interface_.pending_transactions() >= max_pending_transactions_) {
      return;
    }
    if (current_request_ == replay_journal_.end()) {
      interface_.inform_generator_stopped();
      return;
    }
    std::visit(
        boost::hof::match(
            [this](const request::OrderAddRequest& request) {
              auto client_order_id = interface_.send_order_add_request(request.client_id,
                  request.security_id,
                  request.time_in_force,
                  request.order_type,
                  request.direction,
                  request.price,
                  request.quantity);
              CHECK(request.client_order_id == client_order_id,
                  "Replay and generated order_ids must match, replay_client_order_id={} generated_client_order_id={}",
                  client_order_id.value(),
                  request.client_order_id.value());
            },
            [this](const request::OrderCancelRequest& request) {
              auto client_cancellation_id =
                  interface_.send_order_cancel_request(request.client_id, request.client_order_id, request.security_id);
              CHECK(request.client_cancellation_id == client_cancellation_id,
                  "Replay and generated cancellation_ids must match, replay_client_cancellation_id={} "
                  "generated_client_cancellation_id={}",
                  client_cancellation_id.value(),
                  request.client_cancellation_id.value());
            },
            [this](const request::OrderReplaceRequest& request) {
              auto client_replacement_id = interface_.send_order_replace_request(request.client_id,
                  request.client_order_id,
                  request.security_id,
                  request.direction,
                  request.price,
                  request.quantity);
              CHECK(request.client_replacement_id == client_replacement_id,
                  "Replay and generated replacement_ids must match, replay_client_replacement_id={} "
                  "generated_client_replacement_id={}",
                  client_replacement_id.value(),
                  request.client_replacement_id.value());
            }),
        current_request_->object);
    current_request_++;
  }

private:
  GeneratorInterface& interface_;

  const uint32_t max_pending_transactions_;

  Journal<request::RequestType> replay_journal_;
  decltype(replay_journal_)::iterator<> current_request_;
};

}  // namespace exchange_sim