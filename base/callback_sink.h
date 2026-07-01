#pragma once

#include "base/noncopyable.h"
#include <functional>

namespace exchange_sim {

template<typename ObjectType>
class CallbackSink : NonCopyable {
public:
  template<typename CallbackType>
    requires(
        !std::same_as<std::remove_cvref_t<CallbackType>, CallbackSink> && std::invocable<CallbackType, ObjectType &&>)
  explicit CallbackSink(CallbackType&& callback): callback_(std::forward<CallbackType>(callback)) {}

  CallbackSink(CallbackSink&&) = default;
  CallbackSink& operator=(CallbackSink&&) = default;

  void push(ObjectType&& object) { callback_(std::move(object)); }

private:
  std::function<void(ObjectType&&)> callback_;
};

}  // namespace exchange_sim