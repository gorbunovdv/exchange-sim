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

    just benchmark ./configs/dmitrys-ubuntu.json --apps single_threaded_event_loop_app --reps 10

Each run executes in a fresh systemd-run scope pinned to isolated cores with the dirty page cache drained between runs.
Runs are archived under a label `(git_revision[:6] + ('-dirty' if dirty))` with full metadata (git revision, binary hashes, compiler flags, config hash),
while comparisons refuse mismatched fingerprints by default.

    just evaluate ./configs/dmitrys-ubuntu.json --apps <app1> <app2> --labels <label> --percentiles 50 99

Or:

    just evaluate ./configs/dmitrys-ubuntu.json --apps <app2> --labels <label1> <label2> --percentiles 50 99

For a single application this reports median +- max drift per bucket and flags anomalous between-runs
drift (INCONSISTENT), for two applications/labels it reports median differences with the combined noise floor
in every row and a verdict (IMPROVED / REGRESSED / -).

GRUB must be patched to isolate CPU cores and remove certain interrupts:

    sudo just patch-grub
    reboot

For measurements and to be able to run perf successfully run this on Linux after applying CPU isolation and
performance settings (settings do not persist after reboot, apply-runtime must be run each time):

    sudo just apply-runtime

The benchmark must be run on a machine with isolated cores and performance settings applied.

## Restrictions

- Although mac-clang profile and mac-clang-debug-asan preset exist, it is not guaranteed code works or even is compilable on macOS. It will not be tested in CI and it should be used for development purposes only.
