#pragma once

#include "base/noncopyable.h"
#include <functional>

namespace exchange_sim {

template<typename ObjectType>
class CallbackSink : NonCopyable {
public:
  template<typename CallbackType>
    requires(!std::same_as<std::remove_cvref_t<CallbackType>, CallbackSink>)
  explicit CallbackSink(CallbackType&& callback): callback_(std::forward<CallbackType>(callback)) {}

  CallbackSink(CallbackSink&&) = default;
  CallbackSink& operator=(CallbackSink&&) = default;

  void push(ObjectType object) { callback_(std::move(object)); }

private:
  std::function<void(ObjectType)> callback_;
};

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