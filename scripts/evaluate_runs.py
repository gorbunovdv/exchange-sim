import argparse

from calc_stats import latency_fields, percentiles
from helpers.single_benchmark import SingleBenchmark
from tabulate import tabulate


def print_metric_table_comparison(rbgbp1, rbgbp2, metric):
    metrics_table = []
    common_groups = rbgbp1.keys() & rbgbp2.keys()
    for group in sorted(common_groups):
        pct_dict1 = rbgbp1[group]
        pct_dict2 = rbgbp2[group]
        if sorted(pct_dict1.keys()) != sorted(percentiles):
            raise Exception(
                f"Not all of percentiles are available for group in run1,"
                f" symbol={group.symbol} type={group.type} trades_count={group.trades_count}"
            )
        if sorted(pct_dict2.keys()) != sorted(percentiles):
            raise Exception(
                f"Not all of percentiles are available for group in run1,"
                f" symbol={group.symbol} type={group.type} trades_count={group.trades_count}"
            )
        pct1_items = [j[1] for j in sorted(pct_dict1.items())]
        pct2_items = [j[1] for j in sorted(pct_dict2.items())]
        row = [group.symbol, group.type, group.trades_count, min(pct1_items[0][0], pct2_items[0][0])]
        for item1, item2 in zip(pct1_items, pct2_items, strict=True):
            median_difference = item2[1] - item1[1]
            rel_difference = abs(median_difference) / item1[1] * 100
            row.extend((median_difference, rel_difference))
        metrics_table.append(tuple(row))
    headers = ["symbol", "type", "trades_count", "events_count"]
    for percentile in percentiles:
        headers.extend((f"pct {percentile} median difference", "rel difference%"))
    print(f"======={metric}=======")
    print(tabulate(metrics_table, headers=headers, tablefmt="github", intfmt=",", floatfmt=",.2f"))


def print_metric_table(rbgbp, metric):
    metrics_table = []
    for group in sorted(rbgbp.keys()):
        pct_dict = rbgbp[group]
        if sorted(pct_dict.keys()) != sorted(percentiles):
            raise Exception(
                f"Not all of percentiles are available for group,"
                f" symbol={group.symbol} type={group.type} trades_count={group.trades_count}"
            )
        pct_items = [j[1] for j in sorted(pct_dict.items())]
        row = [group.symbol, group.type, group.trades_count, pct_items[0][0]]
        for item in pct_items:
            row.extend([item[1], item[2], item[3]])
        metrics_table.append(row)
    headers = ["symbol", "type", "trades_count", "events_count"]
    for percentile in percentiles:
        headers.extend((f"pct {percentile} median", "max drift", "rel drift %"))
    print(f"======={metric}========")
    print(tabulate(metrics_table, headers=headers, tablefmt="github", intfmt=",", floatfmt=",.2f"))


def evaluate_runs(*, config_path, preset, app1, app2, label1, label2, no_fingerprint_check):
    if app1 == app2 and label1 == label2:
        sb = SingleBenchmark(config_path, preset, app1, label1, no_fingerprint_check)
        for metric in latency_fields:
            print_metric_table(sb.results_by_group_by_percentile(metric), metric)
    else:
        sb1 = SingleBenchmark(config_path, preset, app1, label1, no_fingerprint_check)
        sb2 = SingleBenchmark(config_path, preset, app2, label2, no_fingerprint_check)

        for metric in latency_fields:
            print_metric_table_comparison(
                sb1.results_by_group_by_percentile(metric), sb2.results_by_group_by_percentile(metric), metric
            )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("config_file", help="Path to the config file")
    parser.add_argument("--preset", default="clang-release", type=str, help="CMake preset")
    parser.add_argument("--apps", nargs="+", type=str, help="Application that will be compared")
    parser.add_argument("--no-fingerprint-check", action="store_true", help="Do not check for correct fingerprints")
    parser.add_argument("--labels", nargs="*", type=str, help="Labels that will be used for comparison")
    args = parser.parse_args()

    if not args.apps or len(args.apps) > 2:
        raise Exception("Apps count must be less than 2 and nonempty")
    app1, app2 = args.apps[0], args.apps[min(1, len(args.apps) - 1)]
    if not args.labels or len(args.labels) == 0:
        label1, label2 = "", ""
    else:
        if len(args.labels) > 2:
            raise Exception("Labels count must be less than 2")
        label1, label2 = args.labels[0], args.labels[min(1, len(args.labels) - 1)]

    evaluate_runs(
        config_path=args.config_file,
        preset=args.preset,
        app1=app1,
        app2=app2,
        label1=label1,
        label2=label2,
        no_fingerprint_check=args.no_fingerprint_check,
    )


if __name__ == "__main__":
    main()
