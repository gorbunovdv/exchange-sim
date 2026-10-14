# Benchmarking

This document describes the benchmarking machine setup. The numbers themselves are in [benchmark_results.md](benchmark_results.md).

## Machine setup

Cores must be isolated in order to run benchmarks.

    sudo just patch-grub ./configs/dmitrys-ubuntu.json

`just patch-grub` backs up `/etc/default/grub` and replaces the kernel cmdline with the one built from config and runs
`update-grub`. For `configs/dmitrys-ubuntu.json` on Intel Core 9 Ultra 285K machine the grub line would be:

    isolcpus=domain,managed_irq,2,3,4,5,6,7 nohz_full=2,3,4,5,6,7 rcu_nocbs=2,3,4,5,6,7
    irqaffinity=0,1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
    default_hugepagesz=1G hugepagesz=1G hugepages=8

The shared and all dedicated cpu cores in the json runtime config are isolated, every other core is not. The number of 
huge pages is the sum of `memory.huge_os_pages_count` (for mimalloc) plus `memory.huge_os_pages_file_count` (for replay 
file). Reboot is required after `just patch-grub`.

Runtime settings must also be applied:

    sudo just apply-runtime ./configs/dmitrys-ubuntu.json

It applies:
- `performance` governor on all cores
- Enables turbo with `no_turbo=0`
- `kptr_restrict=0`, `perf_event_paranoid=1` for perf
- `system.slice` and `user.slice` are set to the nonisolated cores

In `dmitrys-ubuntu.json` config the request journal is placed under `./tmpfs/` in the config as nvme can't keep up with 
the write rate. Please note that no scripts mount or check it. The hard drive setup do affect the measurement results as
the apps write several million requests per second.

## HW & OS settings rationale

The main benchmarking machine has Intel Core Ultra 9 285K, 64GB RAM, Samsung SSD 9100 PRO 2TB and Ubuntu 26.04 under the hood.

Here are the main non-obvious points behind the settings (or lack of them):

- Intel Core Ultra 9 285K lacks the SMT support, so there's no need to turn it off.
- The app has been tested with turbo enabled/disabled. Turned on turbo makes the frequency a lot higher (up to 5.7 GHz vs 3.7 GHz 
of basic frequency). The current result with turbo stays within the CONSISTENT threshold, so there's little need to turn it off.
- The app has been tested with/without core sleep prevention (`processor.max_cstate=1 intel_idle.max_cstate=0`). The core is 
able to achieve a better frequency during the app benchmark with other cores allowed to sleep, so it's been decided to leave core sleep prevention off.
- 1GB huge pages are used both for replay file and the mimalloc heap. `mlockall` keeps everything resident in memory.

## Pre-run checks

`check_isolation.py` checks the actual hardware, kernel and OS state prior to every run. Each of the settings above is 
validated and written as metadata, the run is aborted if any check fails:

- every core from config in `/sys/devices/system/cpu/isolated` and `nohz_full`
- `rcu_nocbs` in `/proc/cmdline` contains all isolated cores and `irqaffinity` in `/proc/cmdline` contains none of them
- turbo is enabled, `kptr_restrict=0` and `perf_event_paranoid=-1`
- every core is in `performance` mode
- There are enough available OS allocated huge pages if the config reserves huge pages.

## just benchmark

    just benchmark [-h] [--preset PRESET] [--label LABEL] [--reps REPS] [--apps [APPS ...]] [--mimalloc-verbose-show-stats] 
    [--use-perf] [--perf-parameters [PERF_PARAMETERS ...]] [--no-aslr] [--timeout TIMEOUT]
    [--hard-timeout HARD_TIMEOUT] [--huge-pages HUGE_PAGES] [--skip-isolation-check] [--skip-cache-drop]
    config_path

- Script rebuilds all targets in preset from scratch (`clang-release` by default)
- The default label is `branch-git_revision[:6](-dirty)`. The scripts throws if label already exists for any of the apps.
`just remove-label` and `just rename-label` are implemented for convenience.
- Default app set is: each of `*_app` target except `empty_app`.
- Runs are ordered by `rep` and then by `app`.
- For each `(rep, app)`:
  - `check_isolation.py` runs and `metadata.json` is written 
  - page cache is dropped (`sync; echo 3 > drop_caches`, please note that sudo is required)
  - the app is run under `systemd-run --scope --slice=hft.slice -p AllowedCPUs=<cpus>` with `MIMALLOC_RESERVE_HUGE_OS_PAGES` 
passed from config
  - each run is wrapped in `timeout -s INT -k <hard-timeout> <timeout>`, is asked to stop at `--timeout` seconds 
(a minute by default), `--hard-timeout` later it is killed
  - `calc_stats.cpp` joins all journals by `origin_time` and prints json with latencies
  - `calc_stats.cpp` aggregates all these latencies into `stats.json` by percentiles and writes `summary.json`
  with throughput and rejections count
  - all artifacts (perf, metadata, logs, stats, summary) are copied to `<benchmark_path>/<app>/<label>/run_<rep>/`
- Only exit codes 0 and 124 are considered successful, any other exit code stops the run
- `--use-perf` and `--perf-parameters` could be used to run `perf stat` on each run and copy results
- `--no-aslr` could be used to disable ASLR with `setarch --addr-no-randomize`
- `--skip-isolation-check` skip isolation check, but metadata is still collected. You may add keys `skip_isolation_check`,
`skip_cache_drop`, `without_systemd` and `fallback_to_none` to the config, take a look at `config/machine-no-isol.json`.

