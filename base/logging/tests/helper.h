#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>

inline std::string read_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::in);
    return (std::ostringstream() << file.rdbuf()).str();
}
