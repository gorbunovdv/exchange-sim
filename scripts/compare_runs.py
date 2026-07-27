import argparse

from calc_stats import latency_fields, percentiles
from evaluate_single_run import SingleBenchmark
from tabulate import tabulate


def print_metric_table(rbgbp1, rbgbp2, metric):
    metrics_table = []
    if rbgbp1.keys() != rbgbp2.keys():
        raise Exception("Different groups in two runs")
    for (symbol, type, trades_count), pct_dict1 in rbgbp1.items():
        pct_dict2 = rbgbp2[(symbol, type, trades_count)]
        if sorted(pct_dict1.keys()) != sorted(percentiles):
            raise Exception(
                f"Not all of percentiles are available for group in run1,"
                f" symbol={symbol} type={type} trades_count={trades_count}"
            )
        if sorted(pct_dict2.keys()) != sorted(percentiles):
            raise Exception(
                f"Not all of percentiles are available for group in run1,"
                f" symbol={symbol} type={type} trades_count={trades_count}"
            )
        pct1_items = (j[1] for j in sorted(pct_dict1.items()))
        pct2_items = (j[1] for j in sorted(pct_dict2.items()))
        row = [symbol, type, trades_count, min(next(iter(pct1_items))[0], next(iter(pct2_items))[0])]
        for item1, item2 in zip(pct1_items, pct2_items, strict=True):
            median_difference = item2[1] - item1[1]
            rel_difference = abs(median_difference) / item1[1] * 100
            row.extend((median_difference, rel_difference))
        metrics_table.append(tuple(row))
    headers = ["symbol", "type", "trades_count", "events_count"]
    for percentile in percentiles:
        headers.extend((f"pct {percentile} median difference", "rel difference"))
    print(f"======={metric}=======")
    print(tabulate(metrics_table, headers=headers, tablefmt="github", intfmt=",", floatfmt=",.2f"))


def compare_runs(*, config_path, preset, app1, app2, label1, label2, no_fingerprint_check):
    sb1 = SingleBenchmark(config_path, preset, app1, label1, no_fingerprint_check)
    sb2 = SingleBenchmark(config_path, preset, app2, label2, no_fingerprint_check)

    for metric in latency_fields:
        print_metric_table(
            sb1.results_by_group_by_percentile(metric), sb2.results_by_group_by_percentile(metric), metric
        )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("config_file", help="Path to the config file")
    parser.add_argument("--preset", required=True, type=str, help="CMake preset the build will use")
    parser.add_argument("--app1", required=True, type=str, help="Application that will be run")
    parser.add_argument("--app2", required=True, type=str, help="Application that will be run")
    parser.add_argument("--no-fingerprint-check", action="store_true", help="Do not check for correct fingerprints")
    parser.add_argument("--label1", required=True, help="Label of first run")
    parser.add_argument("--label2", required=True, help="Label of second run")
    args = parser.parse_args()

    compare_runs(
        config_path=args.config_file,
        preset=args.preset,
        app1=args.app1,
        app2=args.app2,
        label1=args.label1,
        label2=args.label2,
        no_fingerprint_check=args.no_fingerprint_check,
    )


if __name__ == "__main__":
    main()
