#pragma once
#include "base/file.h"
#include "base/threaded_journal.h"
#include "base/threading/runtime.h"

#include "domain/security_storage.h"

#include "exchange/domain/market_events.h"
#include "exchange/domain/reports.h"
#include "exchange/domain/requests.h"

#include <glaze/json/generic_fwd.hpp>

namespace exchange_sim::concepts {

template<typename Generator>
concept OrderGenerator = market_events::Processor<Generator> &&
    std::constructible_from<Generator,
        const std::shared_ptr<Runtime>&,
        SecurityStorage&,
        ThreadedJournal<File, request::RequestType>&,
        ThreadedJournal<File, market_events::EventType>&,
        ThreadedJournal<File, report::ReportType>&,
        typename Generator::OutputSinkType&,
        const glz::generic&> &&
    report::Processor<Generator> && requires(Generator& generator, const Generator& const_generator) {
      { const_generator.generator_stopped() } -> std::same_as<bool>;
      { generator.generate_event() };
    };

}  // namespace exchange_sim::concepts
