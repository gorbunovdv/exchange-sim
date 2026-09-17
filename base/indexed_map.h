#pragma once

#include "base/logging/logging_backend.h"

#include <ankerl/unordered_dense.h>
#include <vector>

namespace exchange_sim {

template<typename Key,
    typename Value,
    typename Hasher = std::hash<Key>,
    typename UnderlyingMap = ankerl::unordered_dense::map<Key, Value, Hasher>>
class IndexedMap : public UnderlyingMap {
public:
  void emplace(const Key& key, const Value& value) {
    CHECK(UnderlyingMap::try_emplace(key, value).second, "Key emplaced already exists");
  }

  const Value& operator[](size_t index) const { return UnderlyingMap::values()[index].second; }

private:
};

}  // namespace exchange_sim
