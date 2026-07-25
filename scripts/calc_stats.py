#!/usr/bin/env python3

import argparse
import json
import math
from collections import defaultdict

import ijson
from helpers.config import load_config
from tqdm import tqdm


def calculate_percentile(data, percentile):
    if not data:
        return 0
    sorted_data = sorted(data)

    if percentile == 0:
        return sorted_data[0]

    index = math.ceil((percentile / 100.0) * len(sorted_data)) - 1

    return sorted_data[max(0, min(index, len(sorted_data) - 1))]


latency_fields = [
    "first_report",
    "last_report",
    "first_market_event",
    "last_market_event",
]


percentiles = [50, 75, 90, 99, 99.9]


def calculate_groups(path):
    groups = defaultdict(lambda: defaultdict(list))
    try:
        with open(path) as f:
            for item in tqdm(
                ijson.items(f, "item"),
                mininterval=10,
                maxinterval=10,
                desc="Parsing initial json",
            ):
                msg_type = item.get("type")
                trades_count = item.get("trades_count", 0)
                symbol = item.get("symbol")
                group_key = (symbol, msg_type, trades_count)

                for field in latency_fields:
                    if field in item:
                        groups[group_key][field].append(item[field])
    except FileNotFoundError as e:
        raise Exception(f"Error: Could not open '{path}'") from e

    return groups


def calculate_metrics(groups):
    results = []
    for metric in latency_fields:
        for group_key in sorted(groups.keys()):
            symbol, type_label, trades_count = group_key

            values = groups[group_key][metric]
            if not values:
                continue

            current_results = [calculate_percentile(values, p) for p in percentiles]

            results.append(
                {
                    "symbol": symbol,
                    "type": type_label,
                    "trades_count": trades_count,
                    "percentiles": {str(percentiles[i]): current_results[i] for i in range(len(percentiles))},
                }
            )
    return results


def calc_stats(input_file, output_file, quiet):
    groups = calculate_groups(input_file)

    current_results = calculate_metrics(groups)

    with open(output_file, "w") as f:
        print(json.dumps(current_results, indent=2), file=f)

    if not quiet:
        print(json.dumps(current_results, indent=2))


def main():
    parser = argparse.ArgumentParser(description="Calculate percentiles from latency data.")
    parser.add_argument("config_path", type=str, help="Path to the JSON config file")
    parser.add_argument("-q", "--quiet", action="store_true", help="Quiet mode")
    args = parser.parse_args()

    config = load_config(args.config_path)

    calc_stats(input_file=config["calc_stats"]["output_file"], output_file=config["results"]["stats"], quiet=args.quiet)


if __name__ == "__main__":
    main()
