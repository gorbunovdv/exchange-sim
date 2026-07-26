#pragma once

#include "base/logging/loggable_class.h"
#include "base/processor.h"
#include "base/spsc_queue.h"
#include "base/threading/core_allocator.h"
#include "base/time/rdtsc.h"

#include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>

#include <functional>
#include <thread>

namespace exchange_sim {

template<typename ArgumentClass, typename BaseClass>
  requires ProcessorOf<BaseClass, ArgumentClass>
// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
class ThreadReceiver : public LoggableClass {
public:
  template<typename... Args>
  explicit ThreadReceiver(std::string name, size_t queue_size, CoreLease lease, Args&&... args):
      LoggableClass(name),
      name_(std::move(name)),
      lease_(std::move(lease)),
      base_class_(std::forward<Args>(args)...),
      sink_(queue_size),
      ready_to_receive_(false),
      thread_([this](const std::stop_token& stop_token) { run(stop_token); }) {}

  auto& get_sink() { return sink_; }

  BaseClass& base() { return base_class_; }

  bool ready_to_receive() { return ready_to_receive_; }

private:
  void run(const std::stop_token& stop_token) {
    CPPTRACE_TRY {
      auto consumer = [this](auto&& event) {
        base_class_.process(std::forward<decltype(event)>(event));
      };

      lease_.pin_thread(name_);
      ready_to_receive_ = true;
      while (!stop_token.stop_requested()) {
        sink_.consume(consumer);
      }
      Ticks latest_event = Ticks::rdtsc();
      while (latest_event.nanoseconds_till_now() < 100ms) {
        if (sink_.consume(consumer)) {
          latest_event = Ticks::rdtsc();
        }
      }
      CHECK(sink_.empty(), "Sink must be empty prior to exit");
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
      LoggableClass::flush_log();
      throw;
    }
  }

  std::string name_;
  CoreLease lease_;
  BaseClass base_class_;
  SPSCQueue<ArgumentClass> sink_;
  std::atomic<bool> ready_to_receive_;
  std::jthread thread_;
};

}  // namespace exchange_sim
