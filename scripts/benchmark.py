#!/usr/bin/python3
import argparse
import shutil
import subprocess
import sys
from pathlib import Path

import plumbum
from benchmark_one import DEFAULT_PERF, benchmark_one

from helpers.config import load_config
from helpers.metadata import Metadata
from helpers.root_path import ROOT_PATH


def clear_cache():
    plumbum.local["sudo"]["sh"]["-c"]["sync; echo 3 > /proc/sys/vm/drop_caches"] & plumbum.FG


def make_single_run(
    *,
    config_path,
    config,
    preset,
    label,
    current_rep,
    app,
    timeout,
    hard_timeout,
    mimalloc_verbose_show_stats,
    perf,
    no_aslr,
):
    print(f"Running current_rep={current_rep} app={app} preset={preset}")
    clear_cache()
    benchmark_one(
        app=app,
        config_path=config_path,
        preset=preset,
        timeout=timeout,
        hard_timeout=hard_timeout,
        mimalloc_verbose_show_stats=mimalloc_verbose_show_stats,
        perf=perf,
        no_aslr=no_aslr,
    )
    dest_path = ROOT_PATH / Path(config["benchmark_path"]) / app / label / f"run_{current_rep:02d}"
    dest_path.mkdir(parents=True, exist_ok=True)
    for path in config["results"].values():
        path = ROOT_PATH / path
        if path.exists():
            shutil.copy(path, dest_path)


def benchmark(
    *, preset, label, reps, apps, config_path, timeout, hard_timeout, mimalloc_verbose_show_stats, perf, no_aslr
):
    with plumbum.local.cwd(ROOT_PATH):
        plumbum.local["just"]["build-from-scratch"][preset].run(stdout=subprocess.DEVNULL, stderr=sys.stderr)
    config = load_config(config_path)
    metadata = Metadata(ROOT_PATH, preset, config_path)
    if not label:
        label = metadata.encode_label()
        print(f"No label provided, using label={label}")
    if not apps:
        apps = list(metadata.app_targets().keys())
        print(f"Apps are empty, using current list of apps={apps}")
    for app in apps:
        dest_path = ROOT_PATH / Path(config["benchmark_path"]) / app / label
        if dest_path.exists():
            raise Exception(f"Path {dest_path} already exists, please back it up or remove it")
        dest_path.mkdir(parents=True, exist_ok=False)
    for rep in range(1, reps + 1):
        for app in apps:
            make_single_run(
                config_path=config_path,
                config=config,
                preset=preset,
                label=label,
                current_rep=rep,
                app=app,
                timeout=timeout,
                hard_timeout=hard_timeout,
                mimalloc_verbose_show_stats=mimalloc_verbose_show_stats,
                perf=perf,
                no_aslr=no_aslr,
            )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preset", default="clang-release", help="Preset to use for benchmark")
    parser.add_argument(
        "--label", required=False, help="Label to mark current run (by default it's 'git-{sha256:6}[-dirty]'"
    )
    parser.add_argument("--reps", type=int, default=5, help="Repetitions of run to make")
    parser.add_argument("--apps", nargs="+", type=str, help="Applications to run")
    parser.add_argument("--mimalloc-verbose-show-stats", action="store_true", help="Show mimalloc verbose stats")
    parser.add_argument("--use-perf", action="store_true", help="Use perf")
    parser.add_argument("--perf-parameters", nargs="*", default=DEFAULT_PERF, help="Parameters to pass to perf")
    parser.add_argument("--no-aslr", action="store_true", help="Do not use aslr")

    parser.add_argument("--timeout", type=int, default=60, help="Timeout for each run")
    parser.add_argument("--hard-timeout", type=int, default=5, help="Hard timeout for each run")

    parser.add_argument("config_path", help="Path to config")

    args = parser.parse_args()

    benchmark(
        preset=args.preset,
        label=args.label,
        reps=args.reps,
        apps=args.apps,
        config_path=args.config_path,
        timeout=args.timeout,
        hard_timeout=args.hard_timeout,
        mimalloc_verbose_show_stats=args.mimalloc_verbose_show_stats,
        perf=args.perf_parameters if args.use_perf else [],
        no_aslr=args.no_aslr,
    )


if __name__ == "__main__":
    main()
