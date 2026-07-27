import dataclasses
import json
from collections import defaultdict
from pathlib import Path

from helpers.metadata import Metadata
from helpers.root_path import ROOT_PATH


@dataclasses.dataclass(eq=True, frozen=True, order=True)
class Group:
    symbol: str
    type: str
    trades_count: int
    percentile: float


@dataclasses.dataclass(eq=True, frozen=True, order=True)
class GroupWithoutPercentile:
    symbol: str
    type: str
    trades_count: int


class SingleBenchmark:
    @staticmethod
    def _get_fingerprint(path: Path):
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
            label = self.metadata.encode_label()
            print(f"No label provided, using label={label}")
        benchmark_path = Path(self.metadata.config["benchmark_path"]) / application / label
        if not benchmark_path.exists():
            raise Exception(f"No such benchmark found, path={benchmark_path}")
        total_run_paths = list(benchmark_path.glob("*"))
        if len(total_run_paths) == 0:
            raise Exception(f"No runs found in benchmark_path={benchmark_path}")
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
                results_by_group[
                    Group(result["symbol"], result["type"], result["trades_count"], float(result["percentile"]))
                ].append(result)
        results_by_group_by_percentile: defaultdict[
            GroupWithoutPercentile, dict[float, tuple[int, int, int, float]]
        ] = defaultdict(dict)
        for group, values in sorted(results_by_group.items()):
            values.sort(key=lambda j: j["value"])
            median = values[len(values) // 2]["value"]
            max_abs = max(abs(median - value["value"]) for value in values)
            max_rel = max_abs / abs(median) * 100
            current_group = GroupWithoutPercentile(group.symbol, group.type, group.trades_count)
            results_by_group_by_percentile[current_group][group.percentile] = (
                min(value["events_count"] for value in values),
                median,
                max_abs,
                max_rel,
            )
        return results_by_group_by_percentile
