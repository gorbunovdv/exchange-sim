# exchange-sim

A low-latency exchange simulator in C++23.

It models the venue side of electronic market: a matching engine that keeps an
L3 order book per instrument with FIFO price-time priority fed by a synthetic
order generator with a market-data dispatcher and a report generator. The
focus is carefully measured latency and throughput under realistic load.

## Status

Early work in progress. Current milestone (MVP): single-threaded matching engine 
+ synthetic order generator + market-data dispatcher + report generator.

## Scope

- Matching: FIFO price-time priority, deterministic per instrument
- Order operations: Add / Cancel / Replace
- L3 order book per instrument
- Synthetic order generator, market-data dispatcher, execution reports
- Benchmarking: latency and throughput

## Requirements

- Linux (developed on Ubuntu 26.04 LTS)
- C++23 compiler (GCC or Clang)
- CMake and Conan 2

## Bootstrap
    
    just bootstrap

## Build

    just build

## Benchmark

Use the release preset and a config:

    just benchmark single_threaded_callback_app configs/dmitrys-ubuntu.json

The recipe rebuilds all the code from scratch, runs the config patch step, starts the
application for up to 60 seconds via `systemd-run` using the CPU settings from
the config, then runs both statistics calculators. Journal files and calculated
latencies of each request are written to the paths from the config, by default under
`./results/`. Script prints the results to the console.

To use another CMake preset pass it as the third argument:

    just benchmark single_threaded_callback_app configs/default.json clang-release

For stable measurements run this on Linux after applying CPU isolation and
performance settings:

    sudo just apply-core-isol-and-perf

The benchmark must be run on a machine with isolated cores and performance settings applied.

## Restrictions

- Although mac-clang profile and mac-clang-debug-asan preset exist, it is not guaranteed code works or even is compilable on macOS. It will not be tested in CI and it should be used for development purposes only.
