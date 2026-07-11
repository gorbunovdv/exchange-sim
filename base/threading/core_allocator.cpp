#include "core_allocator.h"

#include <cpptrace/from_current.hpp>

#include <iostream>
#include <ranges>

#ifdef __APPLE__
#include "base/macros.h"

#include <mach/mach.h>
#include <mach/thread_policy.h>
#endif

namespace exchange_sim {

namespace {

bool pin_current_thread_to_core(CpuID core) {
#ifdef __APPLE__
  UNUSED(core);
  return true;
#else
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core.value(), &cpuset);
  return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
#endif
}

bool set_thread_name(const std::string& name) {
#ifdef __APPLE__
  return pthread_setname_np(name.substr(0, 63).c_str()) == 0;
#else
  return pthread_setname_np(pthread_self(), name.substr(0, 15).c_str()) == 0;
#endif
}

}  // namespace

void CoreLease::pin_thread(const std::string& name) const {
  CHECK(pin_current_thread_to_core(cpu_id_),
      "Could not pin thread to cpu_id={} type={}",
      cpu_id_.value(),
      magic_enum::enum_name(core_type_));
  CHECK(set_thread_name(name), "Could not set thread name to {}", name);
}

CoreLease::~CoreLease() noexcept {
  CPPTRACE_TRY {
    if (allocator_ != nullptr) {
      allocator_->release_core(cpu_id_, core_type_);
    }
  }
  CPPTRACE_CATCH(...) {
    try {
      try {
        throw;
      } catch (const std::exception& e) {
        ERROR("Exception occurred: {}", e.what());
      } catch (...) {
        ERROR("Unknown exception occurred");
      }
      ERROR("\nStacktrace:");
      ERROR("{}", cpptrace::from_current_exception().to_string());
    } catch (...) {
      std::cerr << "Logger threw during handling another exception" << '\n';
    }
  }
}

CoreLease CoreAllocator::lease_core(CoreType type) {
  std::lock_guard lock(mutex_);
  if (type == CoreType::DEDICATED) {
    if (dedicated_cores_.empty()) {
      ERROR("No dedicated cores available, returning shared");
      return CoreLease{CoreType::SHARED, shared_core_, this};
    }
    auto cpu_id = dedicated_cores_.back();
    dedicated_cores_.pop_back();
    return CoreLease{CoreType::DEDICATED, cpu_id, this};
  }
  return CoreLease{CoreType::SHARED, shared_core_, nullptr};
}

void CoreAllocator::release_core(CpuID cpu_id, CoreType type) {
  std::lock_guard lock(mutex_);
  if (type == CoreType::DEDICATED) {
    CHECK(std::ranges::find(dedicated_cores_, cpu_id) == dedicated_cores_.end(),
        "Core already released, cpu_id={}",
        cpu_id.value());
    dedicated_cores_.push_back(cpu_id);
  }
}

}  // namespace exchange_sim
