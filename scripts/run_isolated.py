#!/usr/bin/env python3

import json
import os
import sys
from pathlib import Path


def main():
    cmd, config = list(sys.argv[1:-1]), Path(sys.argv[-1])
    if not cmd:
        raise Exception("No command provided")
    cores_json = json.loads(config.read_text())["cpu_cores"]
    if "disable_core_pinning" not in cores_json or not cores_json["disable_core_pinning"]:
        cores = [cores_json["shared_core"], *cores_json["dedicated_cores"]]
        cmd = [
            "systemd-run",
            "--scope",
            "--slice=hft.slice",
            "-p",
            "AllowedCPUs={}".format(",".join(map(str, cores))),
            *cmd,
            str(config),
        ]
    else:
        cmd = [
            *cmd,
            str(config),
        ]
    print(f"Running command: {cmd}")
    try:
        os.execvp(cmd[0], cmd)
    except Exception as e:
        print(e)
        sys.exit(1)


if __name__ == "__main__":
    main()
