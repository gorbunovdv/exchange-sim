#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

TEST(TestLogWrite, TestWrite) {
    spdlog::info("Hello, world!");
}