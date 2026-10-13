#!/usr/bin/env python3

import json
import os
import sys
from pathlib import Path


def main():
    cmd, config_path = list(sys.argv[1:-1]), Path(sys.argv[-1])
    if not cmd:
        raise Exception("No command provided")
    config = json.loads(config_path.read_text())
    cores_json = config["cpu_cores"]
    cores = [] if config.get("fallback_to_none", False) else [cores_json["shared_core"]] + cores_json["dedicated_cores"]
    cores_cmd = (
        [
            "-p",
            "AllowedCPUs={}".format(",".join(map(str, cores))),
        ]
        if cores
        else []
    )
    if config.get("without_systemd", False):
        cmd = [*cmd, str(config_path)]
    else:
        cmd = [
            "systemd-run",
            "--scope",
            "--slice=hft.slice",
            *cores_cmd,
            *cmd,
            str(config_path),
        ]
    print(f"Running command: {cmd}")
    try:
        os.execvp(cmd[0], cmd)
    except Exception as e:
        print(e)
        sys.exit(1)


if __name__ == "__main__":
    main()
