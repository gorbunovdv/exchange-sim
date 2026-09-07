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
  RecordJournal journal("RecordJournal", path, JournalMode::WRITE, records.size() + 1, std::nullopt);
  for (const auto& record : records) {
    journal.write(record);
  }
}

}  // namespace

TEST(JournalTest, WriteReadTest) {
  static const std::vector<RecordJournal::value_type> records = {
      {.object = Record{.a = 1, .b = 2}, .timestamp = Ticks{0}},
      {.object = Record{.a = 2, .b = 1}, .timestamp = Ticks{1}},
  };

  TemporaryDirectory temp_dir("journal_test");
  TemporaryFile temp_journal_file(temp_dir, "journal_test");

  write_records(temp_journal_file.path(), records);

  RecordJournal journal("RecordJournal", temp_journal_file.path(), JournalMode::READ, 2);
  for (const auto& record : records) {
    ASSERT_TRUE(journal.consume_if_available([&](const auto& actual) {
      EXPECT_EQ(actual.timestamp, record.timestamp);
      EXPECT_EQ(actual.object, record.object);
    }));
  }

  bool has_more_events = false;
  EXPECT_FALSE(journal.consume_if_available([&](const auto&) { has_more_events = true; }));
  EXPECT_FALSE(has_more_events);
}

TEST(JournalTest, IteratesOverAllRecords) {
  static const std::vector<RecordJournal::value_type> records = {
      {.object = Record{.a = 1, .b = 10}, .timestamp = Ticks{11}},
      {.object = Record{.a = 2, .b = 20}, .timestamp = Ticks{22}},
      {.object = Record{.a = 3, .b = 30}, .timestamp = Ticks{33}},
  };

  TemporaryDirectory temp_dir("journal_iterator_test");
  TemporaryFile temp_journal_file(temp_dir, "journal_iterator_test");
  write_records(temp_journal_file.path(), records);

  RecordJournal journal("RecordJournal", temp_journal_file.path(), JournalMode::READ, 2);
  for (const auto& [index, actual] : std::views::zip(std::views::iota(0), journal)) {
    EXPECT_EQ(actual.timestamp, records[index].timestamp);
    EXPECT_EQ(actual.object, records[index].object);
  }
}

TEST(JournalTest, BigEventsCount) {
  static constexpr size_t constRecordCount = 256;

  TemporaryDirectory temp_dir("journal_destructor_flush_test");
  TemporaryFile temp_journal_file(temp_dir, "journal_destructor_flush_test");

  std::vector<RecordJournal::value_type> records;
  records.reserve(constRecordCount);
  for (size_t i = 0; i < constRecordCount; ++i) {
    records.emplace_back(Record{.a = static_cast<int32_t>(i), .b = static_cast<int64_t>(i * 10)}, Ticks{i});
  }

  write_records(temp_journal_file.path(), records);

  EXPECT_EQ(std::filesystem::file_size(temp_journal_file.path()), constRecordCount * RecordJournal::value_size);

  RecordJournal journal("RecordJournal", temp_journal_file.path(), JournalMode::READ, 32);
  for (const auto& expected : records) {
    ASSERT_TRUE(journal.consume_if_available([&](const RecordJournal::value_type& actual) {
      EXPECT_EQ(actual.timestamp, expected.timestamp);
      EXPECT_EQ(actual.object, expected.object);
    }));
  }
  ASSERT_FALSE(journal.consume_if_available([](const RecordJournal::value_type&) {}));
}

TEST(JournalTest, EmptyJournal) {
  TemporaryDirectory temp_dir("empty_journal_test");
  TemporaryFile temp_journal_file(temp_dir, "empty_journal_test");

  RecordJournal journal("RecordJournal", temp_journal_file.path(), JournalMode::READ, 5);
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

  RecordJournal journal("RecordJournal", temp_journal_file.path(), JournalMode::READ, 5);
  bool consumer_called = false;
  EXPECT_FALSE(journal.consume_if_available([&](const auto&) { consumer_called = true; }));
  EXPECT_FALSE(consumer_called);
}

TEST(JournalTest, WrongMode) {
  TemporaryDirectory temp_dir("journal_mode_test");
  TemporaryFile temp_journal_file(temp_dir, "journal_mode_test");

  {
    RecordJournal journal("RecordJournal", temp_journal_file.path(), JournalMode::READ, 2);
    EXPECT_THROW(journal.write(Record{.a = 1, .b = 2}, Ticks{1}), std::runtime_error);
  }

  {
    RecordJournal journal("RecordJournal", temp_journal_file.path(), JournalMode::WRITE, 2, std::nullopt);
    EXPECT_THROW(journal.consume_if_available([](const auto&) {}), std::runtime_error);
    EXPECT_THROW(journal.begin(), std::runtime_error);
  }
}

TEST(JournalTest, MissingJournal) {
  TemporaryDirectory temp_dir("missing_journal_test");
  auto missing_path = temp_dir.path() / "missing_journal";

  EXPECT_THROW(RecordJournal("RecordJournal", missing_path, JournalMode::READ, 1), std::runtime_error);
}
