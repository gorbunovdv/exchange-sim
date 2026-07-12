#include "runtime.h"

#include "base/threading/signals.h"

namespace exchange_sim {

Runtime::Runtime(const Config& config):
    core_allocator_(config.core_allocator_config),
    shutdown_signal_(stop_source_, core_allocator_.lease_core(CoreType::SHARED)) {}

}  // namespace exchange_sim