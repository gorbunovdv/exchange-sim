#pragma once

#include "base/file.h"

#include <filesystem>

namespace exchange_sim {

class InMemoryFile : NonCopyable {
public:
  enum class Mode : uint8_t { READ, WRITE };

  explicit InMemoryFile(const std::filesystem::path& path, Mode mode, bool binary, bool hugepages);

  ~InMemoryFile() { munmap(buffer_, round_up(size_)); }

  size_t read(std::span<char> buffer);

  bool eof() const { return position_ == size_; }

private:
  static constexpr size_t round_up(size_t size) { return ((size + (1 << 30) - 1) >> 30) << 30; }

  File file_;
  size_t size_;
  char* buffer_;
  size_t position_ = 0;
};

}  // namespace exchange_sim
