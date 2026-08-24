#pragma once
#include <type_traits>

namespace exchange_sim {

template<typename>
struct object_method_argument;

template<typename R, typename C, typename Arg>
struct object_method_argument<R (C::*)(Arg)> {
  using type = Arg;
};

template<typename R, typename C, typename Arg>
struct object_method_argument<R (C::*)(Arg) const> {
  using type = Arg;
};

template<typename R, typename C, typename Arg>
struct object_method_argument<R (C::*)(Arg) noexcept> {
  using type = Arg;
};

template<typename R, typename C, typename Arg>
struct object_method_argument<R (C::*)(Arg) const noexcept> {
  using type = Arg;
};

template<typename T>
using object_method_argument_t = object_method_argument<T>::type;

template<typename Callable>
using callable_argument_t = object_method_argument_t<decltype(&std::remove_cvref_t<Callable>::operator())>;

}  // namespace exchange_sim