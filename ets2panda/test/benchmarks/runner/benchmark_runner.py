#!/usr/bin/env python3
# coding: utf-8
# Copyright (c) 2025-2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


import subprocess
import re
from collections import defaultdict
from pathlib import Path
from typing import List, Dict, Union, DefaultDict

from metrics_utils import parse_metric_value, format_time_ms, format_mem_mb


def run_and_parse(command: List) -> Dict:
    print(f"Executing: {' '.join(command)}")
    try:
        result = subprocess.run(command, capture_output=True, text=True, check=True, encoding="utf-8")
    except (FileNotFoundError, subprocess.CalledProcessError) as e:
        print(f"Error executing command: {e}")
        if isinstance(e, subprocess.CalledProcessError):
            print(f"Stdout:\n{e.stdout}\nStderr:\n{e.stderr}")
        return {}

    line_regex = re.compile(r":@(?P<phase>[\w\/-]+)\s*:\s*time=(?P<time>[\d\.\w]+)\s*maxrss=(?P<mem>[\d\.\w]+)")
    metrics = {}
    for line in result.stdout.splitlines():
        match = line_regex.search(line)
        if match:
            data = match.groupdict()
            metrics[data["phase"]] = {
                "time_ns": parse_metric_value(data["time"]),
                "mem_bytes": parse_metric_value(data["mem"]),
            }
    return metrics


def collect_perf_for_file(command: List, iterations: int) -> DefaultDict[str, Dict]:
    aggregated_results: DefaultDict[str, Dict] = defaultdict(lambda: {"time": 0, "mem": 0, "count": 0})
    for i in range(iterations):
        print(f"--- Running iteration {i + 1}/{iterations} ---")
        metrics = run_and_parse(command)
        if not metrics:
            print("Failed to get metrics for this run. Skipping.")
            continue
        for phase, values in metrics.items():
            aggregated_results[phase]["time"] += values["time_ns"]
            aggregated_results[phase]["mem"] += values["mem_bytes"]
            aggregated_results[phase]["count"] += 1
    return aggregated_results


def average_results(aggregated_results: DefaultDict[str, Dict]) -> List[Dict]:
    averaged_metrics: List[Dict[str, Union[str, float]]] = []
    for phase, data in aggregated_results.items():
        if data["count"] > 0:
            avg_time = float(data["time"]) / int(data["count"])
            avg_mem = float(data["mem"]) / int(data["count"])
            averaged_metrics.append({"phase": phase, "avg_time_ns": avg_time, "avg_mem_bytes": avg_mem})

    averaged_metrics.sort(key=lambda x: x["avg_time_ns"], reverse=True)
    return averaged_metrics


def save_averaged_results(averaged_metrics: List, output_path: Path, runs: int) -> None:
    if not averaged_metrics:
        print("No metrics to save.")
        return

    max_phase_len = max(len(str(m["phase"])) for m in averaged_metrics)
    header = f"================ es2panda perf metrics (Averaged over {runs} runs) ================\n"

    lines = [header]
    for metric in averaged_metrics:
        phase_str = f":@{metric['phase']}"
        time_str = format_time_ms(float(metric["avg_time_ns"]))
        mem_str = format_mem_mb(float(metric["avg_mem_bytes"]))
        lines.append(f"{phase_str:<{max_phase_len + 3}}:  time={time_str:<12}  maxrss={mem_str:<12}")

    output_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"\n✅ Averaged results saved to: {output_path}")


def run_benchmark_for_file(command: List, iterations: int, perf_path: Path) -> None:
    aggregated = collect_perf_for_file(command, iterations)
    if not aggregated:
        raise RuntimeError(f"No data aggregated for {perf_path}")

    averaged = average_results(aggregated)
    save_averaged_results(averaged, perf_path, iterations)
