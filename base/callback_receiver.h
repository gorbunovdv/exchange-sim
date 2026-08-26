#pragma once

#include "base/callback_sink.h"
#include "base/processor.h"

#include <functional>

namespace exchange_sim {

template<typename ArgumentClass, typename BaseClass>
  requires ProcessorOf<BaseClass, ArgumentClass>
class CallbackReceiver {
public:
  template<typename... Args>
  explicit CallbackReceiver(Args&&... args): base_class_(std::forward<Args>(args)...) {}

  auto& get_sink() { return *this; }

  BaseClass& base() { return base_class_; }

  void push(ArgumentClass&& object) { base_class_.process(std::move(object)); }

private:
  BaseClass base_class_;
};

}  // namespace exchange_sim
