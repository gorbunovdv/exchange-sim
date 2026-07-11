#pragma once

#include "base/logging/loggable_class.h"
#include "base/logging/logging_backend.h"
#include "base/noncopyable.h"
#include "base/time/rdtsc.h"

#include "macros.h"
#include "throughput.h"

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

  explicit Journal(const std::string& name, const std::filesystem::path& journal_path, JournalMode mode, size_t size):
      LoggableClass(name),
      stream_(journal_path, (mode == JournalMode::WRITE ? std::ios::out : std::ios::in) | std::ios::binary),
      mode_(mode),
      queue_(size),
      throughput_(name) {
    if (!stream_.is_open()) {
      CRITICAL("Could not open filename={} mode={}", journal_path.string(), magic_enum::enum_name(mode));
    }
    if (mode == JournalMode::WRITE) {
      thread_ = std::jthread([this](const std::stop_token& token) {
        while (!token.stop_requested()) {
          flush();
          std::this_thread::yield();
        }
        while (head_ != tail_) {
          flush();
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
    value_type data;
    stream_.read(reinterpret_cast<char*>(data.data()), data.size());
    if (stream_) {
      std::forward<Consumer>(consumer)(data);
      return true;
    }
    return false;
  }

  ~Journal() { thread_.get_stop_source().request_stop(); }

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

  std::jthread thread_;

  std::fstream stream_;
  JournalMode mode_;

  // todo(gorbunovdv): Align the access to the value_type
  std::vector<std::array<std::byte, sizeof(value_type)>> queue_;

  // todo(gorbunovdv): Remove false sharing
  std::atomic<size_t> head_ = 0, tail_ = 0;

  Throughput throughput_;
};

}  // namespace exchange_sim
