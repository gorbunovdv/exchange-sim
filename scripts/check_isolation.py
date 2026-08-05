#!/usr/bin/env python3

import argparse
import os
import re
from pathlib import Path

from helpers.config import load_config


class CPUMetadata:
    PREFIX_PATH = Path("/sys/devices/system/cpu")

    def __init__(self, core):
        self.core_path = self.PREFIX_PATH / core

    def scaling_governor(self):
        scaling_governor_path = self.core_path / "cpufreq/scaling_governor"
        if not scaling_governor_path.exists():
            raise Exception(f"Scaling governor file not found, path={scaling_governor_path}")
        return scaling_governor_path.read_text().strip()

    def cpu_idle_states(self):
        cpu_idle_path = self.core_path / "cpuidle"
        if not cpu_idle_path.exists():
            raise Exception(f"CPU idle path not found, path={cpu_idle_path}")
        result = []
        for state in os.listdir(cpu_idle_path):
            cpu_state_name = cpu_idle_path / state / "name"
            result.append(cpu_state_name.read_text().strip())
        return result


class IsolationMetadata:
    def __init__(self):
        self.isolated = self._read_isolated()
        self.nohz_full = self._read_nohz_full()
        self.cmd_line_linux = self._read_cmd_line_linux()

    @staticmethod
    def parse_cores(text):
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

    @classmethod
    def _read_isolated(cls):
        SYS_ISOLATED = Path("/sys/devices/system/cpu/isolated")
        if not SYS_ISOLATED.exists():
            raise Exception(f"Isolated CPUs file not found, path={SYS_ISOLATED}")
        return cls.parse_cores(SYS_ISOLATED.read_text())

    @classmethod
    def _read_nohz_full(cls):
        SYS_NOHZ_FULL = Path("/sys/devices/system/cpu/nohz_full")
        if not SYS_NOHZ_FULL.exists():
            raise Exception(f"Nohz full CPUs file not found, path={SYS_NOHZ_FULL}")
        return cls.parse_cores(SYS_NOHZ_FULL.read_text())

    @staticmethod
    def _read_cmd_line_linux():
        CMD_LINE_LINUX = Path("/proc/cmdline")
        if not CMD_LINE_LINUX.exists():
            raise Exception(f"Linux command line file not found, path={CMD_LINE_LINUX}")
        return CMD_LINE_LINUX.read_text().strip()

    @staticmethod
    def _read_meminfo():
        MEMINFO = Path("/proc/meminfo")
        if not MEMINFO.exists():
            raise Exception(f"Memory information file not found, path={MEMINFO}")
        return {
            key.strip(): value.strip()
            for key, value in (line.split(": ", 1) for line in MEMINFO.read_text().strip().split("\n"))
        }
    @staticmethod
    def _read_no_turbo():
        NO_TURBO = Path("/sys/devices/system/cpu/intel_pstate/no_turbo")
        if not NO_TURBO.exists():
            raise Exception(f"No turbo file not found, path={NO_TURBO}")
        return NO_TURBO.read_text().strip()

    def get_metadata(self):
        return {
            "isolated": self.isolated,
            "nohz_full": self.nohz_full,
            "cmd_line_linux": self._read_cmd_line_linux(),
            "no_turbo": self._read_no_turbo(),
            "huge_page_size": self._read_meminfo()["Hugepagesize"],
            "huge_page_count": self._read_meminfo()["HugePages_Total"],
        }

    def check_cmd_line_linux(self, isolated):
        cmd_line_linux = self._read_cmd_line_linux()
        parameters = dict(line.split("=", 1) for line in cmd_line_linux.split() if line.count("=") >= 1)
        rcu_nocbs = parameters.get("rcu_nocbs")
        if rcu_nocbs is None:
            raise Exception("rcu_nocbs parameter not found in cmd_line_linux")
        rcu_nocbs_cores = self.parse_cores(rcu_nocbs)
        for core in isolated:
            if core not in rcu_nocbs_cores:
                raise Exception(f"Core {core} is not in rcu_nocbs")
        irq_affinity = parameters.get("irqaffinity")
        if irq_affinity is None:
            raise Exception("irqaffinity parameter not found in cmd_line_linux")
        irq_affinity_cores = self.parse_cores(irq_affinity)
        for core in isolated:
            if core in irq_affinity_cores:
                raise Exception(f"Core {core} is in irqaffinity")

    def run_isolation_check(self, isolated, huge_pages):
        if self._read_no_turbo() != "0":
            raise Exception("no_turbo is not disabled")
        for core in isolated:
            if core not in self.isolated:
                raise Exception(f"Core {core} is not isolated")
            if core not in self.nohz_full:
                raise Exception(f"Core {core} is not in nohz_full")
        pattern = re.compile(r"cpu\d+")
        for cpu in os.listdir(CPUMetadata.PREFIX_PATH):
            if not re.fullmatch(pattern, cpu):
                continue
            current_cpu = CPUMetadata(cpu)
            if current_cpu.scaling_governor() != "performance":
                raise Exception(f"Core {cpu} is not in performance mode")
            if any(name not in ["POLL", "C1"] for name in current_cpu.cpu_idle_states()):
                raise Exception(f"Core {cpu} is not in POLL or C1 idle states")
        if huge_pages is not None:
            meminfo = self._read_meminfo()
            if meminfo["Hugepagesize"] != "1048576 kB":
                raise Exception(f"Huge pages size is not 1GB, found: {meminfo['Hugepagesize']}")
            if int(meminfo["HugePages_Total"]) < huge_pages:
                raise Exception(f"Not enough huge pages, expected at least {huge_pages}")
        self.check_cmd_line_linux(isolated)


def check_isolation(*, path):
    metadata = IsolationMetadata()
    data = load_config(path)

    current = data.get("cpu_cores")
    if current is None:
        raise Exception("No isolated CPUs configured in config file")
    huge_pages = None
    if data.get("memory", {}).get("reserve_huge_os_pages", False):
        huge_pages = data["memory"]["huge_os_pages_count"]

    shared, dedicated = current["shared_core"], current["dedicated_cores"]
    isolated = [shared, *dedicated]

    metadata.run_isolation_check(isolated=isolated, huge_pages=huge_pages)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("config_file", help="Path to the config file")
    args = parser.parse_args()

    check_isolation(path=Path(args.config_file))


if __name__ == "__main__":
    main()
