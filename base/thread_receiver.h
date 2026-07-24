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
  explicit ThreadReceiver(std::string name, size_t queue_size, CoreLease lease, Args&&... args):
      LoggableClass(name),
      name_(name),
      lease_(std::move(lease)),
      base_class_(std::forward<Args>(args)...),
      sink_(queue_size),
      ready_to_receive_(false),
      thread_([this](const std::stop_token& stop_token) {
        CPPTRACE_TRY {
          lease_.pin_thread(name_);
          ready_to_receive_ = true;
          while (!stop_token.stop_requested()) {
            while (sink_.consume([this](ArgumentClass& object) { base_class_.process(std::move(object)); })) {
            }
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
      }) {}

  auto& get_sink() { return sink_; }

  BaseClass& base() { return base_class_; }

  bool ready_to_receive() { return ready_to_receive_; }

private:
  std::string name_;
  CoreLease lease_;
  BaseClass base_class_;
  SPSCQueue<ArgumentClass> sink_;
  std::atomic<bool> ready_to_receive_;
  std::jthread thread_;
};

}  // namespace exchange_sim
