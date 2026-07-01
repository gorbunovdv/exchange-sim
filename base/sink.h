#pragma once
#include <utility>

namespace exchange_sim {

template<typename Sink, typename Object>
concept SinkOf = requires(Sink& sink, Object object) { sink.push(std::move(object)); };

}  // namespace exchange_sim