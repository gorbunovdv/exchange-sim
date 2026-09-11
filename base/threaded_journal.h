#pragma once

#include "base/journal.h"
#include "base/spsc_queue.h"
#include "base/threading/core_allocator.h"

#include <cpptrace/from_current.hpp>
#include <cpptrace/from_current_macros.hpp>

#include <atomic>
#include <thread>

namespace exchange_sim {

template<typename FileType, typename POD>
  requires(std::is_trivially_copyable_v<POD> && !std::is_pointer_v<POD>)
class ThreadedJournal : NonMoveable, public LoggableClass {
public:
  using value_type = typename Journal<FileType, POD>::value_type;
  using iterator = JournalIterator<ThreadedJournal>;

  static constexpr size_t value_size = sizeof(value_type);

  enum class Mode : std::uint8_t { READ, WRITE };

  explicit ThreadedJournal(const std::string& name,
      FileType&& file,
      Mode mode,
      size_t size,
      std::optional<CoreLease>&& core_lease = std::nullopt):
      LoggableClass(name),
      name_(name),
      core_lease_(std::move(core_lease)),
      journal_(name,
          std::move(file),
          mode == Mode::WRITE ? decltype(journal_)::Mode::WRITE : decltype(journal_)::Mode::READ),
      mode_(mode),
      queue_(size) {
    CHECK(size > 1, "Journal size should be greater than 1");
    if (mode_ == Mode::WRITE) {
      thread_ = std::jthread([this](const std::stop_token& token) { run_write_journal(token); });
    } else {
      thread_ = std::jthread([this](const std::stop_token& token) { run_read_journal(token); });
    }
  }

  void write(const POD& value, Ticks timestamp) {
    CHECK(mode_ == Mode::WRITE, "Mode should be write");
    queue_.emplace(value, timestamp);
  }

  void write(const value_type& structure) {
    CHECK(mode_ == Mode::WRITE, "Mode should be write");
    queue_.emplace(structure);
  }

  template<typename Consumer>
  bool consume(Consumer&& consumer) {
    CHECK(mode_ == Mode::READ, "Mode should be read");
    auto fully_read_file = [this]() {
      return stopped_.load(std::memory_order_acquire) && queue_.empty();
    };

    do {
      if (queue_.consume(consumer)) {
        return true;
      }
      CHECK(fully_read_file(), "Producer is not fast enough in Journal");
    } while (!fully_read_file());

    return false;
  }

  auto begin() {
    CHECK(mode_ == Mode::READ, "Mode should be read");
    return iterator(this);
  }

  auto end() { return iterator(); }

  bool ready() const { return ready_.load(std::memory_order_acquire); }

private:
  void flush() {
    queue_.consume_span([this](std::span<value_type> data) {
      journal_.write(data);
      return !data.empty();
    });
  }

  void fill() {
    auto fill_impl = [&]() {
      queue_.produce_span([this](std::span<value_type> data) { return journal_.read(data); });
    };
    // We need to call fill_impl three times to fill the whole capacity
    fill_impl();
    fill_impl();
    fill_impl();
  }

  void run_read_journal(const std::stop_token& token) {
    CPPTRACE_TRY {
      if (core_lease_.has_value()) {
        core_lease_.value().pin_thread(name_);
      } else {
        WARNING("Not pinning journal thread as core lease is empty");
      }
      fill();
      ready_.store(true, std::memory_order_release);
      INFO("Journal thread has started to fill the queue, initial_filled_count={}", queue_.size());
      while (!token.stop_requested() && !journal_.eof()) {
        fill();
        std::this_thread::sleep_for(1ms);
      }
      stopped_.store(true, std::memory_order_release);
    }
    CPPTRACE_CATCH(...) {
      try {
        throw;
      } catch (const std::exception& e) {
        ERROR("Exception occurred: {}", e.what());
      } catch (...) {
        ERROR("Unknown exception occurred");
      }
      ERROR("\nStacktrace:");
      ERROR("{}", cpptrace::from_current_exception().to_string());
      flush_log();
      throw;
    }
  }

  void run_write_journal(const std::stop_token& token) {
    CPPTRACE_TRY {
      if (core_lease_.has_value()) {
        core_lease_.value().pin_thread(name_);
      } else {
        WARNING("Not pinning journal thread as core lease is empty");
      }
      ready_.store(true, std::memory_order_release);
      INFO("Journal thread has started to process the queue");
      while (!token.stop_requested()) {
        flush();
        std::this_thread::sleep_for(1ms);
      }
      while (!queue_.empty()) {
        flush();
      }
      stopped_.store(true, std::memory_order_release);
    }
    CPPTRACE_CATCH(...) {
      try {
        throw;
      } catch (const std::exception& e) {
        ERROR("Exception occurred: {}", e.what());
      } catch (...) {
        ERROR("Unknown exception occurred");
      }
      ERROR("\nStacktrace:");
      ERROR("{}", cpptrace::from_current_exception().to_string());
      flush_log();
      throw;
    }
  }

  std::string name_;
  std::optional<CoreLease> core_lease_;

  Journal<FileType, POD> journal_;
  Mode mode_;

  SPSCQueue<value_type> queue_;

  std::atomic<bool> ready_ = false;
  std::atomic<bool> stopped_ = false;

  std::jthread thread_;
};

}  // namespace exchange_sim
