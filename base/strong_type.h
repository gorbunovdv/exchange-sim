#pragma once

#include <vector>

namespace exchange_sim {

template<typename Underlying, typename Tag, typename... Skills>
class StrongType : public Skills... {
public:
  using UnderlyingType = Underlying;
  using TagType = Tag;

  explicit StrongType(UnderlyingType value): value_(value) {}

  UnderlyingType value() const { return value_; }

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
  bool operator==(this const T& self, const T& other) {
    return self.value() == other.value();
  }

  template<typename T>
  bool operator!=(this const T& self, const T& other) {
    return self.value() != other.value();
  }
};

}  // namespace skills

}  // namespace exchange_sim