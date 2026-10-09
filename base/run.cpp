#include "run.h"

#include "base/config/config.h"
#include "base/logging/logging_backend.h"
#include "base/threading/core_allocator.h"
#include "base/threading/runtime.h"
#include "base/time/rdtsc.h"

#include <magic_enum/magic_enum.hpp>

#include <iostream>
#include <mimalloc.h>

#if !__has_feature(address_sanitizer) && !defined(__SANITIZE_ADDRESS__) && !__has_feature(thread_sanitizer) && \
    !defined(__SANITIZE_THREAD__)
#include <mimalloc-new-delete.h>
#endif

namespace exchange_sim {

std::tuple<std::shared_ptr<Runtime>, glz::generic> read_config_and_initialize_logging(
    std::span<char*> args, bool secondary_app) {
  mi_option_set(mi_option_purge_delay, -1);
  block_shutdown_signals();
#if !defined(__APPLE__) && !__has_feature(address_sanitizer) && !defined(__SANITIZE_ADDRESS__) && \
    !__has_feature(thread_sanitizer) && !defined(__SANITIZE_THREAD__)
  if (int ec = mlockall(MCL_CURRENT | MCL_FUTURE); ec != 0) {
    throw std::system_error(errno, std::generic_category(), "mlockall failed");
  }
#endif

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
  if (!secondary_app) {
    auto expected_huge_pages = config.contains("memory") && config["memory"]["reserve_huge_os_pages"].get_boolean() ?
        config["memory"]["huge_os_pages_count"].as<size_t>() :
        0;
    size_t requested_huge_pages = mi_option_get(mi_option_reserve_huge_os_pages);
    CHECK(expected_huge_pages == requested_huge_pages,
        "Memory setup mismatch, allocated_huge_pages={} expected={}",
        requested_huge_pages,
        expected_huge_pages);
  }
  bool fallback_to_none = config.contains("fallback_to_none") && config["fallback_to_none"].get_boolean();
  Runtime::Config rt_config{
      .core_allocator_config = {
          .fallback_to_none = fallback_to_none,
          .shared_core = !fallback_to_none ? std::optional{CpuID{config["cpu_cores"]["shared_core"].as<uint32_t>()}} :
                                             std::nullopt,
          .dedicated_cores = !fallback_to_none ?
              std::vector{std::from_range,
                  config["cpu_cores"]["dedicated_cores"].get_array() |
                      std::views::transform([](auto&& v) { return CpuID{v.template as<uint32_t>()}; })} :
              std::vector<CpuID>{},
      }};
  auto runtime = std::make_shared<Runtime>(rt_config);
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
      .cpu_id = !fallback_to_none ? std::optional{runtime->core_allocator().lease_core(CoreType::SHARED).cpu_id()} :
                                    std::nullopt,
  };
  LoggingBackend::init(logging_config);
  INFO("Initialized logging backend");
  runtime->core_allocator().lease_core(CoreType::SHARED).pin_thread("main");
  INFO("config={}", glz::write_json(config).value());
  INFO("Initialized time backend, tsc_hw: {}", rdtsc_frequency());
  return std::tuple{std::move(runtime), config};
}

}  // namespace exchange_sim
