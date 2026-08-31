#pragma once

#include <magic_enum/magic_enum.hpp>

#include <random>

namespace exchange_sim {

class Random {
public:
  explicit Random(std::uint32_t seed): generator_{seed} {}

  template<typename Strong>
  Strong rand_int_strong_type(Strong::UnderlyingType from, Strong::UnderlyingType to) {
    return Strong{rand_int(from, to)};
  }

  template<typename Int>
  Int rand_int(Int from, Int to) {
    CHECK(from <= to, "'from' must be less than or equal to 'to', from={} to={}", from, to);
    return std::uniform_int_distribution<Int>{from, to}(generator_);
  }

  template<typename Enum>
    requires(std::is_enum_v<Enum> && magic_enum::enum_count<Enum>() > 0)
  Enum rand_enum() {
    static constexpr auto values = magic_enum::enum_values<Enum>();
    static_assert(!values.empty(), "'values' must not be empty");

    return values[rand_int<size_t>(0, magic_enum::enum_count<Enum>() - 1)];
  }

  template<typename Decimal>
  Decimal rand_decimal(Decimal from, Decimal to, Decimal step) {
    auto interval_steps = static_cast<int64_t>((to - from) / step);
    return from + rand_int<int64_t>(0, interval_steps) * step;
  }

private:
  std::mt19937 generator_;
};

}  // namespace exchange_sim