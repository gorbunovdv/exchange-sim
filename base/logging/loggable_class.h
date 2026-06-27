#pragma once

#include "base/logging/logging_backend.h"
#include <quill/Logger.h>
#include <string>

namespace exchange_sim {

class LoggableClass {
public:
  void flush_log() const { logger_->flush_log(); }

protected:
  explicit LoggableClass(const std::string& name);

  quill::Logger* get_logger() const { return logger_; }

private:
  quill::Logger* logger_;
};

}  // namespace exchange_sim
