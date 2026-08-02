#!/usr/bin/env python3

import argparse
import json
from collections import defaultdict

import ijson
import numpy
from tqdm import tqdm

from helpers.config import load_config
from helpers.root_path import ROOT_PATH

latency_fields = [
    "first_report",
    "last_report",
    "first_market_event",
    "last_market_event",
]


percentiles = [0, 50, 75, 90, 99, 99.9, 100]


def calculate_groups(path):
    groups: defaultdict[tuple[str, str, int], defaultdict[str, list[int]]] = defaultdict(lambda: defaultdict(list))
    try:
        with open(path) as f:
            for item in tqdm(
                ijson.items(f, "item"),
                mininterval=10,
                maxinterval=10,
                desc="Parsing initial json",
            ):
                msg_type, trades_count, symbol = item.get("type"), item.get("trades_count"), item.get("symbol")
                dct = groups[(symbol, msg_type, trades_count)]
                for field in latency_fields:
                    if field in item:
                        dct[field].append(item[field])
    except FileNotFoundError as e:
        raise Exception(f"Error: Could not open '{path}'") from e

    return groups


def calculate_metrics(groups):
    results: list[dict[str, str | int | float]] = []
    for metric in tqdm(latency_fields, mininterval=10, maxinterval=10, desc="Calculating metrics"):
        for group_key in sorted(groups.keys()):
            symbol, type_label, trades_count = group_key

            values = groups[group_key][metric]
            if not values:
                continue

            current_results = numpy.percentile(values, percentiles, method="inverted_cdf").tolist()

            results.extend(
                {
                    "metric": metric,
                    "symbol": symbol,
                    "type": type_label,
                    "trades_count": trades_count,
                    "events_count": len(values),
                    "percentile": percentile,
                    "value": value,
                }
                for percentile, value in zip(map(str, percentiles), current_results, strict=True)
            )
    return results


def calc_stats(*, input_file, output_file, quiet):
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

    calc_stats(
        input_file=ROOT_PATH / config["calc_stats"]["output_file"],
        output_file=ROOT_PATH / config["results"]["stats"],
        quiet=args.quiet,
    )


if __name__ == "__main__":
    main()
