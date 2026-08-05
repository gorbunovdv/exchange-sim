import argparse
from pathlib import Path

import plumbum
from calc_stats import calc_stats
from check_isolation import check_isolation
from plumbum.commands.base import BoundCommand
from plumbum.machines import LocalCommand
from write_metadata import write_metadata

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


def benchmark_one(
    *, app, config_path, preset, mimalloc_verbose_show_stats, perf, no_aslr, timeout, hard_timeout, huge_pages
):
    config = load_config(config_path)
    if huge_pages is None and "memory" in config and config["memory"]["reserve_huge_os_pages"]:
        huge_pages = config["memory"]["huge_os_pages_count"]
    log_path = ROOT_PATH / config["results"]["logs"]
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with plumbum.local.cwd(ROOT_PATH), open(log_path, "w") as flog:
        just_run = plumbum.local["just"]["run"]
        check_isolation(path=config_path)
        metadata_path = Path(config["results"]["metadata"])
        write_metadata(
            config_path=config_path,
            preset=preset,
            app=app,
            quiet=True,
            output_path=metadata_path,
            run_metadata={
                "perf": perf,
                "timeout": timeout,
                "hard_timeout": hard_timeout,
                "no_aslr": no_aslr,
                "mimalloc_verbose_show_stats": mimalloc_verbose_show_stats,
                "huge_pages": huge_pages,
            },
        )
        cmd = plumbum.local[ROOT_PATH / f"cmake-build-{preset}/exchange/application" / app][config_path]
        if mimalloc_verbose_show_stats or huge_pages:
            env: BoundCommand | LocalCommand = plumbum.local["env"]
            if mimalloc_verbose_show_stats:
                env = env["MIMALLOC_VERBOSE=1"]["MIMALLOC_SHOW_STATS=1"]
            if huge_pages:
                env = env[f"MIMALLOC_RESERVE_HUGE_OS_PAGES={huge_pages}"]
            cmd = env[cmd]
        perf_path = ROOT_PATH / config["results"]["perf"]
        perf_path.unlink(missing_ok=True)
        if perf:
            perf_cmd = plumbum.local["perf"]["stat"]["-o"][perf_path]
            for perf_param in perf:
                perf_cmd = perf_cmd["-e"][perf_param]
            cmd = perf_cmd["--"][cmd]
        if no_aslr:
            cmd = plumbum.local["setarch"]["--addr-no-randomize"][cmd]
        cmd = plumbum.local["timeout"]["-s"]["INT"]["-k"][hard_timeout][timeout][cmd]
        cmd = plumbum.local["python3"][ROOT_PATH / "scripts/run_isolated.py"][cmd]
        print("Running app: ", cmd)
        cmd.run(
            stdout=flog,
            stderr=flog,
            retcode=(
                0,
                124,
            ),
        )
        calc_stats_cmd = just_run[ROOT_PATH / f"cmake-build-{preset}/exchange/application/calc_stats"][config_path]
        print("Running calc_stats: ", calc_stats_cmd)
        calc_stats_cmd.run(stdout=flog, stderr=flog)
        calc_stats(input_file=config["calc_stats"]["output_file"], output_file=config["results"]["stats"], quiet=True)


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
    args = parser.parse_args()
    benchmark_one(
        app=args.app,
        config_path=args.config_path,
        preset=args.preset,
        mimalloc_verbose_show_stats=args.mimalloc_verbose_show_stats,
        perf=args.perf_parameters if args.use_perf else [],
        no_aslr=args.no_aslr,
        timeout=args.timeout,
        hard_timeout=args.hard_timeout,
        huge_pages=args.huge_pages,
    )


if __name__ == "__main__":
    main()
