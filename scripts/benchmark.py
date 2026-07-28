#!/usr/bin/python3
import argparse
import re
import shutil
import time
from pathlib import Path

import plumbum

from helpers.config import load_config
from helpers.metadata import Metadata
from helpers.root_path import ROOT_PATH


def read_dirty():
    result = re.search(r"Dirty:\s+(\d+)", Path("/proc/meminfo").read_text())
    if result is None:
        raise Exception("Could not read dirty memory")
    return int(result.group(1))


def clear_cache():
    plumbum.local["sync"] & plumbum.FG
    while read_dirty() > 100_000:
        print("Waiting for cache to clear...")
        time.sleep(1)


def make_single_run(config_path, config, preset, label, current_rep, app):
    print(f"Running current_rep={current_rep} app={app} preset={preset}")
    clear_cache()
    with plumbum.local.cwd(ROOT_PATH):
        plumbum.local["just"]["benchmark-one"][app][config_path][preset] & plumbum.FG
    dest_path = ROOT_PATH / Path(config["benchmark_path"]) / app / label / f"run_{current_rep:02d}"
    dest_path.mkdir(parents=True, exist_ok=True)
    for path in config["results"].values():
        shutil.copy(path, dest_path)


def benchmark(*, preset, label, reps, apps, config_path):
    with plumbum.local.cwd(ROOT_PATH):
        plumbum.local["just"]["build-from-scratch"][preset] & plumbum.FG
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
                config_path=config_path, config=config, preset=preset, label=label, current_rep=rep, app=app
            )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preset", default="clang-release", help="Preset to use for benchmark")
    parser.add_argument(
        "--label", required=False, help="Label to mark current run (by default it's 'git-{sha256:6}[-dirty]'"
    )
    parser.add_argument("--reps", type=int, default=5, help="Repetitions of run to make")
    parser.add_argument("--apps", nargs="+", type=str, help="Applications to run")
    parser.add_argument("config_path", help="Path to config")

    args = parser.parse_args()

    benchmark(preset=args.preset, label=args.label, reps=args.reps, apps=args.apps, config_path=args.config_path)


if __name__ == "__main__":
    main()
