# Benchmarking

## Machine setup

Cores must be isolated in order to run benchmarks.

    sudo just patch-grub

`just patch-grub` backs up `/etc/default/grub` and replaces the kernel cmdline to:

    isolcpus=domain,managed_irq,2-7 nohz_full=2-7 rcu_nocbs=2-7
    irqaffinity=0-1,8-23 default_hugepagesz=1G hugepagesz=1G hugepages=8

The shared and all dedicated cpu cores in the json runtime config must be isolated. 

Runtime settings must also be applied:

    sudo just apply-runtime

It applies:
- `performance` governor on all cores
- Enables turbo with `no_turbo=0`
- `kptr_restrict=0`, `perf_event_paranoid=1` for perf
- `system.slice` and `user.slice` are set to the nonisolated cores

## HW & OS settings rationale

The main benchmarking machine has Intel Core Ultra 9 285K, 64GB RAM and Ubuntu 26.04 under hood.

Here are the main non-obvious points behind the settings (or lack of them):

- Intel Core Ultra 9 285K lacks the SMT support, so there's no need to turn it off.
- The app has been tested with turbo enabled/disabled. Turned on turbo makes frequency a lot higher (up to 5.7 GHz vs 3.7 GHz 
of basic frequency). The current result with turbo stays within CONSISTENT threshold, so there's little need to turn it off.
- The app has been tested with/without core sleep prevention (`processor.max_cstate=1 intel_idle.max_cstate=0`). The core is 
able to achieve a better frequency during the app benchmark with other cores allowed to sleep, so it's been decided to leave core sleep prevention off.

## Pre-run checks

`check_isolation.py` checks the actual hardware, kernel and OS state prior to benchmark. 
Each of the settings above is validated and written as metadata:

- every core from config is isolated and all settings (nohz_full/rcu_nocbs/irqaffinity) applied correctly.
- turbo is enabled
- `performance` governor
- There are enough available OS allocated huge pages if the config reserves huge pages.

## just benchmark

    just benchmark <config> [--apps APPS...] [--reps REPS] [--label LABEL] [--use-perf] [--no-aslr]

- Script rebuilds all targets in preset from scratch
- Default label is `branch-git_revision[:6](-dirty)`. `just remove-label` and `just rename-label` are implemented for convenience.
- Default app set is: every `*_app` target except `empty_app`.
- For each `(rep, app)`:
  - page cache is dropped (`sync; echo 3 > drop_caches`)
  - the app is run under `hft.slice` scope with `-p AllowedCPUs=<cpus>`, each run lasts `--timeout` seconds (a minute by default). `MIMALLOC_RESERVE_HUGE_OS_PAGES` is passed from config.
  - `calc_stats.cpp` joins all journals by `origin_time` and prints json with latencies
  - `calc_stats.py` aggregates all these latencies into statistics by percentiles
  - artifacts (perf, metadata, logs, stats) are copied to `<benchmark_path>/<app>/<label>/run_<rep>/`
- `--use-perf` and `--perf-parameters` could be used to run `perf stat` on each run and copy results
- `--no-aslr` could be used to disable ASLR

## Fingerprinting

Each run metadata json contains:

- timestamp
- app name and executable sha256
- git state
  - branch
  - revision
  - is_dirty
- compiler flags
- preset
- cmake cache variables (sanitizers, build type, compilers)
- full config copy
- observed isolation metadata

## just evaluate

    just evaluate <config> --apps APP1 [APP2] --labels LABEL1 [LABEL2] --percentiles 50 99.9

Fingerprints must match within one app/label (unless `--no-fingerprint-check` enabled). Groups with fewer than 10000 events are skipped. First `calc_stats.requests_to_skip` requests are skipped. Currently four metric values are calculated: `first_report`, `last_report`, `first_market_data`, `last_market_data`. Available percentiles are `[0, 50, 75, 99, 99.9, 100]`.

### Single mode

Single mode evaluates one app/label. It calculates the percentiles and the drift of this percentile across the runs: `median +- drift (relative%)`. The bucket is flagged as `INCONSISTENT` when relative drift is more than 2% and absolute drift is more than 10ns.

### Compare mode

Compare mode compares either two apps or two labels (or both): `delta_median +- (drift_1 + drift_2) (relative%)`. The verdict is `IMPROVED`/`REGRESSED` in case `abs(delta_median) > drift_1 + drift_2 + 10` and `relative` is more than 2%.
