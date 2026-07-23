#pragma once

#include "base/callback_sink.h"
#include "base/processor.h"

#include <functional>
#include <thread>

namespace exchange_sim {

template<typename ArgumentClass, typename BaseClass>
  requires ProcessorOf<BaseClass, ArgumentClass>
// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
class ThreadReceiver {
public:
  template<typename... Args>
  explicit ThreadReceiver(size_t queue_size, Args&&... args):
      base_class_(std::forward<Args>(args)...), sink_(queue_size), thread_([this](const std::stop_token& stop_token) {
        CPPTRACE_TRY {
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

private:
  BaseClass base_class_;
  SPSCQueue<ArgumentClass> sink_;
  std::jthread thread_;
};

}  // namespace exchange_sim
