#!/usr/bin/env python3

import argparse
import os
from datetime import datetime
from pathlib import Path

import plumbum

from helpers.config import load_config


def get_cpu_cores(config):
    if "cpu_cores" not in config:
        raise Exception("No isolated cores in config file")
    cpu_cores = config["cpu_cores"]

    isolated_cores = [cpu_cores["shared_core"], *cpu_cores["dedicated_cores"]]
    cpu_count = os.cpu_count()
    if cpu_count is None:
        raise Exception("Could not determine CPU count")
    all_cores = set(range(cpu_count))
    irq_cores = all_cores - set(isolated_cores)
    if not irq_cores:
        raise Exception("At least one nonisolated core is required")
    return isolated_cores, irq_cores


def build_cmdline(config):
    isolated_cores, irq_cores = get_cpu_cores(config)

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
                f"hugepages={config['memory']['huge_os_pages_count'] + config['memory']['huge_os_pages_file_count']}",
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
    replace_sed_str = f's/^GRUB_CMDLINE_LINUX_DEFAULT=.*/GRUB_CMDLINE_LINUX_DEFAULT="{cmdline}"/'
    sed = plumbum.local["sed"]["-i"][replace_sed_str][GRUB_PATH]
    sed.run()
    plumbum.local["update-grub"].run()
    print("Patched CPU cores, please make sure to reboot your system for the changes to take effect")


def apply_runtime(*, config_path):
    _, irq_cores = get_cpu_cores(load_config(config_path))
    allowed_cpus = ",".join(map(str, sorted(irq_cores)))
    applying_values = [
        ("0", "/sys/devices/system/cpu/intel_pstate/no_turbo"),
        ("0", "/proc/sys/kernel/kptr_restrict"),
        ("1", "/proc/sys/kernel/perf_event_paranoid"),
    ]

    plumbum.local["cpupower"]["frequency-set"]["-g"]["performance"].run()
    for value, path in applying_values:
        (plumbum.local["echo"][value] | plumbum.local["tee"][path]).run()
    for slice in ["system.slice", "user.slice"]:
        plumbum.local["systemctl"]["set-property"]["--runtime"][slice][f"AllowedCPUs={allowed_cpus}"].run()
    print("Applied runtime settings")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("action", choices=["patch-grub", "apply-runtime"], help="Settings to apply")
    parser.add_argument("config_file", type=Path, help="Path to the config file")
    parser.add_argument("--dry-run", action="store_true", help="Dry run mode")
    args = parser.parse_args()

    if args.action == "patch-grub":
        patch_grub(config_path=args.config_file, dry_run=args.dry_run)
    else:
        if args.dry_run:
            raise Exception("Dry run mode is not supported for apply-runtime action")
        apply_runtime(config_path=args.config_file)


if __name__ == "__main__":
    main()
