#pragma once

#include "base/noncopyable.h"

#include <filesystem>
#include <format>
#include <unistd.h>

namespace exchange_sim {

class TemporaryDirectory : NonCopyable, NonMoveable {
public:
  explicit TemporaryDirectory(const std::string& prefix) {
    auto path_string = (std::filesystem::temp_directory_path() / std::format("{}_{}", prefix, "XXXXXX")).string();
    if (::mkdtemp(path_string.data()) == nullptr) {
      throw std::runtime_error("failed to create temporary directory");
    }
    path_ = std::move(path_string);
  }

  ~TemporaryDirectory() noexcept {
    std::error_code ec;
    std::filesystem::remove_all(path_, ec);
  }

  const std::filesystem::path& path() const { return path_; }

private:
  std::filesystem::path path_;
};

class TemporaryFile : NonCopyable, NonMoveable {
public:
  explicit TemporaryFile(const TemporaryDirectory& dir, const std::string& prefix) {
    auto path_string = (dir.path() / std::format("{}_XXXXXX", prefix)).string();
    if (int fd = mkstemp(path_string.data()); fd != -1) {
      ::close(fd);
    } else {
      throw std::runtime_error("failed to create temporary file");
    }
    path_ = std::move(path_string);
  }

  ~TemporaryFile() noexcept {
    std::error_code ec;
    std::filesystem::remove(path_, ec);
  }

  const std::filesystem::path& path() const { return path_; }

private:
  std::filesystem::path path_;
};

}  // namespace exchange_sim