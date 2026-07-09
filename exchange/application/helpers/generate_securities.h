#pragma once

namespace exchange_sim {

std::vector<SecurityConfig> generate_securities(const glz::generic& config) {
  return {std::from_range, config["securities"].get_array() | std::views::transform([](auto&& security) {
            return SecurityConfig{
                .symbol = Symbol(security["symbol"].template as<std::string>()),
                .tick_size = Price(security["tick_size"].template as<double>()),
                .lot_size = Amount(security["lot_size"].template as<double>()),
                .low_price = Price(security["low_price"].template as<double>()),
                .high_price = Price(security["high_price"].template as<double>()),
                .max_amount = Amount(security["max_amount"].template as<double>()),
            };
          })};
}

}  // namespace exchange_sim