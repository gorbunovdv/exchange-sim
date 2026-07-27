#!/usr/bin/env python3

import argparse
import json
import sys
from pathlib import Path

from helpers.config import load_config

SYS_ISOLATED = Path("/sys/devices/system/cpu/isolated")


def parse_cpulist(text):
    cpus: set[int] = set()
    text = text.strip()
    if not text:
        return []
    for part in text.split(","):
        part = part.strip()
        if not part:
            continue
        if "-" in part:
            first, last = map(int, part.split("-"))
            cpus.update(range(first, last + 1))
        else:
            cpus.add(int(part))
    return sorted(cpus)


def read_isolated():
    if not SYS_ISOLATED.exists():
        raise Exception("Isolated CPUs file not found")
    return parse_cpulist(SYS_ISOLATED.read_text())


def patch_config(*, path, shared, dedicated, dry_run, quiet):
    data = load_config(path)

    current = data.get("cpu_cores")
    if current is None:
        raise Exception("No isolated CPUs configured in config file")

    desired = {
        "shared_core": shared,
        "dedicated_cores": dedicated,
    }
    if current == desired:
        if not quiet:
            print("No changes needed")
        return 0

    data["cpu_cores"] = desired
    if not quiet:
        print("Desired isolated CPUs:", json.dumps(desired))

    if dry_run:
        print("Config cpu_cores do not match this machine's isolated CPUs - run ./scripts/patch_config.py <config>")
        return 1

    with open(path, "w") as f:
        f.write(json.dumps(data, indent=2))

    return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("config_file", help="Path to the config file")
    parser.add_argument(
        "--dry-run", action="store_true", help="Do not apply changes (exit_code 1 if changes are needed)"
    )
    parser.add_argument("--quiet", action="store_true", help="Do not print anything")
    args = parser.parse_args()

    cores = read_isolated()
    shared, dedicated = cores[0], cores[1:]
    return patch_config(
        path=Path(args.config_file), shared=shared, dedicated=dedicated, dry_run=args.dry_run, quiet=args.quiet
    )


if __name__ == "__main__":
    sys.exit(main())
