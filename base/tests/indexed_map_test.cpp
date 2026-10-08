#include "base/indexed_map.h"

#include <gtest/gtest.h>

#include <string>

using namespace exchange_sim;

namespace {

void fill_map(IndexedMap<std::string, int>& map) {
  map.emplace("first", 10);
  map.emplace("second", 20);
  map.emplace("third", 30);
}

void expect_element(IndexedMap<std::string, int>& map, const std::string& key, int value) {
  ASSERT_TRUE(map.contains(key));
  auto iterator = map.find(key);
  ASSERT_NE(iterator, map.end());
  ASSERT_EQ(iterator->first, key);
  ASSERT_EQ(iterator->second, value);
}

void expect_missing_element(IndexedMap<std::string, int>& map, const std::string& key) {
  ASSERT_FALSE(map.contains(key));
  ASSERT_EQ(map.find(key), map.end());
}

}  // namespace

TEST(IndexedMapTest, EmptyMap) {
  IndexedMap<std::string, int> map;

  ASSERT_TRUE(map.empty());
  ASSERT_EQ(map.size(), 0);
  ASSERT_EQ(map.begin(), map.end());
  expect_missing_element(map, "missing");
}

TEST(IndexedMapTest, Emplace) {
  IndexedMap<std::string, int> map;
  map.emplace("first", 10);
  auto [second, inserted] = map.try_emplace("second", 20);

  ASSERT_TRUE(inserted);
  ASSERT_FALSE(map.empty());
  ASSERT_EQ(map.size(), 2);
  ASSERT_EQ(second->first, "second");
  ASSERT_EQ(second->second, 20);
  expect_element(map, "first", 10);
  expect_element(map, "second", 20);
}

TEST(IndexedMapTest, EmplaceExisting) {
  IndexedMap<std::string, int> map;
  fill_map(map);

  auto result = map.try_emplace("second", 200);

  ASSERT_FALSE(result.second);
  ASSERT_EQ(map.size(), 3);
  expect_element(map, "second", 20);

  ASSERT_THROW(map.emplace("second", 200), std::runtime_error);
  ASSERT_EQ(map.size(), 3);
  expect_element(map, "second", 20);
}

TEST(IndexedMapTest, EraseByKey) {
  IndexedMap<std::string, int> map;
  fill_map(map);
  map.emplace("fourth", 40);

  map.erase("second");

  ASSERT_EQ(map.size(), 3);
  expect_missing_element(map, "second");
  expect_element(map, "first", 10);
  expect_element(map, "third", 30);
  expect_element(map, "fourth", 40);
}

TEST(IndexedMapTest, EraseByIterator) {
  IndexedMap<std::string, int> map;
  fill_map(map);

  auto third = map.find("third");
  ASSERT_NE(third, map.end());
  map.erase(third);

  ASSERT_EQ(map.size(), 2);
  expect_missing_element(map, "third");
  expect_element(map, "first", 10);
  expect_element(map, "second", 20);

  auto first = map.find("first");
  ASSERT_NE(first, map.end());
  map.erase(first);

  ASSERT_EQ(map.size(), 1);
  expect_missing_element(map, "first");
  expect_element(map, "second", 20);
}

TEST(IndexedMapTest, EraseFirstElement) {
  IndexedMap<std::string, int> map;
  fill_map(map);
  map.emplace("fourth", 40);

  auto first = map.find("first");
  ASSERT_NE(first, map.end());
  map.erase(first);

  ASSERT_EQ(map.size(), 3);
  expect_missing_element(map, "first");
  expect_element(map, "second", 20);
  expect_element(map, "third", 30);
  expect_element(map, "fourth", 40);
}

TEST(IndexedMapTest, EraseMiddleElement) {
  IndexedMap<std::string, int> map;
  fill_map(map);
  map.emplace("fourth", 40);

  auto second = map.find("second");
  ASSERT_NE(second, map.end());
  map.erase(second);

  ASSERT_EQ(map.size(), 3);
  expect_missing_element(map, "second");
  expect_element(map, "first", 10);
  expect_element(map, "third", 30);
  expect_element(map, "fourth", 40);
}

TEST(IndexedMapTest, ErasePrelastElement) {
  IndexedMap<std::string, int> map;
  fill_map(map);
  map.emplace("fourth", 40);

  auto third = map.find("third");
  ASSERT_NE(third, map.end());
  map.erase(third);

  ASSERT_EQ(map.size(), 3);
  expect_missing_element(map, "third");
  expect_element(map, "first", 10);
  expect_element(map, "second", 20);
  expect_element(map, "fourth", 40);
}

TEST(IndexedMapTest, EraseLastElement) {
  IndexedMap<std::string, int> map;
  fill_map(map);
  map.emplace("fourth", 40);

  auto fourth = map.find("fourth");
  ASSERT_NE(fourth, map.end());
  map.erase(fourth);

  ASSERT_EQ(map.size(), 3);
  expect_missing_element(map, "fourth");
  expect_element(map, "first", 10);
  expect_element(map, "second", 20);
  expect_element(map, "third", 30);
}

TEST(IndexedMapTest, IndexesAllElements) {
  IndexedMap<std::string, int> map;
  fill_map(map);
  map.emplace("fourth", 40);

  std::set<int> actual;
  for (size_t i = 0; i < map.size(); i++) {
    actual.insert(map[i]);
  }
  std::set expected = {10, 20, 30, 40};
  EXPECT_EQ(actual, expected);
}
