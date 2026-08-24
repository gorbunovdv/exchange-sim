#!/usr/bin/env python3

import argparse
import os
from datetime import datetime
from pathlib import Path

import plumbum

from helpers.config import load_config


def build_cmdline(config):
    if "cpu_cores" not in config:
        raise Exception("No isolated cores in config file")
    cpu_cores = config["cpu_cores"]

    isolated_cores = [cpu_cores["shared_core"], *cpu_cores["dedicated_cores"]]
    cpu_count = os.cpu_count()
    all_cores = set(range(cpu_count))
    irq_cores = all_cores - set(isolated_cores)
    if not irq_cores:
        raise Exception("At least one nonisolated core is required")

    isolated = ",".join(map(str, sorted(isolated_cores)))
    irq_affinity = ",".join(map(str, sorted(irq_cores)))
    parameters = [
        f"isolcpus=domain,managed_irq,{isolated}",
        f"nohz_full={isolated}",
        f"rcu_nocbs={isolated}",
        f"irqaffinity={irq_affinity}",
    ]

    if "memory" in config and config["memory"]["reserve_huge_os_pages"]:
        parameters.extend(
            [
                "default_hugepagesz=1G",
                "hugepagesz=1G",
                f"hugepages={config['memory']['huge_os_pages_count']}",
            ]
        )
    return " ".join(parameters)


def backup_grub(grub_path):
    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    plumbum.local["cp"][grub_path][f"{grub_path}.bak_{timestamp}"].run()


def patch_grub(*, config_path, dry_run=False):
    GRUB_PATH = Path("/etc/default/grub")

    cmdline = build_cmdline(load_config(config_path))
    if dry_run:
        print(f'GRUB_CMDLINE_LINUX_DEFAULT="{cmdline}"')
        return

    backup_grub(GRUB_PATH)
    sed = plumbum.local["sed"]
    replace_sed_str = f's/^GRUB_CMDLINE_LINUX_DEFAULT=.*/GRUB_CMDLINE_LINUX_DEFAULT="{cmdline}"/'
    sed = sed["-i"][replace_sed_str][GRUB_PATH]
    sed.run()
    plumbum.local["update-grub"].run()
    print("Patched CPU cores, please make sure to reboot your system for the changes to take effect")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("config_file", type=Path, help="Path to the config file")
    parser.add_argument("--dry-run", action="store_true", help="Dry run mode")
    args = parser.parse_args()

    patch_grub(config_path=args.config_file, dry_run=args.dry_run)


if __name__ == "__main__":
    main()
