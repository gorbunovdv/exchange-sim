#!/usr/bin/python3
import argparse
import shutil
from pathlib import Path

import plumbum
from helpers.config import load_config
from helpers.metadata import Metadata
from helpers.root_path import ROOT_PATH


def make_single_run(config_path, config, preset, label, current_rep, app):
    print(f"Running current_rep={current_rep} app={app} preset={preset}")
    with plumbum.local.cwd(ROOT_PATH):
        plumbum.local["just"]["benchmark"][app][config_path][preset] & plumbum.FG
    dest_path = ROOT_PATH / Path(config["benchmark_path"]) / app / label / f"run_{current_rep:02d}"
    dest_path.mkdir(parents=True, exist_ok=True)
    for path in config["results"].values():
        shutil.copy(path, dest_path)


def encode_label(metadata):
    branch = metadata["branch"].replace("/", "__")
    sha = metadata["sha"][:6]
    return f"{branch}-{sha}" + ("-dirty" if metadata["dirty"] else "")


def benchmark(*, preset, label, reps, apps, config_path):
    with plumbum.local.cwd(ROOT_PATH):
        plumbum.local["just"]["build"][preset] & plumbum.FG
    config = load_config(config_path)
    metadata = Metadata(ROOT_PATH, preset, config_path)
    if not label:
        label = encode_label(metadata.git_metadata())
        print(f"No label provided, using label={label}")
    if not apps:
        apps = list(metadata.app_targets().keys())
        print(f"Apps are empty, using current list of apps={apps}")
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
