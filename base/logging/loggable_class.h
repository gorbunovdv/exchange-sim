#pragma once
#include "logging_backend.h"
#include <quill/Frontend.h>
#include <string>

namespace exchange_sim {

class LoggableClass {
public:
  LoggableClass(const std::string& name);

  ~LoggableClass() {
    logger_->flush_log();
  }

  quill::Logger* get_logger() {
    return logger_;
  }

private:
  quill::Logger* logger_;
};

}
