#include "signals.h"

#include "base/threading/core_allocator.h"

namespace exchange_sim {

sigset_t shutdown_signal_set() {
  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  sigaddset(&signal_set, SIGTERM);
  sigaddset(&signal_set, SIGHUP);
  sigaddset(&signal_set, constWakeupSignal);
  return signal_set;
}

void block_shutdown_signals() {
  sigset_t signal_set = shutdown_signal_set();
  if (int rc = pthread_sigmask(SIG_BLOCK, &signal_set, nullptr); rc != 0) {
    throw std::runtime_error(std::format("pthread_sigmask failed, rc={}", rc));
  }
}

ShutdownSignalThread::ShutdownSignalThread(std::stop_source stop_source, CoreLease&& core_lease):
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


ShutdownSignalThread::~ShutdownSignalThread() {
  if (thread_.joinable()) {
    teardown_ = true;
    pthread_kill(thread_.native_handle(), constWakeupSignal);
    thread_.join();
  }
}

}  // namespace exchange_sim