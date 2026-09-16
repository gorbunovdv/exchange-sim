#pragma once

#include "exchange/domain/engine_events.h"

#include <glaze/glaze.hpp>

namespace exchange_sim::concepts {

template<typename Dispatcher>
concept ExchangeDispatcher = engine_events::Processor<Dispatcher> &&
    std::constructible_from<Dispatcher,
        const glz::generic&,
        typename Dispatcher::OutputMDSinkType&,
        typename Dispatcher::OutputORSinkType&>;

}  // namespace exchange_sim::concepts
