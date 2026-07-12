#include "base/journal.h"
#include "base/temporary.h"

#include <gtest/gtest.h>

using namespace exchange_sim;

TEST(JournalTest, WriteReadTest) {
  struct Struct {
    int32_t a;
    int64_t b;
  };

  TemporaryDirectory temp_dir("journal_test");
  TemporaryFile temp_journal_file(temp_dir, "journal_test");

  {
    Journal<Struct> journal("Journal<Struct>", temp_journal_file.path(), JournalMode::WRITE, std::nullopt, 10);
    journal.write(Ticks{0}, Struct{.a = 1, .b = 2});
    journal.write(Ticks{1}, Struct{.a = 2, .b = 1});
  }

  {
    Journal<Struct> journal("Journal<Struct>", temp_journal_file.path(), JournalMode::READ);
    EXPECT_TRUE(journal.consume_if_available([](auto&& s) {
      EXPECT_EQ(s.timestamp, Ticks{0});
      EXPECT_EQ(s.value.a, 1);
      EXPECT_EQ(s.value.b, 2);
    }));
    EXPECT_TRUE(journal.consume_if_available([](auto&& s) {
      EXPECT_EQ(s.timestamp, Ticks{1});
      EXPECT_EQ(s.value.a, 2);
      EXPECT_EQ(s.value.b, 1);
    }));
  }
}
