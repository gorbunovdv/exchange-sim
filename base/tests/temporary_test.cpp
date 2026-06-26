#include "base/temporary.h"
#include <gtest/gtest.h>

TEST(TemporaryTest, BasicTemporaryTest) {
  using namespace exchange_sim;

  std::string file_path, directory_path;
  {
    TemporaryDirectory directory("test");
    TemporaryFile file(directory, "test");

    std::tie(directory_path, file_path) = std::make_tuple(directory.path().string(), file.path().string());

    EXPECT_TRUE(std::filesystem::exists(directory.path()));
    EXPECT_TRUE(std::filesystem::exists(file.path()));
  }

  EXPECT_FALSE(std::filesystem::exists(directory_path));
  EXPECT_FALSE(std::filesystem::exists(file_path));
}