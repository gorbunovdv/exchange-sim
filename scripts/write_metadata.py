#!/usr/bin/python3
import argparse
import json
from pathlib import Path

from helpers.config import load_config
from helpers.metadata import Metadata
from helpers.root_path import ROOT_PATH


def write_metadata(*, config_path, preset, output_path, app, quiet=False, dry_run=False):
    metadata = Metadata(ROOT_PATH, preset, config_path).collect_metadata(app)
    if not dry_run:
        output_path.parent.mkdir(parents=True, exist_ok=True)
        with open(output_path, "w") as f:
            print(json.dumps(metadata, indent=2), file=f)
    if not quiet:
        print(json.dumps(metadata, indent=2))
    return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("config_file", type=Path, help="Path to the config file")
    parser.add_argument("--preset", required=True, type=str, help="CMake preset the build will use")
    parser.add_argument("--app", required=True, type=str, help="Application that will be run")
    parser.add_argument("--dry-run", action="store_true", help="Dry run mode")
    parser.add_argument("-q", "--quiet", action="store_true", help="Quiet mode")
    args = parser.parse_args()

    config = load_config(args.config_file)

    write_metadata(
        preset=args.preset,
        output_path=Path(config["results"]["metadata"]),
        config_path=args.config_file,
        app=args.application,
        quiet=args.quiet,
        dry_run=args.dry_run,
    )


if __name__ == "__main__":
    main()
