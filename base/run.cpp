#include "run.h"

#include "base/config/config.h"
#include "base/logging/logging_backend.h"
#include "base/threading/core_allocator.h"
#include "base/time/rdtsc.h"

#include <magic_enum/magic_enum.hpp>

#include <iostream>

namespace exchange_sim {

std::tuple<CoreLease, std::shared_ptr<CoreAllocator>, glz::generic> read_config_and_initialize_logging(
    std::span<char*> args) {
  auto value_or_raise = [](std::string_view type, auto expected) {
    if (!expected.has_value()) {
      throw std::runtime_error(std::format("{} has no correct value", type));
    }
    return expected.value();
  };

  if (args.size() != 2) {
    std::cout << "usage: " << args[0] << " <config_file>" << '\n';
    throw std::runtime_error("wrong number of arguments");
  }
  glz::generic config = load_config_from_file(args[1]);
  CoreAllocator::Config core_config{
      .shared_core = config["cpu_cores"]["shared_core"].as<uint32_t>(),
      .dedicated_cores = std::vector{std::from_range,
          config["cpu_cores"]["dedicated_cores"].get_array() |
              std::views::transform([](auto&& v) { return v.template as<uint32_t>(); })},
  };
  auto core_manager = std::make_shared<CoreAllocator>(core_config);
  auto core_lease = core_manager->lease_core(CoreType::SHARED);
  core_lease.pin_thread("main");
  LoggingBackend::LoggingConfig logging_config{
      .type = value_or_raise("logging.type",
          magic_enum::enum_cast<LoggingBackend::LoggingConfig::Type>(
              config["logging"]["type"].as<std::string_view>(), magic_enum::case_insensitive)),
      .path = config["logging"].contains("path") ? std::optional{config["logging"]["path"].as<std::string>()} :
                                                   std::nullopt,
      .log_level = value_or_raise("logging.log_level",
          magic_enum::enum_cast<quill::LogLevel>(
              config["logging"]["log_level"].as<std::string>(), magic_enum::case_insensitive)),
      .format = quill::PatternFormatterOptions(config["logging"]["format"].as<std::string>()),
      .cpu_id = core_manager->lease_core(CoreType::SHARED).cpu_id(),
  };
  LoggingBackend::init(logging_config);
  INFO("Initialized logging backend");
  INFO("config={}", glz::write_json(config).value());
  INFO("Initialized time backend, tsc_hw: {}", rdtsc_frequency());
  return std::tuple{std::move(core_lease), std::move(core_manager), config};
}

}  // namespace exchange_sim
