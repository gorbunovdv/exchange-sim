#include "generate_securities.h"

namespace exchange_sim {

std::vector<SecurityConfig> generate_securities(const glz::generic& config) {
  std::vector<SecurityConfig> securities;
  securities.reserve(config["securities"].get_array().size());
  for (auto&& security : config["securities"].get_array()) {
    securities.push_back({
        .symbol = Symbol(security["symbol"].as<std::string>()),
        .tick_size = Price(security["tick_size"].as<double>()),
        .lot_size = Amount(security["lot_size"].as<double>()),
        .low_price = Price(security["low_price"].as<double>()),
        .high_price = Price(security["high_price"].as<double>()),
        .max_amount = Amount(security["max_amount"].as<double>()),
        .section = security["section"].as<std::string>(),
    });
  }
  return securities;
}

}  // namespace exchange_sim