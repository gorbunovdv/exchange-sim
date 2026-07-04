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

## Restrictions

- Although mac-clang profile and mac-clang-debug-asan preset exist, it is not guaranteed code works or even is compilable on macOS. It will not be tested in CI and it should be used for development purposes only.