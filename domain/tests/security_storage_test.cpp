#include "domain/security_storage.h"

#include <gtest/gtest.h>

using namespace exchange_sim;

TEST(SecurityStorage, BasicSecurityStorageTest) {
  SecurityStorage storage{{
      SecurityConfig{
          .symbol = Symbol{"A"},
          .tick_size = Price(1.),
          .lot_size = Amount(100),
          .low_price = Price(10),
          .high_price = Price(1000),
          .max_amount = Amount(10000),
          .section = "SECTION0",
      },
      SecurityConfig{
          .symbol = Symbol{"B"},
          .tick_size = Price(0.1),
          .lot_size = Amount(10),
          .low_price = Price(1),
          .high_price = Price(100),
          .max_amount = Amount(10000),
          .section = "SECTION1",
      },
  }};
  EXPECT_EQ(storage.size(), 2);
  EXPECT_EQ(storage.get_id_by_symbol_or_null(Symbol{"A"}), SecurityID{0});
  EXPECT_EQ(storage.get_id_by_symbol_or_null(Symbol{"B"}), SecurityID{1});
  EXPECT_EQ(storage.get_id_by_symbol_or_null(Symbol{"C"}), std::nullopt);
  EXPECT_TRUE(storage.has_security(SecurityID{0}));
  EXPECT_TRUE(storage.has_security(SecurityID{1}));
  EXPECT_FALSE(storage.has_security(SecurityID{2}));
  EXPECT_EQ(storage.get_security(SecurityID{0}).symbol, Symbol{"A"});
  EXPECT_EQ(storage.get_security(SecurityID{1}).symbol, Symbol{"B"});
}

TEST(SecurityStorage, EmptySecurityTest) {
  EXPECT_THROW(SecurityStorage{{}}, std::runtime_error);
}

TEST(SecurityStorage, DuplicateSymbolsTest) {
  EXPECT_THROW(SecurityStorage({
                   SecurityConfig{
                       .symbol = Symbol{"A"},
                       .tick_size = Price(1.),
                       .lot_size = Amount(100),
                       .low_price = Price(10),
                       .high_price = Price(1000),
                       .max_amount = Amount(10000),
                       .section = "SECTION0",
                   },
                   SecurityConfig{.symbol = Symbol{"A"},
                       .tick_size = Price(0.1),
                       .lot_size = Amount(10),
                       .low_price = Price(1),
                       .high_price = Price(100),
                       .max_amount = Amount(10000),
                       .section = "SECTION1"},
               }),
      std::runtime_error);
}