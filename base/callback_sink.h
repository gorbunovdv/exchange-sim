#pragma once

#include "base/noncopyable.h"
#include "base/traits.h"

#include <functional>

namespace exchange_sim {

template<typename ObjectType, typename CallbackType>
class CallbackSink : NonCopyable {
public:
  explicit CallbackSink(auto&&... args): callback_(std::forward<decltype(args)>(args)...) {}

  CallbackSink(CallbackSink&&) = default;
  CallbackSink& operator=(CallbackSink&&) = default;

  void push(ObjectType&& object) { callback_(std::move(object)); }

private:
  CallbackType callback_;
};

template<typename Callback>
CallbackSink(Callback&&) -> CallbackSink<callable_argument_t<Callback>, std::remove_cvref_t<Callback>>;

}  // namespace exchange_sim
