#include "file.h"

#include <magic_enum/magic_enum.hpp>

namespace exchange_sim {

File::File(const std::filesystem::path& path, Mode mode, bool binary):
    file_(path, [](Mode mode, bool binary) {
      std::ios_base::openmode flags = mode == Mode::READ ? std::ios::in : std::ios::out;
      if (binary) {
        flags |= std::ios::binary;
      }
      return flags;
    }(mode, binary)) {
  CHECK(file_.is_open(), "Could not open filename={} mode={}", path.string(), magic_enum::enum_name(mode));
}

size_t File::read(std::span<char> buffer) {
  file_.read(buffer.data(), static_cast<std::streamsize>(buffer.size_bytes()));
  size_t bytes_read = file_.gcount();
  CHECK(!file_.bad(), "Encountered error while reading from file");
  return bytes_read;
}

size_t File::write(std::span<const char> buffer) {
  file_.write(buffer.data(), static_cast<std::streamsize>(buffer.size_bytes()));
  CHECK(file_, "Encountered error while writing to file");
  return buffer.size_bytes();
}

}  // namespace exchange_sim
