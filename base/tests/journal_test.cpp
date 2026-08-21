#include "base/journal.h"
#include "base/temporary.h"

#include <gtest/gtest.h>

#include <array>
#include <fstream>
#include <ranges>
#include <vector>

using namespace exchange_sim;

namespace {

struct Record {
  int32_t a;
  int64_t b;

  bool operator==(const Record&) const = default;
};

using RecordJournal = Journal<Record>;

void write_records(const std::filesystem::path& path, const std::vector<RecordJournal::value_type>& records) {
  RecordJournal journal("RecordJournal", path, JournalMode::WRITE, std::nullopt, records.size() + 1);
  for (const auto& record : records) {
    journal.write(record.timestamp, record.value);
  }
}

}  // namespace

TEST(JournalTest, WriteReadTest) {
  static const std::vector<RecordJournal::value_type> records = {
      {Ticks{0}, Record{.a = 1, .b = 2}},
      {Ticks{1}, Record{.a = 2, .b = 1}},
  };

  TemporaryDirectory temp_dir("journal_test");
  TemporaryFile temp_journal_file(temp_dir, "journal_test");

  write_records(temp_journal_file.path(), records);

  RecordJournal journal("RecordJournal", temp_journal_file.path(), JournalMode::READ);
  for (const auto& record : records) {
    ASSERT_TRUE(journal.consume_if_available([&](const auto& actual) {
      EXPECT_EQ(actual.timestamp, record.timestamp);
      EXPECT_EQ(actual.value, record.value);
    }));
  }

  bool has_more_events = false;
  EXPECT_FALSE(journal.consume_if_available([&](const auto&) { has_more_events = true; }));
  EXPECT_FALSE(has_more_events);
}

TEST(JournalTest, IteratesOverAllRecords) {
  static const std::vector<RecordJournal::value_type> records = {
      {Ticks{11}, Record{.a = 1, .b = 10}},
      {Ticks{22}, Record{.a = 2, .b = 20}},
      {Ticks{33}, Record{.a = 3, .b = 30}},
  };

  TemporaryDirectory temp_dir("journal_iterator_test");
  TemporaryFile temp_journal_file(temp_dir, "journal_iterator_test");
  write_records(temp_journal_file.path(), records);

  RecordJournal journal("RecordJournal", temp_journal_file.path(), JournalMode::READ);
  for (const auto& [index, actual] : std::views::zip(std::views::iota(0), journal)) {
    EXPECT_EQ(actual.timestamp, records[index].timestamp);
    EXPECT_EQ(actual.value, records[index].value);
  }
}

TEST(JournalTest, BigEventsCount) {
  static constexpr size_t constRecordCount = 256;

  TemporaryDirectory temp_dir("journal_destructor_flush_test");
  TemporaryFile temp_journal_file(temp_dir, "journal_destructor_flush_test");

  std::vector<RecordJournal::value_type> records;
  records.reserve(constRecordCount);
  for (size_t i = 0; i < constRecordCount; ++i) {
    records.push_back({Ticks{i}, Record{.a = static_cast<int32_t>(i), .b = static_cast<int64_t>(i * 10)}});
  }

  write_records(temp_journal_file.path(), records);

  EXPECT_EQ(std::filesystem::file_size(temp_journal_file.path()), constRecordCount * RecordJournal::value_size);

  RecordJournal journal("RecordJournal", temp_journal_file.path(), JournalMode::READ);
  for (const auto& expected : records) {
    ASSERT_TRUE(journal.consume_if_available([&](const RecordJournal::value_type& actual) {
      EXPECT_EQ(actual.timestamp, expected.timestamp);
      EXPECT_EQ(actual.value, expected.value);
    }));
  }
  ASSERT_FALSE(journal.consume_if_available([](const RecordJournal::value_type&) {}));
}

TEST(JournalTest, EmptyJournal) {
  TemporaryDirectory temp_dir("empty_journal_test");
  TemporaryFile temp_journal_file(temp_dir, "empty_journal_test");

  RecordJournal journal("RecordJournal", temp_journal_file.path(), JournalMode::READ);
  EXPECT_EQ(journal.begin(), journal.end());

  bool consumer_called = false;
  EXPECT_FALSE(journal.consume_if_available([&](const auto&) { consumer_called = true; }));
  EXPECT_FALSE(consumer_called);
}

TEST(JournalTest, TornRecord) {
  TemporaryDirectory temp_dir("torn_journal_test");
  TemporaryFile temp_journal_file(temp_dir, "torn_journal_test");

  std::array<std::byte, RecordJournal::value_size - 1> torn_record{};
  {
    std::ofstream stream(temp_journal_file.path(), std::ios::out | std::ios::binary);
    stream.write(reinterpret_cast<const char*>(torn_record.data()), torn_record.size());
    ASSERT_TRUE(stream);
  }

  RecordJournal journal("RecordJournal", temp_journal_file.path(), JournalMode::READ);
  bool consumer_called = false;
  EXPECT_FALSE(journal.consume_if_available([&](const auto&) { consumer_called = true; }));
  EXPECT_FALSE(consumer_called);
}

TEST(JournalTest, MissingJournal) {
  TemporaryDirectory temp_dir("missing_journal_test");
  auto missing_path = temp_dir.path() / "missing_journal";

  EXPECT_THROW(RecordJournal("RecordJournal", missing_path, JournalMode::READ), std::runtime_error);
}
