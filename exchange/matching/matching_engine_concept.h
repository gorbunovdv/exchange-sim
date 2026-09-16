#pragma once

#include "domain/security_storage.h"

#include "exchange/domain/requests.h"

#include <glaze/glaze.hpp>

namespace exchange_sim::concepts {

template<typename Engine>
concept MatchingEngine = request::Processor<Engine> &&
    std::constructible_from<Engine, const glz::generic&, const SecurityStorage&, typename Engine::OutputSinkType&>;

}  // namespace exchange_sim::concepts
