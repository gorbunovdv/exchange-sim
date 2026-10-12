# exchange-sim

## Overview

A low-latency exchange simulator in C++23.

It models the venue side of an electronic market: a matching engine that keeps an L3 order book per instrument with 
FIFO price-time priority fed by a synthetic order generator. All requests, reports and market data events are timestamped
with TSC and written to a journal. The focus is carefully measured in-process round trip latency for single threaded apps 
and throughput for multi threaded apps.

Order generator, matching engine and exchange dispatcher are wired through C++ concepts and run unchanged in different 
transport designs: direct callbacks, a single threaded event loop and connected by lock-free SPSC queues pinned threads for 
each stage.

## Results

The numbers below are in-process round trips. Take a look at [benchmarking.md](docs/benchmarking.md) for the exact info on
benchmarking and [benchmark_results.md](docs/benchmark_results.md) for the full tables, system specifications and environment.

| Application     | Request | p50, ns | p99, ns | p99.9, ns | Throughput, req/s |
|-----------------|---------|---------|---------|-----------|-------------------|
| `replay_el_app` | ADD     | <TBD>   | <TBD>   | <TBD>     | <TBD>             |
| `replay_el_app` | CANCEL  | <TBD>   | <TBD>   | <TBD>     |                   |
| `replay_el_app` | REPLACE | <TBD>   | <TBD>   | <TBD>     |                   |
| `replay_cb_app` | ADD     | <TBD>   | <TBD>   | <TBD>     | <TBD>             |
| `replay_mt_app` | ADD     | <TBD>   | <TBD>   | <TBD>     | <TBD>             |

## Main specifications

- Each stage satisfies the `ProcessorOf` concept. The same stage is compiled into callback, event-loop and multi threaded
applications. Take a look at [architecture.md](docs/architecture.md).
- Each transport satisfies the `SinkOf` concept. `SPSCQueue` is lock-free with cached indices, correct memory ordering and
128-bytes separation of the producer and consumer states. `AlignedQueue` is a wrapper that puts every element on its own
cache line. Take a look at [base.md](docs/base.md).
- There are no heap allocations on the hot path: it's checked in runtime, `operator new` is overridden and `AllocatorTracking`
sums up all allocations that actually happened after initialization. In case there are some allocations, the app fails after
the loop.
- `StlL3BookContainer` that is used to store l3 book is basic `std::map` + `std::list` + `std::unordered_map`, but each
node is preallocated with `PlaceArena`. Take a look at [matching.md](docs/matching.md).
- All requests, reports and market data events are timestamped with TSC and written to a journal. Events are joined by 
`origin_time` by `calc_stats` and those round trips are grouped by security, request type and the number of trades. 
`just evaluate` print latencies by percentile of each group.
- All benchmarks should be run on system with isolated cores, with huge pages allocated by OS and specific setup of `nohz_full`,
`mlockall` and so on. The page cache is dropped prior to each run. Every run is fingerprinted. `just evaluate` emits `IMPROVED`/
`REGRESSED`/`INCONSISTENT` taking the drift into account. Take a look at [benchmarking.md](docs/benchmarking.md).
- `ShiftOrderGenerator` resemble a real market: it's basically a state machine per each security with quiet periods, price
changes and book replenishment after price change. Take a look at [order_generation.md](docs/order_generation.md).

## Requirements

- Linux (developed and benchmarked on Ubuntu 26.04 LTS)
- C++23 compiler (GCC 15 or Clang 21, the presets and conan profiles specifically target those versions), CMake (>= 4.2.3), 
Conan 2, Just, Ninja, ccache

Compiler versions used for development are GCC 15.2.0 and Clang 21.1.8.

The scripts expect an Intel CPU with `intel_pstate` and no SMT. Take a look at [benchmarking.md](docs/benchmarking.md) for
details.

## Quickstart

    just bootstrap
    just build
    just test

`just bootstrap` installs apt packages and creates a Python venv at `.venv/` with packages required.

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

There are currently three order generators implemented: `SimpleOrderGenerator`, `ShiftOrderGenerator` and `ReplayOrderGenerator`. 
Take a look at the [order_generation.md](docs/order_generation.md) for details.

## Designs

There are 8 apps that implement different order generations, design and transport between components.

| Application     | Transport                              | OrderGenerator | Design                                                              |
|-----------------|----------------------------------------|----------------|---------------------------------------------------------------------|
| `simple_cb_app` | Direct callback, no queueing           | Simple         | Single-threaded                                                     |
| `simple_el_app` | Queues                                 | Simple         | Single event-loop on one thread                                     |
| `simple_mt_app` | Single-producer-single-consumer queues | Simple         | Each component on a separate thread that is pinned to separate core |
| `shift_cb_app`  | Direct callback, no queueing           | Shift          | Single-threaded                                                     |
| `shift_el_app`  | Queues                                 | Shift          | Single event-loop on one thread                                     |
| `replay_cb_app` | Direct callback, no queueing           | Replay         | Single-threaded                                                     |
| `replay_el_app` | Queues                                 | Replay         | Single event-loop on one thread                                     |
| `replay_mt_app` | Single-producer-single-consumer queues | Replay         | Each component on a separate thread that is pinned to separate core |

Empty app is `empty_app`. It's a separate app that does nothing.

Currently `ShiftOrderGenerator` is not supported with multi-threaded stages because of the current implementation of the shift design.

## What we're measuring

Latency is measured e2e, a tsc timestamp is taken once the OrderGenerator emits the request and a tsc timestamp is taken 
once it receives the market data/reports back. All requests/reports/market data events are written to respective POD Journals
along with timestamps, calc_stats then matches events by origin_time and calculates the round-trips. Currently,
first_report/first_market_event and last_report/last_market_event round trips are calculated.

Current latency includes the time events have spent waiting in the transport. Multithreaded app cannot be compared with single-threaded app directly,
since no event spends any significant time waiting in the transport in a single-threaded app. If there's a need to compare latency of a multithreaded app with a single-threaded app,
`max_pending_transactions` must be set to 1 so that there would be events of a single request in the queues.

- `check_isolation` verifies the actual hardware and operating system state prior to benchmarking.
- Metadata that contains the current setup, git states, application sha is written, runs are archived under labels (`branch-git_revision[:6](-dirty)` by default) with full fingerprint.
- The dirty page cache is drained prior to each run. Every run executes in a separate systemd-run scope pinned to isolated cores.

Take a look at [benchmarking.md](docs/benchmarking.md) for more info.

## Result evaluation

`just benchmark` runs each of apps several times, calculating the latencies during each run. `just evaluate` can either evaluate a
single app or compare two apps:

- Single app evaluation measures the drift of single app across runs by percentile. It emits either `-` or `INCONSISTENT` based on drift.
- Two-way app evaluation measures the difference between latency percentile of two apps. It emits either `-`, `REGRESSED` or `IMPROVED` based on data.
- Both modes print a summary with requests per second and rejections count by request type and rejection reason.

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

- Currently `ShiftOrderGenerator` and `SimpleOrderGenerator` use only one ClientID{0}.
- Currently `ShiftOrderGenerator` is not supported with multi-threaded stages because of the current implementation of the shift design.
- Please note that the hard drive setup may affect the measurements. The replay app writes several million events per second.
- Although mac-clang profile and mac-clang-debug-asan preset exist, it is not guaranteed code works or even is compilable on macOS. 
It will not be tested in CI and it should be used for development purposes only.

## Future plans

- Get rid of StlContainers and allocations on hot path
- Security sharding in multi_threaded_app
