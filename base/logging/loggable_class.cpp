#include "loggable_class.h"

namespace exchange_sim {

LoggableClass::LoggableClass(const std::string& name) {
  logger_ = quill::Frontend::create_or_get_logger(
    name,
    LoggingBackend::instance().get_sink(),
    LoggingBackend::instance().get_format()
  );
  logger_->set_log_level(LoggingBackend::instance().get_log_level());
}

}