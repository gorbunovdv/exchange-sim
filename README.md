# exchange-sim

## Overview

A low-latency exchange simulator in C++23.

It models the venue side of electronic market: a matching engine that keeps an
L3 order book per instrument with FIFO price-time priority fed by a synthetic
order generator with a market-data dispatcher and a report generator. The
focus is carefully measured latency and throughput under realistic load.

## Architecture at glance

The lifecycle of a single order is following: SimpleOrderGenerator creates an order -> the request is being passed to MatchingEngine via 
respective transport -> MatchingEngine passes request to L3Book that maintains the respective security -> L3Book performs basic risk checks using 
basic RiskChecker -> L3Book outputs internal engine events to EventDispatcher via respective transport -> EventDispatcher converts
internal engine events to market data events and report events and sends them to SimpleOrderGenerator via respective transport.

The transport layer is based on Sink concept that is implemented using callbacks, queues or spsc queues.

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

    exchange/application/   Applications: single_threaded_event_loop_app, calc_stats and so on
    exchange/domain/        Exchange-specific types: requests, reports, engine_events and so on
    exchange/input/         Order generation classes: SimpleOrderGenerator, OrderGeneratorWrapper and so on
    exchange/matching/      Matching related classes, matching engine and l3 book implementation
    exchange/output/        Event output classes: ExchangeDispatcher

## Designs

There are three apps that implement different design and transport between components.

| Application                    | Transport                              | Design                                                              |
|--------------------------------|----------------------------------------|---------------------------------------------------------------------|
| single_threaded_callback_app   | Direct callback, no queueing           | Single-threaded                                                     |
| single_threaded_event_loop_app | Queues                                 | Single event-loop on one thread                                     |
| multi_threaded_app             | Single-producer-single-consumer queues | Each component on a separate thread that is pinned to separate core |

Components are shared between these apps, only transport/threading design is different.

## What we're measuring

Latency is measured e2e, a tsc timestamp is taken once the SimpleOrderGenerator emits the request and a tsc timestamp is taken 
once it receives the market data/reports back. All requests/reports/market data events are written to respective POD Journals
along with timestamps, calc_stats then matches events by origin_time and calculates the round-trips. Currently,
first_report/first_market_data and last_report/last_market_data round trips are calculated.

- check_isolation verifies the actual hardware and operating system state prior to benchmarking.
- Metadata that contains current setup, git states, application sha is written, runs are archived under git_revision[:6] labels with full fingerprint.
- The dirty page cache is drained prior to each run, every run executes in separate systemd-run scope pinned to isolated cores.

## Result evaluation

just benchmark runs each of apps several times, calculating the latencies during each run. just evaluate can each evaluate a
single app or compare two apps:

- Single app evaluation measures the drift of single app across runs by percentile. It emits either - or INCONSISTENT if drift is too big.
- Two-way app evaluation measures the difference between latency percentile of two apps. It emits either -, REGRESSED or IMPROVED based on data.

Either different apps could be evaluated or one app across different labels.

## Scope

- Matching: FIFO price-time priority, deterministic per instrument
- Order operations: Add / Cancel / Replace
- L3 order book per instrument
- Synthetic order generator, market-data dispatcher, execution reports
- Benchmarking: latency and throughput

## Requirements

- Linux (developed and benchmarked on Ubuntu 26.04 LTS)
- C++23 compiler (GCC or Clang), CMake, Conan 2, Just

## Quickstart
    
    just bootstrap
    just build
    just test

## Core isolation
    
    sudo just patch-grub && sudo reboot
    sudo just apply-runtime
    just benchmark ./configs/dmitrys-ubuntu.json --apps single_threaded_event_loop_app --reps 10
    just evaluate  ./configs/dmitrys-ubuntu.json --apps single_threaded_event_loop_app --labels <label> --percentiles 50 99

The benchmark must be run on a machine with isolated cores and performance settings applied.

## Restrictions

- Although mac-clang profile and mac-clang-debug-asan preset exist, it is not guaranteed code works or even is compilable on macOS. It will not be tested in CI and it should be used for development purposes only.

## Current plans

- Security sharding in multi_threaded_app