#pragma once

#include "base/callback_sink.h"
#include "base/processor.h"

#include <functional>
#include <thread>

namespace exchange_sim {

template<typename ArgumentClass, typename BaseClass>
  requires ProcessorOf<BaseClass, ArgumentClass>
// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
class ThreadReceiver : public LoggableClass {
public:
  template<typename... Args>
  explicit ThreadReceiver(
      std::string name, size_t queue_size, CoreLease lease, std::stop_source stop_source, Args&&... args):
      LoggableClass(name),
      name_(name),
      lease_(std::move(lease)),
      stop_source_(std::move(stop_source)),
      base_class_(std::forward<Args>(args)...),
      sink_(queue_size),
      ready_to_receive_(false),
      thread_([this](const std::stop_token& stop_token) { run(stop_token); }) {}

  ~ThreadReceiver() {
    if (thread_.joinable()) {
      thread_.get_stop_source().request_stop();
      thread_.join();
    }
  }

  auto& get_sink() { return sink_; }

  BaseClass& base() { return base_class_; }

  bool ready_to_receive() { return ready_to_receive_; }

private:
  void run(std::stop_token stop_token) {
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
      stop_source_.request_stop();
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

  std::string name_;
  CoreLease lease_;
  std::stop_source stop_source_;
  BaseClass base_class_;
  SPSCQueue<ArgumentClass> sink_;
  std::atomic<bool> ready_to_receive_;
  std::jthread thread_;
};

}  // namespace exchange_sim
