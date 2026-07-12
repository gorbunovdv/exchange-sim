#include "runtime.h"

namespace exchange_sim {

ShutdownSignal::ShutdownSignal(std::stop_source stop_source, CoreLease&& core_lease):
    stop_source_(std::move(stop_source)), core_lease_(std::move(core_lease)), thread_([this]() {
      core_lease_.pin_thread("signal_thread");
      sigset_t signal_set = shutdown_signal_set();
      for (;;) {
        int sig = 0;
        int rc = sigwait(&signal_set, &sig);
        if (rc == EINTR) {
          continue;
        }
        CHECK(rc == 0, "sigwait failed, rc={}", rc);
        if (sig == constWakeupSignal) {
          if (teardown_.load()) {
            return;
          }
          continue;
        }
        if (stop_source_.stop_requested()) {
          std::_Exit(128 + sig);
        }
        INFO("Received signal={}, graceful shutdown (Ctrl-C again to abort)", sig);
        stop_source_.request_stop();
      }
    }) {}


ShutdownSignal::~ShutdownSignal() {
  if (thread_.joinable()) {
    teardown_ = true;
    pthread_kill(thread_.native_handle(), constWakeupSignal);
    thread_.join();
  }
}


Runtime::Runtime(const Config& config):
    core_allocator_(config.core_allocator_config),
    shutdown_signal_(stop_source_, core_allocator_.lease_core(CoreType::SHARED)) {}

}  // namespace exchange_sim