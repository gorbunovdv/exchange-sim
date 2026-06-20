#include "loggable_class.h"

namespace exchange_sim {

LoggableClass::LoggableClass(const std::string& name): logger_(LoggingBackend::instance().create_logger(name)) {
}

}