import argparse
import json
from pathlib import Path

SYS_ISOLATED = Path("/sys/devices/system/cpu/isolated")

def parse_cpulist(text):
    cpus = set()
    text = text.strip()
    if not text:
        return []
    for part in text.split(","):
        part = part.strip()
        if not part:
            continue
        if '-' in part:
            first, last = map(int, part.split('-'))
            cpus.update(range(first, last + 1))
        else:
            cpus.add(int(part))
    return list(sorted(cpus))

def read_isolated():
    if not SYS_ISOLATED.exists():
        raise Exception("Isolated CPUs file not found")
    return parse_cpulist(SYS_ISOLATED.read_text())

def patch_config(path, shared, dedicated, dry_run):
    original_text = path.read_text()
    try:
        data = json.loads(original_text)
    except json.JSONDecodeError:
        raise Exception("Invalid JSON in config file")

    current = data.get("cpu_cores")
    if current is None:
        raise Exception("No isolated CPUs configured in config file")

    desired = {
        "shared_core": shared,
        "dedicated_cores": dedicated,
    }
    if current == desired:
        print("No changes needed")
        return

    data["isolated"] = desired
    print("Desired isolated CPUs:", json.dumps(desired))

    if dry_run:
        print("Not patching config as requested")
        return

    with open(path, "w") as f:
        f.write(json.dumps(data, indent=2))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('config_file', help='Path to the config file')
    parser.add_argument('--dry-run', action='store_true', help='Do not apply changes')
    args = parser.parse_args()

    cores = read_isolated()
    shared, dedicated = cores[0], cores[1:]
    patch_config(Path(args.config_file), shared, dedicated, dry_run=args.dry_run)


if __name__ == "__main__":
    main()
