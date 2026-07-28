import argparse

from calc_stats import latency_fields
from tabulate import tabulate

from helpers.single_benchmark import SingleBenchmark


def is_significant_single(pct, rel_threshold, abs_threshold):
    _, _, abs_diff, rel_diff = pct
    return "INCONSISTENT" if rel_diff > rel_threshold and abs_diff > abs_threshold else "-"


def is_significant_pair(pct1, pct2, rel_threshold, abs_threshold):
    _, median1, abs_diff1, _ = pct1
    _, median2, abs_diff2, _ = pct2
    if abs(median2 - median1) / median1 * 100 > rel_threshold:
        if median2 - median1 > abs_diff1 + abs_diff2 + abs_threshold:
            return "REGRESSED"
        if median1 - median2 > abs_diff1 + abs_diff2 + abs_threshold:
            return "IMPROVED"
    return "-"


class SingleRunEvaluation:
    def __init__(self, percentiles, relative_threshold, absolute_threshold):
        self.percentiles = percentiles
        self.relative_threshold = relative_threshold
        self.absolute_threshold = absolute_threshold

    def print_metric_table(self, metric, rbgbp):
        metrics_table = []
        for group in sorted(rbgbp.keys()):
            pct_dict = rbgbp[group]
            if not all(percentile in pct_dict for percentile in self.percentiles):
                raise Exception(
                    f"Not all of percentiles are available for group,"
                    f" symbol={group.symbol} type={group.type} trades_count={group.trades_count}"
                )
            pct_items = [pct_dict[percentile] for percentile in self.percentiles]
            row = [group.symbol, group.type, group.trades_count, pct_items[0][0]]
            for item in pct_items:
                _, median, abs_diff, rel_diff = item
                row.extend(
                    [
                        f"{median:.02f} +- {abs_diff:.02f} ({rel_diff:.02f}%)",
                        is_significant_single(item, self.relative_threshold, self.absolute_threshold),
                    ]
                )
            metrics_table.append(row)
        headers = ["symbol", "type", "trades_count", "events_count"]
        for percentile in self.percentiles:
            headers.extend((f"pct {percentile} median +- max drift (rel drift %)", "actual type"))
        print(f"======={metric}========")
        print(tabulate(metrics_table, headers=headers, tablefmt="github", intfmt=",", floatfmt=",.2f"))


class MultiRunEvaluation:
    def __init__(self, percentiles, rel_threshold, abs_threshold):
        self.percentiles = percentiles
        self.rel_threshold = rel_threshold
        self.abs_threshold = abs_threshold

    def print_metric_table_comparison(self, metric, rbgbp1, rbgbp2):
        metrics_table = []
        common_groups = rbgbp1.keys() & rbgbp2.keys()
        for group in sorted(common_groups):
            pct_dict1 = rbgbp1[group]
            pct_dict2 = rbgbp2[group]
            if not all(percentile in pct_dict1 for percentile in self.percentiles):
                raise Exception(
                    f"Not all of percentiles are available for group in run1,"
                    f" symbol={group.symbol} type={group.type} trades_count={group.trades_count}"
                )
            if not all(percentile in pct_dict2 for percentile in self.percentiles):
                raise Exception(
                    f"Not all of percentiles are available for group in run2,"
                    f" symbol={group.symbol} type={group.type} trades_count={group.trades_count}"
                )
            pct1_items = [pct_dict1[percentile] for percentile in self.percentiles]
            pct2_items = [pct_dict2[percentile] for percentile in self.percentiles]
            row = [group.symbol, group.type, group.trades_count, min(pct1_items[0][0], pct2_items[0][0])]
            for item1, item2 in zip(pct1_items, pct2_items, strict=True):
                _, median1, abs_diff1, _ = item1
                _, median2, abs_diff2, _ = item2
                median_difference = median2 - median1
                rel_difference = abs(median_difference) / median1 * 100
                row.extend(
                    [
                        f"{median1}",
                        f"{median2}",
                        f"{median_difference:.02f} +- {abs_diff1 + abs_diff2:.02f} ({rel_difference:.02f}%)",
                        is_significant_pair(item1, item2, self.rel_threshold, self.abs_threshold),
                    ]
                )
            metrics_table.append(tuple(row))
        headers = ["symbol", "type", "trades_count", "events_count"]
        for percentile in self.percentiles:
            headers.extend(("median1", "median2", f"pct {percentile} median diff +- diff (rel_diff%)", "actual type"))
        print(f"======={metric}=======")
        print(tabulate(metrics_table, headers=headers, tablefmt="github", intfmt=",", floatfmt=",.2f"))


def evaluate_runs(
    *,
    config_path,
    preset,
    app1,
    app2,
    label1,
    label2,
    percentiles,
    no_fingerprint_check,
    rel_threshold,
    abs_threshold,
):
    if app1 == app2 and label1 == label2:
        sb = SingleBenchmark(config_path, preset, app1, label1, no_fingerprint_check)
        es = SingleRunEvaluation(percentiles, rel_threshold, abs_threshold)
        for metric in latency_fields:
            es.print_metric_table(metric, sb.results_by_group_by_percentile(metric))
    else:
        sb1 = SingleBenchmark(config_path, preset, app1, label1, no_fingerprint_check)
        sb2 = SingleBenchmark(config_path, preset, app2, label2, no_fingerprint_check)
        em = MultiRunEvaluation(percentiles, rel_threshold, abs_threshold)
        for metric in latency_fields:
            em.print_metric_table_comparison(
                metric, sb1.results_by_group_by_percentile(metric), sb2.results_by_group_by_percentile(metric)
            )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("config_file", help="Path to the config file")
    parser.add_argument("--preset", default="clang-release", type=str, help="CMake preset")
    parser.add_argument("--apps", nargs="+", type=str, help="Application that will be compared")
    parser.add_argument("--no-fingerprint-check", action="store_true", help="Do not check for correct fingerprints")
    parser.add_argument("--labels", nargs="*", type=str, help="Labels that will be used for comparison")
    parser.add_argument(
        "--percentiles",
        nargs="+",
        type=float,
        default=[50.0],
        help="Percentiles to compare (50, 75, 99, 99.9 are options)",
    )
    parser.add_argument("--relative-threshold", type=float, default=5.0, help="Relative threshold for comparison")
    parser.add_argument("--absolute-threshold", type=float, default=5.0, help="Absolute threshold for comparison")
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
        percentiles=args.percentiles,
        rel_threshold=args.relative_threshold,
        abs_threshold=args.absolute_threshold,
    )


if __name__ == "__main__":
    main()
