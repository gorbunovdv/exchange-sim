#pragma once

#include <limits>
#include <unordered_map>
#include <vector>

namespace exchange_sim {

template<typename Key, typename Value, typename Hasher = std::hash<Key>>
class IndexedMap {
public:
  using value_type = std::pair<Key, Value>;

  class iterator {
  public:
    explicit iterator(IndexedMap* container, size_t index): container_(container), index_(index) {}

    value_type& operator*() const { return container_->key_value_pairs_[index_]; }

    value_type* operator->() { return &(container_->key_value_pairs_[index_]); }

    friend bool operator==(const iterator& a, const iterator& b) {
      return a.container_ == b.container_ && a.index_ == b.index_;
    }

    friend class IndexedMap;

  private:
    IndexedMap* container_;
    size_t index_;
  };

  bool empty() const { return size() == 0; }

  size_t size() const { return key_value_pairs_.size(); }

  iterator find(const Key& key) {
    if (auto it = map_.find(key); it != map_.end()) {
      return iterator(this, it->second);
    }
    return end();
  }

  bool contains(const Key& key) const { return map_.contains(key); }

  void emplace(const Key& key, const Value& value) {
    key_value_pairs_.push_back({key, value});
    map_[key] = key_value_pairs_.size() - 1;
  }

  void erase(const Key& key) { erase(map_.at(key)); }

  void erase(iterator iterator) { erase(iterator.index_); }

  const Value& front() const { return key_value_pairs_.front().second; }

  const Value& operator[](size_t index) const { return key_value_pairs_[index].second; }

  auto begin() { return iterator(this, 0); }

  auto end() { return iterator(this, std::numeric_limits<size_t>::max()); }

private:
  void erase(size_t index) {
    map_.erase(key_value_pairs_[index].first);
    if (index < key_value_pairs_.size()) {
      std::swap(key_value_pairs_[index], key_value_pairs_.back());
      map_[key_value_pairs_[index].first] = index;
    }
    key_value_pairs_.pop_back();
  }

  std::unordered_map<Key, size_t, Hasher> map_;
  std::vector<value_type> key_value_pairs_;
};

}  // namespace exchange_sim