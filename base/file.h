#pragma once

#include "logging/logging_backend.h"

#include <magic_enum/magic_enum.hpp>

#include <filesystem>
#include <fstream>

namespace exchange_sim {

class File {
public:
  enum class Mode { READ, WRITE };

  explicit File(const std::filesystem::path& path, Mode mode, bool binary = false):
      file_(path, [](Mode mode, bool binary) {
        std::ios_base::openmode flags = mode == Mode::READ ? std::ios::in : std::ios::out;
        if (binary) {
          flags |= std::ios::binary;
        }
        return flags;
      }(mode, binary)) {
    CHECK(file_.is_open(), "Could not open filename={} mode={}", path.string(), magic_enum::enum_name(mode));
  }

  size_t read(char* buffer, size_t size) {
    file_.read(buffer, size);
    size_t bytes_read = file_.gcount();
    CHECK(!file_.bad(), "Encountered error while reading from file");
    return bytes_read;
  }

  size_t write(const char* buffer, size_t size) {
    file_.write(buffer, size);
    CHECK(file_, "Encountered error while writing to file");
    return size;
  }

  bool eof() const { return file_.eof(); }

private:
  std::fstream file_;
};

}  // namespace exchange_sim
