#pragma once

#include "base/callback_sink.h"
#include <functional>

namespace exchange_sim {

template<typename ArgumentClass, typename BaseClass>
class SimpleReceiver {
public:
  template<typename... Args>
  explicit SimpleReceiver(Args&&... args):
      base_class_(std::forward<Args>(args)...),
      sink_([this](ArgumentClass object) { return base_class_.process(std::move(object)); }) {}

  auto& get_sink() { return sink_; }

  BaseClass& base() { return base_class_; }

private:
  BaseClass base_class_;
  CallbackSink<ArgumentClass> sink_;
};

}  // namespace exchange_sim