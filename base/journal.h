#pragma once

#include "base/logging/loggable_class.h"
#include "base/logging/logging_backend.h"
#include "base/noncopyable.h"
#include "base/throughput.h"
#include "base/time/rdtsc.h"
#include "base/time/timestamped.h"

#include <boost/stl_interfaces/iterator_interface.hpp>
#include <magic_enum/magic_enum.hpp>

#include <array>
#include <filesystem>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>

namespace exchange_sim {

template<typename JournalType,
    typename BaseType =
        boost::stl_interfaces::proxy_iterator_interface<std::input_iterator_tag, typename JournalType::value_type>>
class JournalIterator : public BaseType {
public:
  explicit JournalIterator(JournalType* journal = nullptr): journal_(journal) {
    BOOST_STL_INTERFACES_STATIC_ASSERT_CONCEPT(JournalIterator, std::input_iterator);
    advance();
  }

  JournalIterator& operator++() {
    advance();
    return *this;
  }

  typename JournalType::value_type operator*() const {
    using ::exchange_sim::get_logger;

    CHECK(current_value_.has_value(), "Iterator is not dereferenceable");
    return *current_value_;
  }

  const typename JournalType::value_type* operator->() const {
    using ::exchange_sim::get_logger;

    CHECK(current_value_.has_value(), "Iterator is not dereferenceable");
    return &(*current_value_);
  }

  bool operator==(const JournalIterator& iterator) const {
    return (journal_ == nullptr || iterator.journal_ == nullptr) && (is_eof_ || iterator.is_eof_);
  }

  using BaseType::operator++;

private:
  void advance() {
    if (!is_eof_ && journal_) {
      is_eof_ =
          !journal_->consume([this](const typename JournalType::value_type& value) { current_value_.emplace(value); });
    }
  }

  JournalType* journal_;
  std::optional<typename JournalType::value_type> current_value_;
  bool is_eof_ = false;
};

template<typename FileType, typename POD>
  requires(std::is_trivially_copyable_v<POD> && !std::is_pointer_v<POD>)
class Journal : NonMoveable, public LoggableClass {
public:
  using value_type = Timestamped<POD>;
  using iterator = JournalIterator<Journal>;

  static constexpr size_t value_size = sizeof(value_type);

  enum class Mode : std::uint8_t { READ, WRITE };

  explicit Journal(const std::string& name, const std::filesystem::path& journal_path, Mode mode):
      LoggableClass(name),
      name_(name),
      stream_(journal_path, mode == Mode::WRITE ? FileType::Mode::WRITE : FileType::Mode::READ, true),
      mode_(mode),
      throughput_(name) {
    CHECK(mode_ == Mode::READ || mode_ == Mode::WRITE, "Invalid journal mode: {}", magic_enum::enum_name(mode_));
  }

  void write(const POD& value, Ticks timestamp) { write(value_type{value, timestamp}); }

  void write(const value_type& structure) { write(std::span(&structure, 1)); }

  void write(std::span<const value_type> data) {
    CHECK(mode_ == Mode::WRITE, "Mode should be write");
    if (!data.empty()) {
      stream_.write(reinterpret_cast<const char*>(data.data()), data.size_bytes());
      throughput_.tick(data.size());
    }
  }

  size_t read(std::span<value_type> data) {
    CHECK(mode_ == Mode::READ, "Mode should be read");
    std::streamsize max_bytes_read = data.size_bytes(), bytes_read = 0;
    while (bytes_read < max_bytes_read) {
      std::streamsize current_bytes_read =
          stream_.read(reinterpret_cast<char*>(data.data()) + bytes_read, max_bytes_read - bytes_read);
      bytes_read += current_bytes_read;
      if (current_bytes_read == 0) {
        break;
      }
    }
    if (bytes_read % sizeof(value_type) != 0) {
      ERROR("Got torn record in journal: read_bytes={} record_size={}", bytes_read, sizeof(value_type));
    }
    return bytes_read / sizeof(value_type);
  }

  template<typename Consumer>
  bool consume(Consumer&& consumer) {
    alignas(value_type) std::array<std::byte, sizeof(value_type)> data;
    auto* value = reinterpret_cast<value_type*>(data.data());
    if (read(std::span(value, 1)) != 0) {
      std::forward<Consumer>(consumer)(*value);
      return true;
    }
    return false;
  }

  auto begin() {
    CHECK(mode_ == Mode::READ, "Mode should be read");
    return iterator(this);
  }

  auto end() { return iterator(); }

  bool ready() const { return true; }

  bool eof() const { return stream_.eof(); }

private:
  std::string name_;
  FileType stream_;
  Mode mode_;
  Throughput throughput_;
};

}  // namespace exchange_sim
