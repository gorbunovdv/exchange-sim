#!/usr/bin/env python3

import sys
import json
import math
from collections import defaultdict

import argparse

def calculate_percentile(data, percentile):
    if not data:
        return 0
    sorted_data = sorted(data)

    if percentile == 0:
        return sorted_data[0]

    index = math.ceil((percentile / 100.0) * len(sorted_data)) - 1

    index = max(0, min(index, len(sorted_data) - 1))

    return sorted_data[index]

def load_config(config_path):
    try:
        with open(config_path, 'r') as f:
            return json.load(f)
    except FileNotFoundError, e:
        print(f"Error: Could not find '{config_path}'", file=sys.stderr)
        raise e

def main():
    parser = argparse.ArgumentParser(description='Calculate percentiles from latency data.')
    parser.add_argument('config_path', type=str, help='Path to the JSON config file')
    args = parser.parse_args()

    config = load_config(args.config_path)

    try:
        with open(config['calc_stats']['output_file'], 'r') as f:
            data = json.load(f)
    except FileNotFoundError:
        print(f"Error: Could not find '{file_path}'", file=sys.stderr)
        return

    groups = defaultdict(lambda: defaultdict(list))

    latency_fields = ['first_report', 'last_report', 'first_market_event', 'last_market_event']

    type_map = {
        0: "ADD",
        1: "CANCEL",
        2: "REPLACE"
    }

    for item in data:
        msg_type = item.get('type', -1)
        trades_count = item.get('trades_count', 0)
        group_key = (msg_type, trades_count)

        for field in latency_fields:
            if field in item:
                groups[group_key][field].append(item[field])

    percentiles = [50, 75, 90, 99]

    for metric in latency_fields:
        print(f"=== Latency Metric: {metric} ===")
        print(f"{'Type':<8} | {'Trades Count':<12} | " + " | ".join([f"p{p:<4}" for p in percentiles]) + " | Count")
        print("-" * 68)

        for group_key in sorted(groups.keys()):
            msg_type, trades_count = group_key
            type_label = type_map.get(msg_type, str(msg_type))

            values = groups[group_key][metric]
            if not values:
                continue

            results = [calculate_percentile(values, p) for p in percentiles]
            formatted_results = " | ".join([f"{res:<5}" for res in results])

            print(f"{type_label:<8} | {trades_count:<12} | {formatted_results} | {len(values)}")

        print("\n")

if __name__ == "__main__":
    main()