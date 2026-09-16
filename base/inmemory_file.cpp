#include "inmemory_file.h"

namespace exchange_sim {

InMemoryFile::InMemoryFile(const std::filesystem::path& path, Mode mode, bool binary):
    file_(path, mode == Mode::READ ? File::Mode::READ : File::Mode::WRITE, binary),
    buffer_(std::filesystem::file_size(path)) {
  CHECK(mode == Mode::READ, "Invalid file mode, mode={}", magic_enum::enum_name(mode));
  INFO("Initializing inmemory file, path={} size={}", path.string(), buffer_.size());
  CHECK(file_.read(std::span(buffer_)) == buffer_.size(), "Could not read file");
  INFO("Finished initializing inmemory file, path={} size={}", path.string(), buffer_.size());
}

size_t InMemoryFile::read(std::span<char> buffer) {
  size_t bytes_read = std::min(buffer.size_bytes(), buffer_.size() - position_);
  memcpy(buffer.data(), buffer_.data() + position_, bytes_read * sizeof(char));
  position_ += bytes_read;
  return bytes_read;
}

}  // namespace exchange_sim
