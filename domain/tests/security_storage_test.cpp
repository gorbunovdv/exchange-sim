#include "domain/security_storage.h"

#include <gtest/gtest.h>

using namespace exchange_sim;

TEST(SecurityStorage, BasicSecurityStorageTest) {
  SecurityStorage storage{{
      SecurityConfig{
          .symbol = "A",
          .tick_size = Price(1.),
          .lot_size = Amount(100),
          .low_price = Price(10),
          .high_price = Price(1000),
          .max_amount = Amount(10000),
      },
      SecurityConfig{
          .symbol = "B",
          .tick_size = Price(0.1),
          .lot_size = Amount(10),
          .low_price = Price(1),
          .high_price = Price(100),
          .max_amount = Amount(10000),
      },
  }};
  EXPECT_EQ(storage.size(), 2);
  EXPECT_EQ(storage.get_id_by_symbol_or_null("A"), 0);
  EXPECT_EQ(storage.get_id_by_symbol_or_null("B"), 1);
  EXPECT_EQ(storage.get_id_by_symbol_or_null("C"), std::nullopt);
  EXPECT_TRUE(storage.has_security(0));
  EXPECT_TRUE(storage.has_security(1));
  EXPECT_FALSE(storage.has_security(2));
  EXPECT_EQ(storage.get_security(0).symbol, "A");
  EXPECT_EQ(storage.get_security(1).symbol, "B");
}

TEST(SecurityStorage, EmptySecurityTest) {
  EXPECT_THROW(SecurityStorage{{}}, std::runtime_error);
}

TEST(SecurityStorage, DuplicateSymbolsTest) {
  EXPECT_THROW(SecurityStorage({
                   SecurityConfig{
                       .symbol = "A",
                       .tick_size = Price(1.),
                       .lot_size = Amount(100),
                       .low_price = Price(10),
                       .high_price = Price(1000),
                       .max_amount = Amount(10000),
                   },
                   SecurityConfig{
                       .symbol = "A",
                       .tick_size = Price(0.1),
                       .lot_size = Amount(10),
                       .low_price = Price(1),
                       .high_price = Price(100),
                       .max_amount = Amount(10000),
                   },
               }),
      std::runtime_error);
}