#pragma once

#include "base/logging/loggable_class.h"
#include "base/logging/logging_backend.h"
#include "base/noncopyable.h"
#include "base/spsc_queue.h"
#include "base/threading/core_allocator.h"
#include "base/throughput.h"
#include "base/time/rdtsc.h"
#include "base/time/timestamped.h"

#include <boost/stl_interfaces/iterator_interface.hpp>
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
  using value_type = Timestamped<POD>;

  static constexpr size_t value_size = sizeof(value_type);

  explicit Journal(const std::string& name,
      const std::filesystem::path& journal_path,
      JournalMode mode,
      size_t size,
      std::optional<CoreLease>&& core_lease = std::nullopt):
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
      thread_ = std::jthread([this](const std::stop_token& token) { run_write_journal(token); });
    } else {
      thread_ = std::jthread([this](const std::stop_token& token) { run_read_journal(token); });
    }
  }

  void write(const POD& value, Ticks timestamp) {
    CHECK(mode_ == JournalMode::WRITE, "Mode should be write");
    queue_.emplace(value, timestamp);
  }

  void write(const value_type& structure) {
    CHECK(mode_ == JournalMode::WRITE, "Mode should be write");
    queue_.emplace(structure);
  }

  template<typename Consumer>
  bool consume_if_available(Consumer&& consumer) {
    CHECK(mode_ == JournalMode::READ, "Mode should be read");
    do {
      if (queue_.consume(std::forward<Consumer>(consumer))) {
        return true;
      }
    } while (!stopped_.load(std::memory_order_acquire) || !queue_.empty());
    return false;
  }

  template<typename BaseType =
               boost::stl_interfaces::proxy_iterator_interface<std::input_iterator_tag, Journal::value_type>>
  class iterator : public BaseType {
  public:
    explicit iterator(Journal* journal): journal_(journal) {
      BOOST_STL_INTERFACES_STATIC_ASSERT_CONCEPT(iterator, std::input_iterator);
      advance();
    }

    iterator& operator++() {
      advance();
      return *this;
    }

    Journal::value_type operator*() const {
      using ::exchange_sim::get_logger;

      CHECK(current_value_.has_value(), "Iterator is not dereferenceable");
      return *current_value_;
    }

    const Journal::value_type* operator->() const {
      using ::exchange_sim::get_logger;

      CHECK(current_value_.has_value(), "Iterator is not dereferenceable");
      return &(*current_value_);
    }

    bool operator==(std::default_sentinel_t) const { return is_eof_; }

    using BaseType::operator++;

  private:
    void advance() {
      if (!is_eof_ && journal_) {
        is_eof_ = !journal_->consume_if_available([this](const value_type& value) { current_value_.emplace(value); });
      }
    }

    Journal* journal_;
    std::optional<value_type> current_value_;
    bool is_eof_ = false;
  };

  auto begin() {
    CHECK(mode_ == JournalMode::READ, "Mode should be read");
    return iterator<>(this);
  }

  auto end() { return std::default_sentinel; }

private:
  void flush() {
    queue_.consume_span([this](std::span<value_type> data) {
      if (!data.empty()) {
        stream_.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(value_type));
        CHECK(stream_, "Failed to write to journal, name={}", name_);
        throughput_.tick(data.size());
        return true;
      }
      return false;
    });
  }

  size_t fill() {
    return queue_.produce_span([this](std::span<value_type> data) {
      std::streamsize max_bytes_read = data.size() * sizeof(value_type);
      std::streamsize bytes_read = 0;
      while (bytes_read < max_bytes_read) {
        stream_.read(reinterpret_cast<char*>(data.data()), max_bytes_read - bytes_read);
        std::streamsize current_bytes_read = stream_.gcount();
        CHECK(!stream_.bad(), "Encountered error while reading from journal");
        bytes_read += current_bytes_read;
        if (!stream_ || current_bytes_read == 0) {
          break;
        }
      }
      if (bytes_read % sizeof(value_type) != 0) {
        ERROR("Got torn record in journal: read_bytes={} record_size={}", bytes_read, sizeof(value_type));
      }
      return bytes_read / sizeof(value_type);
    });
  }

  void run_read_journal(const std::stop_token& token) {
    CPPTRACE_TRY {
      if (core_lease_.has_value()) {
        core_lease_.value().pin_thread(name_);
      } else {
        WARNING("Not pinning journal thread as core lease is empty");
      }
      while (!token.stop_requested() && fill()) {
        std::this_thread::sleep_for(10ms);
      }
      stopped_.store(true, std::memory_order_release);
    } CPPTRACE_CATCH(...) {
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
      while (!token.stop_requested()) {
        flush();
        std::this_thread::sleep_for(10ms);
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

  std::fstream stream_;
  JournalMode mode_;

  SPSCQueue<value_type> queue_;

  Throughput throughput_;

  std::atomic<bool> stopped_ = false;

  std::jthread thread_;
};

}  // namespace exchange_sim
