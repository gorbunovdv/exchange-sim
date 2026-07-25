#!/usr/bin/env python3

import argparse
import json
import math
import sys
from collections import defaultdict

import ijson
from tqdm import tqdm


def calculate_percentile(data, percentile):
    if not data:
        return 0
    sorted_data = sorted(data)

    if percentile == 0:
        return sorted_data[0]

    index = math.ceil((percentile / 100.0) * len(sorted_data)) - 1

    return sorted_data[max(0, min(index, len(sorted_data) - 1))]


def load_config(config_path):
    try:
        with open(config_path) as f:
            return json.load(f)
    except FileNotFoundError as e:
        print(f"Error: Could not find '{config_path}'", file=sys.stderr)
        raise e


def main():
    parser = argparse.ArgumentParser(description="Calculate percentiles from latency data.")
    parser.add_argument("config_path", type=str, help="Path to the JSON config file")
    args = parser.parse_args()

    config = load_config(args.config_path)

    groups = defaultdict(lambda: defaultdict(list))

    latency_fields = [
        "first_report",
        "last_report",
        "first_market_event",
        "last_market_event",
    ]

    try:
        with open(config["calc_stats"]["output_file"]) as f:
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
    except FileNotFoundError:
        print(
            f"Error: Could not find '{config['calc_stats']['output_file']}'",
            file=sys.stderr,
        )
        return

    percentiles = [50, 75, 90, 99]

    for metric in latency_fields:
        print(f"=== Latency Metric: {metric} ===")
        print(
            f"{'Type':<8} | {'Symbol':<8} | {'Trades Count':<12} | "
            + " | ".join([f"p{p:<4}" for p in percentiles])
            + " | Count"
        )
        print("-" * 68)

        for group_key in sorted(groups.keys()):
            symbol, type_label, trades_count = group_key

            values = groups[group_key][metric]
            if not values:
                continue

            results = [calculate_percentile(values, p) for p in percentiles]
            formatted_results = " | ".join([f"{res:<5}" for res in results])

            print(f"{type_label:<8} | {symbol:<8} | {trades_count:<12} | {formatted_results} | {len(values)}")

        print("\n")


if __name__ == "__main__":
    main()
