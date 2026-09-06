#pragma once
#include "base/logging/logging_backend.h"
#include "base/threading/cpu_id.h"

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
  NONE,
};

class CoreAllocator;

class CoreLease : NonCopyable {
public:
  CpuID cpu_id() const {
    CHECK(cpu_id_.has_value(), "cpu_id is not set for CoreLease");
    return cpu_id_.value();
  }

  CoreType type() const { return core_type_; }

  void pin_thread(const std::string& name) const;

  ~CoreLease() noexcept;

  CoreLease(CoreLease&& other) noexcept:
      core_type_(other.core_type_),
      cpu_id_(std::exchange(other.cpu_id_, std::nullopt)),
      allocator_(std::exchange(other.allocator_, nullptr)) {}

  CoreLease& operator=(CoreLease&& other) noexcept {
    if (this != &other) {
      std::swap(core_type_, other.core_type_);
      std::swap(cpu_id_, other.cpu_id_);
      std::swap(allocator_, other.allocator_);
    }
    return *this;
  }

private:
  friend class CoreAllocator;

  explicit CoreLease(CoreType core_type, std::optional<CpuID> cpu_id, CoreAllocator* core_manager):
      core_type_(core_type), cpu_id_(cpu_id), allocator_(core_manager) {}

  CoreType core_type_;
  std::optional<CpuID> cpu_id_;
  CoreAllocator* allocator_ = nullptr;
};

class CoreAllocator : NonMoveable {
public:
  struct Config {
    bool fallback_to_none = false;
    std::optional<CpuID> shared_core;
    std::vector<CpuID> dedicated_cores;
  };

  explicit CoreAllocator(Config config):
      CoreAllocator(config.shared_core, std::move(config.dedicated_cores), config.fallback_to_none) {}

  explicit CoreAllocator(
      std::optional<CpuID> shared_core, std::vector<CpuID> dedicated_cores, bool fallback_to_none = false):
      fallback_to_none_(fallback_to_none), shared_core_(shared_core), dedicated_cores_(std::move(dedicated_cores)) {}

  CoreLease lease_core(CoreType type);

  void release_core(CpuID cpu_id, CoreType type);

private:
  mutable std::mutex mutex_;
  bool fallback_to_none_;
  std::optional<CpuID> shared_core_;
  std::vector<CpuID> dedicated_cores_;
};


}  // namespace exchange_sim
