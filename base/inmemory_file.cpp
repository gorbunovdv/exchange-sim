#include "inmemory_file.h"

#include "time/rdtsc.h"

namespace exchange_sim {

InMemoryFile::InMemoryFile(const std::filesystem::path& path, Mode mode, bool binary):
    file_(path, mode == Mode::READ ? File::Mode::READ : File::Mode::WRITE, binary),
    buffer_(std::filesystem::file_size(path)) {
  CHECK(mode == Mode::READ, "Invalid file mode, mode={}", magic_enum::enum_name(mode));
  INFO("Initializing inmemory file, path={} size={}", path.string(), buffer_.size());
  Ticks start_time = Ticks::rdtsc();
  CHECK(file_.read(buffer_.data(), buffer_.size()) == buffer_.size(), "Could not read file");
  INFO("Initialized inmemory file, path={} size={} time_ns={}", path.string(), buffer_.size(), start_time.nanoseconds_till_now().count());
}

size_t InMemoryFile::read(char* buffer, size_t size) {
  size_t bytes_read = std::min(size, buffer_.size() - position_);
  memcpy(buffer, buffer_.data() + position_, bytes_read * sizeof(char));
  position_ += bytes_read;
  return bytes_read;
}

}  // namespace exchange_sim
