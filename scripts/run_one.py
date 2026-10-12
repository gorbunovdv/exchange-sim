import argparse
import sys
from pathlib import Path

import plumbum
from check_isolation import check_isolation
from plumbum.commands.base import BoundCommand
from plumbum.machines import LocalCommand

from helpers.config import load_config
from helpers.root_path import ROOT_PATH

DEFAULT_PERF = [
    "cpu_core/instructions/",
    "cpu_core/cycles/",
    "cpu_core/l2_rqsts.miss/",
    "cpu_core/mem_load_retired.l3_miss/",
    "cpu_core/LLC-load-misses/",
    "cpu_core/dtlb_load_misses.walk_completed/",
    "cpu_core/dtlb_load_misses.walk_active/",
    "cpu_core/dtlb_store_misses.walk_completed/",
    "context-switches",
    "cpu-migrations",
    "page-faults",
    "major-faults",
    "cpu_core/ld_blocks.address_alias/",
    "cpu_core/ld_blocks.store_forward/",
]


def clear_cache():
    plumbum.local["sudo"]["sh"]["-c"]["sync; echo 3 > /proc/sys/vm/drop_caches"] & plumbum.FG


def run_one(
    *,
    preset,
    app,
    config_path,
    mimalloc_verbose_show_stats,
    huge_pages,
    perf,
    perf_path=None,
    no_aslr,
    hard_timeout,
    timeout,
    skip_isolation_check,
    skip_cache_drop=False,
    build=True,
    flog=sys.stdout,
):
    config = load_config(config_path)
    skip_isolation_check = skip_isolation_check or config.get("skip_isolation_check", False)
    if build:
        with plumbum.local.cwd(ROOT_PATH):
            plumbum.local["just"]["build-from-scratch"][preset].run(stdout=sys.stdout, stderr=sys.stderr)
    if not skip_isolation_check:
        check_isolation(path=config_path)
    if perf and perf_path is None:
        perf_path = config["results"]["perf"]
    if huge_pages is None and "memory" in config and config["memory"]["reserve_huge_os_pages"]:
        huge_pages = config["memory"]["huge_os_pages_count"]
    cmd = plumbum.local[ROOT_PATH / f"cmake-build-{preset}/exchange/application" / app][config_path]
    if mimalloc_verbose_show_stats or huge_pages:
        env: BoundCommand | LocalCommand = plumbum.local["env"]
        if mimalloc_verbose_show_stats:
            env = env["MIMALLOC_VERBOSE=1"]["MIMALLOC_SHOW_STATS=1"]
        if huge_pages:
            env = env[f"MIMALLOC_RESERVE_HUGE_OS_PAGES={huge_pages}"]
        cmd = env[cmd]
    if perf:
        perf_cmd = plumbum.local["perf"]["stat"]["-o"][perf_path]
        for perf_param in perf:
            perf_cmd = perf_cmd["-e"][perf_param]
        cmd = perf_cmd["--"][cmd]
    if no_aslr:
        cmd = plumbum.local["setarch"]["--addr-no-randomize"][cmd]
    cmd = plumbum.local["timeout"]["-s"]["INT"]["-k"][hard_timeout][timeout][cmd]
    cmd = plumbum.local["python3"][ROOT_PATH / "scripts/run_isolated.py"][cmd]
    skip_cache_drop = skip_cache_drop or config.get("skip_cache_drop", False)
    if not skip_cache_drop:
        clear_cache()
    print("Running app: ", cmd)
    cmd.run(
        retcode=(
            0,
            124,
        ),
    )
    if perf and flog == sys.stdout:
        print(Path(perf_path).read_text())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--app", required=True, help="Application to benchmark")
    parser.add_argument("config_path", help="Path to the configuration file")
    parser.add_argument("--preset", default="clang-release", help="CMake preset to use")
    parser.add_argument("--mimalloc-verbose-show-stats", action="store_true", help="Show mimalloc verbose stats")
    parser.add_argument("--use-perf", action="store_true", help="Use perf")
    parser.add_argument("--perf-parameters", nargs="*", default=DEFAULT_PERF, help="Parameters to pass to perf")
    parser.add_argument("--no-aslr", action="store_true", help="Do not use aslr")
    parser.add_argument("--timeout", type=int, default=60, help="Timeout for the benchmark")
    parser.add_argument("--hard-timeout", type=int, default=5, help="Hard timeout for the benchmark")
    parser.add_argument("--huge-pages", type=int, help="Number of huge pages to reserve")
    parser.add_argument("--skip-isolation-check", action="store_true", help="Skip isolation check")
    parser.add_argument("--skip-cache-drop", action="store_true", help="Skip cache drop")
    args = parser.parse_args()

    run_one(
        preset=args.preset,
        config_path=args.config_path,
        mimalloc_verbose_show_stats=args.mimalloc_verbose_show_stats,
        perf=args.perf_parameters if args.use_perf else [],
        no_aslr=args.no_aslr,
        hard_timeout=args.hard_timeout,
        timeout=args.timeout,
        skip_isolation_check=args.skip_isolation_check,
        huge_pages=args.huge_pages,
        app=args.app,
        skip_cache_drop=args.skip_cache_drop,
    )


if __name__ == "__main__":
    main()
