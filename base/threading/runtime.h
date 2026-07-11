#pragma once
#include "base/threading/core_allocator.h"

namespace exchange_sim {

class Runtime {
public:
  struct Config {
    CoreAllocator::Config core_allocator_config;
  };

  explicit Runtime(Config config): core_allocator_(config.core_allocator_config) {}

  CoreAllocator& core_allocator() { return core_allocator_; }

private:
  CoreAllocator core_allocator_;
};

}  // namespace exchange_sim