#!/usr/bin/env python3
import argparse
import json
from collections import defaultdict
from pathlib import Path

from benchmark import encode_label
from calc_stats import latency_fields, percentiles
from helpers.root_path import ROOT_PATH
from tabulate import tabulate
from write_metadata import Metadata


class SingleBenchmark:
    @staticmethod
    def _group(j):
        return j["symbol"], j["type"], j["trades_count"], j["percentile"]

    def _get_fingerprint(self, path):
        metadata_path = path / "metadata.json"
        if not metadata_path.exists():
            raise Exception(f"Could not find metadata, path={metadata_path}")
        metadata = json.loads(metadata_path.read_text())
        del metadata["timestamp"]
        return metadata

    def __init__(self, config_path, preset, application, label=None, no_fingerprint_check=False):
        self.config_path = config_path
        self.preset = preset
        self.application = application
        self.metadata = Metadata(ROOT_PATH, preset, config_path)
        self.no_fingerprint_check = no_fingerprint_check
        if not label:
            label = encode_label(self.metadata.git_metadata())
            print(f"No label provided, using label={label}")
        benchmark_path = Path(self.metadata.config["benchmark_path"]) / application / label
        if not benchmark_path.exists():
            raise Exception(f"No such benchmark found, path={benchmark_path}")
        total_run_paths = list(benchmark_path.glob("*"))
        if not self.no_fingerprint_check:
            fingerprint = self._get_fingerprint(next(iter(total_run_paths)))
            for path in total_run_paths:
                current_fingerprint = self._get_fingerprint(path)
                if fingerprint != current_fingerprint:
                    raise Exception("Fingerprints do not match in run")
            print(f"Fingerprint={json.dumps(fingerprint, indent=2)}")
        self.results = []
        for run_path in total_run_paths:
            json_path = run_path / "stats.json"
            if not json_path.exists():
                raise Exception(f"Could not find stats.json, path={json_path}")
            self.results.extend(json.loads(json_path.read_text()))

    def results_by_group_by_percentile(self, metric):
        results_by_group = defaultdict(list)
        for result in self.results:
            if result["metric"] == metric:
                results_by_group[self._group(result)].append(result)
        results_by_group_by_percentile = defaultdict(dict)
        for (symbol, type, trades_count, percentile), values in sorted(results_by_group.items()):
            values.sort(key=lambda j: j["value"])
            median = values[len(values) // 2]["value"]
            max_abs = max(abs(median - value["value"]) for value in values)
            max_rel = max_abs / abs(median) * 100
            results_by_group_by_percentile[(symbol, type, trades_count)][float(percentile)] = (
                next(iter(values))["events_count"],
                median,
                max_abs,
                max_rel,
            )
        return results_by_group_by_percentile


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
