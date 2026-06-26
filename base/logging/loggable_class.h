#pragma once

#include "base/logging/logging_backend.h"
#include <quill/Frontend.h>
#include <string>

namespace exchange_sim {

class LoggableClass {
public:
  LoggableClass(const std::string& name);

  void flush_log() { logger_->flush_log(); }

protected:
  quill::Logger* get_logger() { return logger_; }

private:
  quill::Logger* logger_;
};

}  // namespace exchange_sim
