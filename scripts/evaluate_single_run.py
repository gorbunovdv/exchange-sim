#!/usr/bin/env python3
import argparse

from calc_stats import latency_fields, percentiles
from helpers.single_benchmark import SingleBenchmark
from tabulate import tabulate


def print_metric_table(results, metric):
    metrics_table = []
    for (symbol, type, trades_count), pct_dict in results.items():
        if sorted(pct_dict.keys()) != sorted(percentiles):
            raise Exception(
                f"Not all of percentiles are available for group,"
                f" symbol={symbol} type={type} trades_count={trades_count}"
            )
        row = [symbol, type, trades_count, next(iter(pct_dict.items()))[1][0]]
        for item in (j[1] for j in sorted(pct_dict.items())):
            row.extend([item[1], item[2], item[3]])
        metrics_table.append(row)
    headers = ["symbol", "type", "trades_count", "events_count"]
    for percentile in percentiles:
        headers.extend((f"pct {percentile} median", "max drift", "rel drift %"))
    print(f"======={metric}========")
    print(tabulate(metrics_table, headers=headers, tablefmt="github", intfmt=",", floatfmt=",.2f"))


def evaluate_single_run(*, config_path, preset, application, label, no_fingerprint_check):
    sb = SingleBenchmark(config_path, preset, application, label, no_fingerprint_check)
    for metric in latency_fields:
        print_metric_table(sb.results_by_group_by_percentile(metric), metric)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("config_file", help="Path to the config file")
    parser.add_argument("--preset", required=True, type=str, help="CMake preset that will be evaluated")
    parser.add_argument("--application", required=True, type=str, help="Application that will be evaluated")
    parser.add_argument("--no-fingerprint-check", action="store_true", help="Do not check for correct fingerprints")
    parser.add_argument(
        "--label", required=False, help="Label of current run (by default it's 'git-{sha256:6}[-dirty]'"
    )
    args = parser.parse_args()
    evaluate_single_run(
        config_path=args.config_file,
        preset=args.preset,
        application=args.application,
        label=args.label,
        no_fingerprint_check=args.no_fingerprint_check,
    )


if __name__ == "__main__":
    main()
