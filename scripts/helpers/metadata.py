import datetime
import hashlib
import json
import re
import typing
from pathlib import Path

import plumbum
from check_isolation import IsolationMetadata

from helpers.config import load_config


def sha256sum(path):
    if not Path(path).exists():
        return "file not exists"
    hash = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(2**10), b""):
            hash.update(chunk)
    return hash.hexdigest()


class Metadata:
    _CACHE_VARIABLES: typing.ClassVar = [
        "CMAKE_BUILD_TYPE",
        "CMAKE_CXX_COMPILER",
        "CMAKE_C_COMPILER",
        "CMAKE_CXX_COMPILER_LAUNCHER",
        "CMAKE_GENERATOR",
        "SANITIZER",
    ]

    def __init__(self, repo_root, preset, config_path):
        self.repo_root = repo_root
        self.preset = preset
        self.build_dir = repo_root / f"cmake-build-{preset}"
        self.compilation_commands_path = self.build_dir / "compile_commands.json"
        self.config_path = self.repo_root / config_path
        self.config = load_config(self.config_path)
        self._isolation_metadata = IsolationMetadata()

    @staticmethod
    def _flags_from_command(cmd):
        _SKIP = []
        flags = []
        skip = False
        for token in cmd.split()[1:]:
            if skip:
                skip = False
                continue
            if token in _SKIP:
                skip = True
                continue
            if not token.startswith("-"):
                continue
            if token.startswith(("-i", "-I", "-o", "-c")):
                continue
            flags.append(token)
        return flags

    def _run_cmd(self, cmd, timeout=1):
        with plumbum.local.cwd(self.repo_root):
            _, out, _ = cmd.run(timeout=timeout)
            return out.strip()

    def git_metadata(self):
        return {
            "sha": self._run_cmd(plumbum.local["git"]["rev-parse"]["HEAD"]),
            "branch": self._run_cmd(plumbum.local["git"]["rev-parse"]["--abbrev-ref"]["HEAD"]),
            "dirty": bool(self._run_cmd(plumbum.local["git"]["status"]["--porcelain"])),
        }

    def isolation_metadata(self):
        return self._isolation_metadata.get_metadata()

    def encode_label(self):
        metadata = self.git_metadata()
        branch = metadata["branch"].replace("/", "__")
        sha = metadata["sha"][:6]
        return f"{branch}-{sha}" + ("-dirty" if metadata["dirty"] else "")

    def _parse_cmake_cache(self):
        cache_path = self.build_dir / "CMakeCache.txt"
        if not cache_path.exists():
            raise Exception(f"Cache path does not exist, path={cache_path}")
        wanted = {}
        for line in cache_path.read_text().splitlines():
            if line.startswith("#") or line.startswith("//") or ":" not in line or "=" not in line:
                continue
            name = line.split(":", 1)[0]
            if name in self._CACHE_VARIABLES:
                wanted[name] = line.split("=", 1)[1]
        for var in self._CACHE_VARIABLES:
            if var not in wanted:
                raise Exception(f"Could not quite catch variable, variable={var}")
        return wanted

    def app_targets(self):
        targets = {}
        if not self.compilation_commands_path.exists():
            raise Exception(
                f"compile_commands.json is absent, run compile before using Metadata,"
                f" path={self.compilation_commands_path}"
            )
        for entry in json.loads(self.compilation_commands_path.read_text()):
            match = re.compile(r"CMakeFiles\/([^/]+)\.dir\/").search(entry["output"])
            if match:
                app_name = match.group(1)
                if app_name.endswith("_app") and app_name != "empty_app":
                    targets[app_name] = entry
        return targets

    def compilation_metadata(self):
        build_dir = self.repo_root / f"cmake-build-{self.preset}"
        return {
            "preset": self.preset,
            "build_dir": str(build_dir),
            "cache_variables": self._parse_cmake_cache(),
        }

    def collect_metadata(self, application, run_metadata):
        return {
            "timestamp": datetime.datetime.now(datetime.UTC).isoformat(),
            "application": application,
            "application_sha": sha256sum(f"cmake-build-{self.preset}/exchange/application/{application}"),
            "config_sha": sha256sum(self.config_path),
            "replay_sha": sha256sum(self.config["order_generator"]["generated_replay"]),
            "git": self.git_metadata(),
            "flags": self._flags_from_command(self.app_targets()[application]["command"]),
            "compilation": self.compilation_metadata(),
            "config": self.config,
            "run_metadata": run_metadata,
            "isolation_metadata": self.isolation_metadata(),
        }
