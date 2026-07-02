#pragma once

#include <boost/decimal/decimal32_t.hpp>
#include <boost/decimal/decimal64_t.hpp>
#include <magic_enum/magic_enum.hpp>

namespace exchange_sim {

using SecurityID = uint32_t;
using Price = boost::decimal::decimal64_t;
using Amount = boost::decimal::decimal32_t;

enum class OrderType : std::uint8_t { LIMIT };
enum class OrderTimeInForce : std::uint8_t { LIMIT, IOC };
enum class Direction : std::uint8_t { BUY, SELL };

constexpr std::string_view format_as(OrderType type) {
  return magic_enum::enum_name(type);
}

constexpr std::string_view format_as(OrderTimeInForce time_in_force) {
  return magic_enum::enum_name(time_in_force);
}

constexpr std::string_view format_as(Direction direction) {
  return magic_enum::enum_name(direction);
}

}  // namespace exchange_sim