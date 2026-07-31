import argparse
import subprocess
import sys
from pathlib import Path

import plumbum
from calc_stats import calc_stats
from patch_config import patch_config
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
]


def benchmark_one(*, app, config_path, preset, mimalloc_verbose_show_stats, perf, no_alsr, timeout, hard_timeout):
    config = load_config(config_path)
    with plumbum.local.cwd(ROOT_PATH), open(config["results"]["logs"], "w") as flog:
        just_run = plumbum.local["just"]["run"]
        if (ec := patch_config(path=config_path, dry_run=True, quiet=True)) != 0:
            return ec
        metadata_path = Path(config["results"]["metadata"])
        write_metadata(config_path=config_path, preset=preset, app=app, quiet=True, output_path=metadata_path)
        cmd = plumbum.local[ROOT_PATH / f"cmake-build-{preset}/exchange/application" / app][config_path]
        if mimalloc_verbose_show_stats:
            cmd = plumbum.local["env"]["MIMALLOC_VERBOSE=1"]["MIMALLOC_SHOW_STATS=1"][cmd]
        if perf:
            perf_cmd = plumbum.local["perf"]["stat"]["-o"][ROOT_PATH / config["results"]["perf"]]
            for perf_param in perf:
                perf_cmd = perf_cmd["-e"][perf_param]
            cmd = perf_cmd["--"][cmd]
        if no_alsr:
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
        calc_stats_cmd.run(stdout=subprocess.DEVNULL, stderr=sys.stderr)
        calc_stats(input_file=config["calc_stats"]["output_file"], output_file=config["results"]["stats"], quiet=True)
    return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--app", required=True, help="Application to benchmark")
    parser.add_argument("config_path", help="Path to the configuration file")
    parser.add_argument("--preset", default="clang-release", help="CMake preset to use")
    parser.add_argument("--mimalloc-verbose-show-stats", action="store_true", help="Show mimalloc verbose stats")
    parser.add_argument("--use-perf", action="store_true", help="Use perf")
    parser.add_argument("--perf-parameters", nargs="*", default=DEFAULT_PERF, help="Parameters to pass to perf")
    parser.add_argument("--no-alsr", action="store_true", help="Do not use ALSR")
    parser.add_argument("--timeout", type=int, default=60, help="Timeout for the benchmark")
    parser.add_argument("--hard-timeout", type=int, default=5, help="Hard timeout for the benchmark")
    args = parser.parse_args()
    return benchmark_one(
        app=args.app,
        config_path=args.config_path,
        preset=args.preset,
        mimalloc_verbose_show_stats=args.mimalloc_verbose_show_stats,
        perf=args.perf_parameters if args.use_perf else [],
        no_alsr=args.no_alsr,
        timeout=args.timeout,
        hard_timeout=args.hard_timeout,
    )


if __name__ == "__main__":
    sys.exit(main())
