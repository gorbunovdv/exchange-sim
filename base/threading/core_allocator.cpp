#include "core_allocator.h"

namespace exchange_sim {

namespace {

bool pin_current_thread_to_core(std::uint32_t core) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);
  return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
}

bool set_thread_name(std::string name) {
  return pthread_setname_np(pthread_self(), name.substr(0, 15).c_str()) == 0;
}

}  // namespace

void CoreLease::pin_thread(std::string name) {
  CHECK(pin_current_thread_to_core(cpu_id_),
      "Could not pin thread to cpu_id={} type={}",
      cpu_id_,
      magic_enum::enum_name(core_type_));
  CHECK(set_thread_name(name), "Could not set thread name to {}", name);
}

CoreLease::~CoreLease() {
  manager_->release_core(cpu_id_, core_type_);
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
  return CoreLease{CoreType::SHARED, shared_core_, this};
}

void CoreAllocator::release_core(CpuID cpu_id, CoreType type) {
  std::lock_guard lock(mutex_);
  if (type == CoreType::DEDICATED) {
    CHECK(std::ranges::find(dedicated_cores_, cpu_id) == dedicated_cores_.end(),
        "Core already released, cpu_id={}",
        cpu_id);
    dedicated_cores_.push_back(cpu_id);
  }
}

}  // namespace exchange_sim
