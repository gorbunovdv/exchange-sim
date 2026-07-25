#!/usr/bin/python3
import argparse
import datetime
import hashlib
import json

import plumbum
from helpers.config import load_config
from helpers.root_path import ROOT_PATH

CACHE_VARIABLES = [
    "CMAKE_BUILD_TYPE",
    "CMAKE_CXX_COMPILER",
    "CMAKE_C_COMPILER",
    "CMAKE_CXX_FLAGS",
    "CMAKE_CXX_FLAGS_RELEASE",
    "CMAKE_CXX_COMPILER_LAUNCHER",
    "CMAKE_GENERATOR",
    "SANITIZER",
]


def sha256sum(path):
    hash = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(2**10), b""):
            hash.update(chunk)
    return hash.hexdigest()


def run_cmd(cmd, path=ROOT_PATH, timeout=1):
    try:
        with plumbum.local.cwd(path):
            _, out, _ = cmd.run(timeout=timeout)
            return out.strip()
    except Exception:
        return ""


def git_metadata(repo_root):
    return {
        "sha": run_cmd(plumbum.local["git"]["rev-parse"]["HEAD"]),
        "branch": run_cmd(plumbum.local["git"]["rev-parse"]["--abbrev-ref"]["HEAD"]),
        "dirty": bool(run_cmd(plumbum.local["git"]["status"]["--porcelain"])),
    }


def parse_cmake_cache(build_dir):
    cache_path = build_dir / "CMakeCache.txt"
    if not cache_path.exists():
        raise Exception(f"Cache path does not exist, path={cache_path}")
    wanted = {}
    for line in cache_path.read_text().splitlines():
        if line.startswith("#") or line.startswith("//") or ":" not in line or "=" not in line:
            continue
        name = line.split(":", 1)[0]
        if name in CACHE_VARIABLES:
            wanted[name] = line.split("=", 1)[1]
    for var in CACHE_VARIABLES:
        if var not in wanted:
            raise Exception(f"Could not quite catch variable, variable={var}")
    return wanted


def compilation_metadata(repo_root, preset):
    build_dir = repo_root / f"cmake-build-{preset}"
    return {"preset": preset, "build_dir": str(build_dir), "cache_variables": parse_cmake_cache(build_dir)}


def collect_metadata(config_path, config, preset, application):
    return {
        "timestamp": datetime.datetime.now(datetime.UTC).isoformat(),
        "application": application,
        "application_sha": sha256sum(f"cmake-build-{preset}/exchange/application/{application}"),
        "config_sha": sha256sum(config_path),
        "git": git_metadata(ROOT_PATH),
        "compilation": compilation_metadata(ROOT_PATH, preset),
        "config": config,
    }


def write_metadata(config, config_path, preset, output_path, application, quiet=False):
    metadata = collect_metadata(config_path, config, preset, application)
    with open(output_path, "w") as f:
        print(json.dumps(metadata, indent=2), file=f)
    if not quiet:
        print(json.dumps(metadata, indent=2))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("config_file", help="Path to the config file")
    parser.add_argument("--preset", required=True, type=str, help="CMake preset the build will use")
    parser.add_argument("--application", required=True, type=str, help="Application that will be run")
    parser.add_argument("-q", "--quiet", action="store_true", help="Quiet mode")
    args = parser.parse_args()

    config = load_config(args.config_file)

    write_metadata(
        config=config,
        preset=args.preset,
        output_path=config["results"]["metadata"],
        config_path=args.config_file,
        application=args.application,
        quiet=args.quiet,
    )


if __name__ == "__main__":
    main()
