#pragma once

#include <boost/stl_interfaces/iterator_interface.hpp>
#include <boost/stl_interfaces/sequence_container_interface.hpp>

#include <unordered_map>
#include <vector>

namespace exchange_sim {

template<typename Key, typename Value, typename Hasher = std::hash<Key>>
class IndexedMap : public boost::stl_interfaces::sequence_container_interface<IndexedMap<Key, Value, Hasher>> {
public:
  using Base = boost::stl_interfaces::sequence_container_interface<IndexedMap>;

  using value_type = std::pair<Key, Value>;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;

  class iterator : public boost::stl_interfaces::iterator_interface<std::random_access_iterator_tag, value_type> {
  public:
    iterator() {
      BOOST_STL_INTERFACES_STATIC_ASSERT_CONCEPT(iterator, std::random_access_iterator);
      BOOST_STL_INTERFACES_STATIC_ASSERT_ITERATOR_TRAITS(iterator,
          std::random_access_iterator_tag,
          std::random_access_iterator,
          value_type,
          reference,
          pointer,
          difference_type);
    }

    explicit iterator(const IndexedMap* container, size_t index): container_(container), index_(index) {}

    value_type& operator*() { return container_->key_value_pairs_[index_]; }
    const value_type& operator*() const { return container_->key_value_pairs_[index_]; }

    iterator& operator+=(difference_type n) {
      index_ += n;
      return *this;
    }

    friend difference_type operator-(const iterator& a, const iterator& b) { return a.index_ - b.index_; }

    friend class IndexedMap;

  private:
    const IndexedMap* container_ = nullptr;
    size_t index_ = 0;
  };

  using const_iterator = iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = reverse_iterator;

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

  void erase(const Key& key) { erase_at(map_.at(key)); }
  void erase(iterator iterator) { erase_at(iterator.index_); }

  iterator begin() const { return iterator(this, 0); }
  iterator end() const { return iterator(this, size()); }

  Value& operator[](size_t index) { return key_value_pairs_[index].second; }
  const Value& operator[](size_t index) const { return key_value_pairs_[index].second; }

private:
  void erase_at(size_t index) {
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