The full output of the app is saved to `<benchmark_path>/<app>/<label>/run_<rep>/logs.txt`.

`just benchmark-one <config> --app <APP>` runs a single benchmark with the given config and app. `just run-one <config> --app <APP>` 
runs a single app without `calc_stats`. Both of them take pretty much the same arguments, take a look at `just run-one --help`.

`just run <command...> <config>` runs `<command...> <config>` in the isolated environment.

Please note that the replay file must be generated before running any app with `ReplayOrderGenerator`, take a look at
[order_generation.md](order_generation.md#recording-a-flow).

## Fingerprinting

Each run metadata json contains:

- timestamp
- app name and executable sha256
- git state: branch, revision, is_dirty
- compiler flags, preset, cmake cache variables (sanitizers, build type, compilers)
- full config copy and config sha256
- replay file sha256
- run parameters: timeout, hard timeout, perf, ASLR, huge pages
- observed isolation metadata

The fingerprint of the run is this json without timestamp. `just evaluate` requires all runs under a single (app, label) pair
to be identical unless `--no-fingerprint-check` is enabled. Please note that the fingerprints of two labels are not compared
with one another.

## calc_stats

`calc_stats` reads all three journals of the run. For every request `calc_stats` calculates the time from `origin_time` to
the first/the last market data event, the first/the last report and the number of trades. All events are joined to corresponding
requests by `origin_time` within a window of 1s (`constMaxTimeDifference`), a round trip longer fails the run with message
`constMaxTimeDifference is too small`.

First `calc_stats.requests_to_skip` requests (1000 by default) are skipped as warm-up. The output files are:

- `calc_stats.output_file` (`requests_info.json`): one json entity per request with stats to look at outliers
- `results.stats` (`stats.json`): for each percentile (`0, 50, 75, 90, 99, 99.9, 100`) for each of four metric values 
(`first_report`, `last_report`, `first_market_event`, `last_market_event`) for each security, type (`ADD`, `CANCEL`, `REPLACE`),
trades count the latency of those requests with current percentile.
- `results.summary` (`summary.json`): the number of non-skipped requests, the duration between the first and the last request,
throughput in requests per second and rejections counts grouped by request type and reason.

| Metric               | What it measures                                                                   |
|----------------------|------------------------------------------------------------------------------------|
| `first_report`       | `origin_time` to the first report                                                  |
| `last_report`        | `origin_time` to the last report (the full round trip)                             |
| `first_market_event` | `origin_time` to the first market data event (only for requests that produced one) |
| `last_market_event`  | `origin_time` to the last market data event (only for requests that produced one)  |

## just evaluate

    just evaluate [-h] [--preset PRESET] [--apps APPS [APPS ...]] [--no-fingerprint-check] [--labels LABELS [LABELS ...]]
    [--app-label-pairs APP_LABEL_PAIRS [APP_LABEL_PAIRS ...]]
    [--max-trades-count MAX_TRADES_COUNT] [--percentiles PERCENTILES [PERCENTILES ...]] [--relative-threshold RELATIVE_THRESHOLD] 
    [--absolute-threshold ABSOLUTE_THRESHOLD]
    [--min-event-count-threshold MIN_EVENT_COUNT_THRESHOLD]
    config_file

Without labels provided, the default label is used: `branch-git_revision[:6](-dirty)`. The default percentiles are `[0, 50, 100]`.
Groups with fewer than `--min-event-count-threshold` (default: 10000) events or more than `--max-trades-count` (default: 10)
trades are skipped. One table per metric and the summary are printed. 

There's also mode to compare two apps/labels pairs.

    just evaluate <config> --app-label-pairs APP1:LABEL1 APP2:LABEL2 --percentile 50 99

### Single mode

Single mode evaluates one app/label. It calculates the percentiles and the drift of this percentile across the runs: 
`median +- drift (relative%)`. The bucket is flagged as `INCONSISTENT` when relative drift is more than 
`--relative-threshold` (default: 3.5%) and absolute drift is more than `--absolute-threshold` (default: 15ns).

### Compare mode

Compare mode compares either two apps or two labels (or both): for each common group it prints `median1`, `median2`,
`delta_median +- (drift_1 + drift_2) (relative%)`. The verdict is `IMPROVED`/`REGRESSED` in case 
`abs(delta_median) > drift_1 + drift_2 + 15` and `relative` is more than 3.5%, otherwise '-'.

The first app/label is used as a baseline, the second one is compared to it. `REGRESSED` means that the second app/label
is slower than the first one.

### Summary

Both modes print the `summary` table after the latency tables, it is built from `summary.json` of each run:

- `throughput` is requests per second, calculated as the number of non-skipped requests divided by the time between 
the first and the last of them.
- `rejections` is the total count of rejected non-skipped requests, grouped by `rejections <type> <reason>`
- For each `rejectins <type> <reason>` there's a separate line that describes this group.

## Label managements

    just remove-label <label> [config]
    just rename-label <old_label> <new_label> [config]

Those two recipes are added for convenient use of labels. Both operate on `<benchmark_path>/*/<label>`, so they affect
each app that has this label.