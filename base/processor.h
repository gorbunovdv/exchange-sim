#pragma once
#include <utility>

namespace exchange_sim {

template<typename Processor, typename Object>
concept ProcessorOf = requires(Processor& processor, Object object) { processor.process(std::move(object)); };

}  // namespace exchange_sim
