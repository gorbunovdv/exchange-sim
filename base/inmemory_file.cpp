#include "inmemory_file.h"

#include <magic_enum/magic_enum.hpp>

#ifdef __APPLE__
#define MAP_HUGETLB 0
#define MAP_HUGE_1GB 0
#endif

namespace exchange_sim {

InMemoryFile::InMemoryFile(const std::filesystem::path& path, Mode mode, bool binary, bool hugepages):
    file_(path, mode == Mode::READ ? File::Mode::READ : File::Mode::WRITE, binary),
    size_(std::filesystem::file_size(path)) {
  CHECK(mode == Mode::READ, "Invalid file mode, mode={}", magic_enum::enum_name(mode));
  INFO("Initializing inmemory file, path={} size={}", path.string(), size_);

  buffer_ = static_cast<char*>(mmap(nullptr,
      round_up(size_),
      PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS | (hugepages ? MAP_HUGETLB | MAP_HUGE_1GB : 0),
      -1,
      0));
  if (buffer_ == MAP_FAILED) {
    throw std::runtime_error("Could not mmap file");
  }

  CHECK(file_.read(std::span(buffer_, buffer_ + size_)) == size_, "Could not read file");
  INFO("Finished initializing inmemory file, path={} size={}", path.string(), size_);
}

size_t InMemoryFile::read(std::span<char> buffer) {
  size_t bytes_read = std::min(buffer.size_bytes(), size_ - position_);
  memcpy(buffer.data(), buffer_ + position_, bytes_read * sizeof(char));
  position_ += bytes_read;
  return bytes_read;
}

}  // namespace exchange_sim
