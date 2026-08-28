#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace exchange_sim {

template<typename Underlying, typename Tag, typename... Skills>
class StrongType : public Skills... {
public:
  using UnderlyingType = Underlying;
  using TagType = Tag;

  template<typename T>
    requires(!std::same_as<std::remove_cvref_t<T>, StrongType> && std::constructible_from<Underlying, T &&>)
  explicit constexpr StrongType(T&& value) noexcept(std::is_nothrow_constructible_v<Underlying, T&&>):
      value_(std::forward<T>(value)) {}

  constexpr UnderlyingType value() const { return value_; }

private:
  UnderlyingType value_;
};

template<typename StrongType, typename ValueType, typename... T>
class StrongTypeIndexedVector : public std::vector<ValueType, T...> {
public:
  using std::vector<ValueType, T...>::vector;

  ValueType& operator[](StrongType index) { return std::vector<ValueType, T...>::operator[](index.value()); }
  const ValueType& operator[](StrongType index) const {
    return std::vector<ValueType, T...>::operator[](index.value());
  }

  ValueType& at(StrongType index) { return std::vector<ValueType, T...>::at(index.value()); }
  const ValueType& at(StrongType index) const { return std::vector<ValueType, T...>::at(index.value()); }

private:
  using std::vector<ValueType, T...>::operator[];
  using std::vector<ValueType, T...>::at;
};

namespace skills {

struct EqualTo {
  template<typename T>
  constexpr bool operator==(this const T& self, const T& other) {
    return self.value() == other.value();
  }

  template<typename T>
  constexpr bool operator!=(this const T& self, const T& other) {
    return self.value() != other.value();
  }
};

struct Ordered {
  template<typename T>
  constexpr bool operator<(this const T& self, const T& other) {
    return self.value() < other.value();
  }

  template<typename T>
  constexpr bool operator>(this const T& self, const T& other) {
    return self.value() > other.value();
  }

  template<typename T>
  constexpr bool operator<=(this const T& self, const T& other) {
    return self.value() <= other.value();
  }

  template<typename T>
  constexpr bool operator>=(this const T& self, const T& other) {
    return self.value() >= other.value();
  }
};

struct Addable {
  template<typename T>
  constexpr T operator+(this const T& self, const T& other) {
    return T{self.value() + other.value()};
  }

  template<typename T>
  constexpr T& operator+=(this T& self, const T& other) {
    self = T{self.value() + other.value()};
    return self;
  }
};

struct Subtractable {
  template<typename T>
  constexpr T operator-(this const T& self, const T& other) {
    return T{self.value() - other.value()};
  }

  template<typename T>
  constexpr T& operator-=(this T& self, const T& other) {
    self = T{self.value() - other.value()};
    return self;
  }
};

struct Modulo {
  template<typename T>
  constexpr T operator%(this const T& self, const T& other) {
    return T{self.value() % other.value()};
  }
};

struct Dividable {
  template<typename T>
  constexpr auto operator/(this const T& self, const T& other) {
    return self.value() / other.value();
  }
};

struct ScalarMultipliable {
  template<std::integral Scalar, typename T>
    requires std::derived_from<T, ScalarMultipliable>
  friend constexpr T operator*(Scalar scalar, const T& value) {
    return T{scalar * value.value()};
  }
};

struct Hashable {};

}  // namespace skills

}  // namespace exchange_sim

template<typename Underlying, typename Tag, typename... Skills>
  requires std::derived_from<exchange_sim::StrongType<Underlying, Tag, Skills...>, exchange_sim::skills::Hashable>
struct std::hash<exchange_sim::StrongType<Underlying, Tag, Skills...>> {
  size_t operator()(const exchange_sim::StrongType<Underlying, Tag, Skills...>& value) const noexcept {
    return std::hash<Underlying>()(value.value());
  }
};
