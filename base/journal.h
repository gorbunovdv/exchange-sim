#pragma once

#include "base/logging/loggable_class.h"
#include "base/logging/logging_backend.h"
#include "base/noncopyable.h"
#include "base/threading/core_allocator.h"
#include "base/throughput.h"
#include "base/time/rdtsc.h"

#include "cache_line.h"

#include <cpptrace/from_current.hpp>
#include <cpptrace/from_current_macros.hpp>
#include <magic_enum/magic_enum.hpp>

#include <filesystem>
#include <fstream>
#include <span>
#include <type_traits>

namespace exchange_sim {

enum class JournalMode : std::uint8_t { READ, WRITE };

template<typename POD>
  requires(std::is_trivially_copyable_v<POD> && !std::is_pointer_v<POD>)
class Journal : NonMoveable, public LoggableClass {
public:
  struct value_type {
    Ticks timestamp;
    POD value;
  };

  static constexpr size_t value_size = sizeof(value_type);

  explicit Journal(const std::string& name,
      const std::filesystem::path& journal_path,
      std::optional<CoreLease>&& core_lease,
      JournalMode mode,
      size_t size):
      LoggableClass(name),
      name_(name),
      core_lease_(std::move(core_lease)),
      stream_(journal_path, (mode == JournalMode::WRITE ? std::ios::out : std::ios::in) | std::ios::binary),
      mode_(mode),
      queue_(size),
      throughput_(name) {
    if (!stream_.is_open()) {
      CRITICAL("Could not open filename={} mode={}", journal_path.string(), magic_enum::enum_name(mode));
    }
    if (mode == JournalMode::WRITE) {
      thread_ = std::jthread([this](const std::stop_token& token) {
        CPPTRACE_TRY {
          CHECK(core_lease_.has_value(), "Must have core lease to start writing journal");
          core_lease_.value().pin_thread(name_);
          while (!token.stop_requested()) {
            flush();
            std::this_thread::yield();
          }
          while (head_ != tail_) {
            flush();
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
      });
    }
  }

  void write(Ticks timestamp, const POD& value) {
    CHECK(mode_ == JournalMode::WRITE, "Mode should be write");
    // todo(gorbunovdv): Use correct memory order
    size_t current_tail = tail_.load(), current_head = head_.load();
    size_t next_head = (current_head + 1) % queue_.size();
    CHECK(next_head != current_tail, "Queue is fully filled");
    std::construct_at(reinterpret_cast<value_type*>(queue_.data() + current_head), timestamp, value);
    head_.store(next_head);
  }

  template<typename Consumer>
  bool consume_if_available(Consumer&& consumer) {
    CHECK(mode_ == JournalMode::READ, "Mode should be read");
    alignas(value_type) std::array<std::byte, sizeof(value_type)> data;
    stream_.read(reinterpret_cast<char*>(data.data()), data.size());
    if (stream_) {
      std::forward<Consumer>(consumer)(std::move(*reinterpret_cast<value_type*>(data.data())));
      return true;
    }
    return false;
  }

  ~Journal() { thread_.get_stop_source().request_stop(); }

  class Iterator {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type = Journal::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;

    explicit Iterator(Journal* journal): journal_(journal) { advance(); }

    Iterator& operator++() {
      advance();
      return *this;
    }

    pointer operator->() {
      using ::exchange_sim::get_logger;

      CHECK(current_value_.has_value(), "Iterator is not dereferenceable");
      return &(*current_value_);
    }

    reference operator*() {
      using ::exchange_sim::get_logger;

      CHECK(current_value_.has_value(), "Iterator is not dereferenceable");
      return *current_value_;
    }

    bool operator==(std::default_sentinel_t) const { return is_eof_; }

  private:
    void advance() {
      if (!is_eof_ && journal_ && journal_->stream_) {
        is_eof_ =
            !journal_->consume_if_available([this](value_type&& value) { current_value_.emplace(std::move(value)); });
      }
    }

    Journal* journal_;
    std::optional<value_type> current_value_;
    bool is_eof_ = false;
  };

  auto begin() {
    CHECK(mode_ == JournalMode::READ, "Mode should be read");
    return Iterator(this);
  }

  auto end() { return std::default_sentinel; }

private:
  void flush() {
    size_t current_head = head_.load(), current_tail = tail_.load();
    size_t last_head = current_tail <= current_head ? current_head : queue_.size();
    std::span data{queue_.data() + current_tail, queue_.data() + last_head};
    if (!data.empty()) {
      stream_.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(value_type));
      throughput_.tick(data.size());
    }
    tail_.store(last_head % queue_.size());
  }

  std::string name_;
  std::optional<CoreLease> core_lease_;

  std::fstream stream_;
  JournalMode mode_;

  struct alignas(value_type) AlignedType {
    std::array<std::byte, sizeof(value_type)> placeholder;
  };

  static_assert(sizeof(AlignedType) == sizeof(value_type), "AlignedType should have the same size as value_type");

  std::vector<AlignedType> queue_;

  alignas(constCacheLineSize) std::atomic<size_t> head_ = 0;
  alignas(constCacheLineSize) std::atomic<size_t> tail_ = 0;

  Throughput throughput_;

  std::jthread thread_;
};

}  // namespace exchange_sim
