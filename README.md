# exchange-sim

## Overview

A low-latency exchange simulator in C++23.

It models the venue side of an electronic market: a matching engine that keeps an
L3 order book per instrument with FIFO price-time priority fed by a synthetic
order generator. The focus is carefully measured e2e latency and throughput.

## Requirements

- Linux (developed and benchmarked on Ubuntu 26.04 LTS)
- C++23 compiler (GCC or Clang), CMake (>= 4.2.3), Conan 2, Just

Compiler versions used for development are GCC 15.2.0 and Clang 21.1.8.

## Quickstart

    just bootstrap
    just build
    just test

    just benchmark ./configs/dmitrys-ubuntu.json --apps simple_el_app shift_cb_app

## Architecture at a glance

The lifecycle of a single order is as follows: 
- OrderGenerator creates an order.
- The request is passed to MatchingEngine via respective transport.
- MatchingEngine passes request to L3Book that maintains the respective security.
- L3Book performs basic risk checks using RiskChecker.
- L3Book updates the l3 book and outputs engine events to ExchangeDispatcher via respective transport.
- ExchangeDispatcher converts internal engine events to market data events and report events.
- ExchangeDispatcher sends them to OrderGenerator via respective transport.

The transport layer is based on the Sink concept implemented using callbacks, queues or spsc queues.

A more thorough architecture is located in [architecture.md](docs/architecture.md).

## Repository layout

    base/       Common classes and types: SPSCQueue, Journal, StrongTypes and so on.
    domain/     Core exchange types: Security, Price, Amount and so on
    exchange/   The exchange itself
    configs/    JSON run configs
    scripts/    Python scripts, mostly benchmark orchestration and result printing
    conan/      CMake conan wrapper and conan profiles

    base/config/        JSON configuration implementation with glaze
    base/logging/       Logging implementation with quill
    base/threading/     Core isolation suite
    base/time/          Tsc timestamping suite

    exchange/application/   Applications: shift_el_app, calc_stats and so on
    exchange/book_containers/   Different implementations of L3 book
    exchange/domain/        Exchange-specific types: requests, reports, engine_events and so on
    exchange/input/         Order generation classes: SimpleOrderGenerator, OrderGeneratorWrapper and so on
    exchange/matching/      Matching related classes, matching engine and l3 book implementation
    exchange/output/        External event output classes: ExchangeDispatcher

## Order generation

There are currently two order generators implemented: `SimpleOrderGenerator` and `ShiftOrderGenerator`. 
Take a look at the [order_generation.md](docs/order_generation.md) for details.

## Designs

There are six apps that implement different order generations, design and transport between components.

| Application     | Transport                              | OrderGenerator | Design                                                              |
|-----------------|----------------------------------------|----------------|---------------------------------------------------------------------|
| `simple_cb_app` | Direct callback, no queueing           | Simple         | Single-threaded                                                     |
| `simple_el_app` | Queues                                 | Simple         | Single event-loop on one thread                                     |
| `simple_mt_app` | Single-producer-single-consumer queues | Simple         | Each component on a separate thread that is pinned to separate core |
| `shift_cb_app`  | Direct callback, no queueing           | Shift          | Single-threaded                                                     |
| `shift_el_app`  | Queues                                 | Shift          | Single event-loop on one thread                                     |
| `shift_mt_app`  | Single-producer-single-consumer queues | Shift          | Each component on a separate thread that is pinned to separate core | 

Empty app is `empty_app`. It's a separate app that does nothing.

Currently `shift_mt_app` is not supported because of the current implementation of the shift design.

## What we're measuring

Latency is measured e2e, a tsc timestamp is taken once the OrderGenerator emits the request and a tsc timestamp is taken 
once it receives the market data/reports back. All requests/reports/market data events are written to respective POD Journals
along with timestamps, calc_stats then matches events by origin_time and calculates the round-trips. Currently,
first_report/first_market_event and last_report/last_market_event round trips are calculated.

- `check_isolation` verifies the actual hardware and operating system state prior to benchmarking.
- Metadata that contains current setup, git states, application sha is written, runs are archived under labels (`branch-git_revision[:6](-dirty)` by default) with full fingerprint.
- The dirty page cache is drained prior to each run, every run executes in separate systemd-run scope pinned to isolated cores.

Take a look at [benchmarking.md](docs/benchmarking.md) for more info.

## Result evaluation

`just benchmark` runs each of apps several times, calculating the latencies during each run. `just evaluate` can either evaluate a
single app or compare two apps:

- Single app evaluation measures the drift of single app across runs by percentile. It emits either `-` or `INCONSISTENT` based on drift.
- Two-way app evaluation measures the difference between latency percentile of two apps. It emits either `-`, `REGRESSED` or `IMPROVED` based on data.

Either different apps can be evaluated or one app across different labels.

## Scope

- Matching: FIFO price-time priority, deterministic per instrument
- Order operations: Add / Cancel / Replace
- L3 order book per instrument
- Synthetic order generator, market-data dispatcher, execution reports
- Benchmarking: latency and throughput

## Core isolation
    
    sudo just patch-grub ./configs/dmitrys-ubuntu.json && sudo reboot
    sudo just apply-runtime ./configs/dmitrys-ubuntu.json
    just benchmark ./configs/dmitrys-ubuntu.json --apps shift_el_app --reps 10
    just evaluate  ./configs/dmitrys-ubuntu.json --apps shift_el_app --labels <label> --percentiles 50 99

The benchmark must be run on a machine with isolated cores and performance settings applied.

## Restrictions

- Currently both order generators use only one ClientID{0}.
- Currently `shift_mt_app` is not supported because of the current implementation of the shift design.
- Although mac-clang profile and mac-clang-debug-asan preset exist, it is not guaranteed code works or even is compilable on macOS. 
It will not be tested in CI and it should be used for development purposes only.

## Future plans

- Security sharding in multi_threaded_app
