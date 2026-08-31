#!/usr/bin/env python3

import argparse
import sys
from pathlib import Path

import plumbum
from check_isolation import check_isolation
from run_one import DEFAULT_PERF, run_one
from write_metadata import write_metadata

from helpers.config import load_config
from helpers.root_path import ROOT_PATH


def benchmark_one(
    *,
    app,
    config_path,
    preset,
    mimalloc_verbose_show_stats,
    perf,
    no_aslr,
    timeout,
    hard_timeout,
    huge_pages,
    skip_isolation_check,
    build=True,
):
    if build:
        with plumbum.local.cwd(ROOT_PATH):
            plumbum.local["just"]["build-from-scratch"][preset].run(stdout=sys.stdout, stderr=sys.stderr)
    if not skip_isolation_check:
        check_isolation(path=config_path)
    config = load_config(config_path)
    log_path = ROOT_PATH / config["results"]["logs"]
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with plumbum.local.cwd(ROOT_PATH), open(log_path, "w") as flog:
        just_run = plumbum.local["just"]["run"]
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
        perf_path = ROOT_PATH / config["results"]["perf"]
        perf_path.unlink(missing_ok=True)
        run_one(
            build=False,
            preset=preset,
            app=app,
            config_path=config_path,
            mimalloc_verbose_show_stats=mimalloc_verbose_show_stats,
            huge_pages=huge_pages,
            perf=perf,
            perf_path=perf_path,
            no_aslr=no_aslr,
            hard_timeout=hard_timeout,
            timeout=timeout,
            flog=flog,
            skip_isolation_check=True,
        )
        calc_stats_cmd = just_run[ROOT_PATH / f"cmake-build-{preset}/exchange/application/calc_stats"][config_path]
        print("Running calc_stats: ", calc_stats_cmd)
        calc_stats_cmd.run(stdout=flog, stderr=flog)


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
        skip_isolation_check=args.skip_isolation_check,
    )


if __name__ == "__main__":
    main()
