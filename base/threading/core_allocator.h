#pragma once
#include "base/logging/logging_backend.h"

#include <boost/scope/scope_exit.hpp>
#include <magic_enum/magic_enum.hpp>

#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace exchange_sim {

enum class CoreType : std::uint8_t {
  DEDICATED,
  SHARED,
};

using CpuID = uint32_t;

class CoreAllocator;

class CoreLease : NonCopyable {
public:
  CpuID cpu_id() const { return cpu_id_; }

  CoreType type() const { return core_type_; }

  void pin_thread(std::string name);

  ~CoreLease();

  CoreLease(CoreLease&&) = default;
  CoreLease& operator=(CoreLease&&) = default;

private:
  friend class CoreAllocator;

  explicit CoreLease(CoreType core_type, CpuID cpu_id, CoreAllocator* core_manager):
      core_type_(core_type), cpu_id_(cpu_id), manager_(core_manager) {}

  CoreType core_type_;
  CpuID cpu_id_;
  CoreAllocator* manager_ = nullptr;
};

class CoreAllocator : NonMoveable {
public:
  struct Config {
    uint32_t shared_core;
    std::vector<uint32_t> dedicated_cores;
  };

  explicit CoreAllocator(Config config): CoreAllocator(config.shared_core, std::move(config.dedicated_cores)) {}

  explicit CoreAllocator(uint32_t shared_core, std::vector<uint32_t> dedicated_cores):
      shared_core_(shared_core), dedicated_cores_(std::move(dedicated_cores)) {}

  CoreLease lease_core(CoreType type);

  void release_core(CpuID cpu_id, CoreType type);

private:
  mutable std::mutex mutex_;
  uint32_t shared_core_;
  std::vector<uint32_t> dedicated_cores_;
};


}  // namespace exchange_sim
