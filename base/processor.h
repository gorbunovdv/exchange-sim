#pragma once

namespace exchange_sim {

template<typename Processor, typename Object>
concept ProcessorOf = requires(Processor& processor, const Object& object) { processor.process(object); };

}  // namespace exchange_sim
