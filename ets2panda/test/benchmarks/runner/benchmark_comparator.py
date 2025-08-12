#!/usr/bin/env python3
# coding: utf-8
# Copyright (c) 2025 Huawei Device Co., Ltd.
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


import os
from pathlib import Path
from typing import Dict, Optional, Tuple, List, Callable

from metrics_utils import parse_perf_file, format_diff, format_time_ms, format_mem_mb


def _get_phase_comparison(phase: str, base: Optional[Dict], new: Optional[Dict]) -> Tuple[str, str, str]:
    if base and new:
        label = phase
        time_diff = new["time_ns"] - base["time_ns"]
        mem_diff = new["mem_bytes"] - base["mem_bytes"]
        time_str = format_diff(time_diff, base["time_ns"], format_time_ms)
        mem_str = format_diff(mem_diff, base["mem_bytes"], format_mem_mb)
    elif new:
        label = f"{phase} [NEW]"
        time_str = f"+{format_time_ms(new['time_ns'])}"
        mem_str = f"+{format_mem_mb(new['mem_bytes'])}"
    elif base:
        label = f"{phase} [REMOVED]"
        time_str = f"-{format_time_ms(base['time_ns'])}"
        mem_str = f"-{format_mem_mb(base['mem_bytes'])}"
    else:
        return "", "", ""
    return label, time_str, mem_str


def _write_report(report_path: Path, base_name: str, new_name: str, results: List[Dict]) -> None:
    if not results:
        print("No common or unique phases to compare.")
        return

    max_phase_len = max(len(r["phase"]) for r in results)
    header = f"Performance Comparison: '{base_name}' vs '{new_name}'\n" + "=" * 80

    lines = [header]
    for r in results:
        phase_str = f":@{r['phase']}"
        lines.append(f"{phase_str:<{max_phase_len + 3}}:  time={r['time_str']:<25} mem={r['mem_str']:<25}")

    report_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"\n✅ Comparison finished! Results saved to: {report_path}")


def _print_and_log(level: str, msg: str, log_dir: Path) -> None:
    if level == "Error":
        print(f"\n❌ {msg}")
        path = log_dir / "error_log.txt"
    else:
        print(f"\n⚠️ {msg}")
        path = log_dir / "warning_log.txt"
    with os.fdopen(os.open(path, os.O_WRONLY | os.O_CREAT | os.O_APPEND, mode=511), "a", encoding="utf-8") as f:
        f.write(msg + "\n")


def _check_regression(
    metric_name: str,
    base_data: Dict,
    new_data: Dict,
    regression: float,
    perf_name: str,
    log_dir: Path,
    is_static: bool = False,
) -> None:
    format_func: Callable[[float], str]
    success = True
    if metric_name == "Time":
        key = "time_ns"
        format_func = format_time_ms
    elif metric_name == "Memory":
        key = "mem_bytes"
        format_func = format_mem_mb
    else:
        raise RuntimeError(f"Unsupported metric: {metric_name}")

    base_sum = sum(base_data.get(p, {}).get(key, 0) for p in ["phases", "EmitProgram"])
    new_sum = sum(new_data.get(p, {}).get(key, 0) for p in ["phases", "EmitProgram"])

    upper_threshold = base_sum * (1 + regression)
    if new_sum > upper_threshold:
        msg = (
            f"[PERF REGRESSION] Failed for {perf_name}: {metric_name} exceeded upper threshold.\n"
            f"\tLimit: {regression:.1%}, Actual: +{((new_sum / base_sum) - 1) * 100:.2f}%\n"
            f"\tBase: {format_func(base_sum)}, New: {format_func(new_sum)}\n"
            f"\tThreshold: < {format_func(upper_threshold)}\n"
        )
        _print_and_log("Error", msg, log_dir)
        success = False

    lower_threshold = base_sum * (1 - regression * 3)
    if is_static and new_sum < lower_threshold:
        msg = (
            f"[UPDATE REQUIRED] Very good perf for {perf_name}: {metric_name} exceeded lower threshold.\n"
            f"\tLimit: -{regression * 3:.1%}, Actual: {((new_sum / base_sum) - 1) * 100:.2f}%\n"
            f"\tBase: {format_func(base_sum)}, New: {format_func(new_sum)}\n"
            f"\tThreshold: > {format_func(lower_threshold)}\n\n"
            "Please update *-max.txt.\n"
        )
        _print_and_log("Warning", msg, log_dir)
        success = False

    if success:
        print(f"\n✅ {metric_name} regression check for {perf_name} finished!")
        msg = (
            f"[UPDATE REQUIRED] Perf statistics for {perf_name}: {metric_name} exceeded lower threshold.\n"
            f"\tLimit: {regression:.1%}, Actual: +{((new_sum / base_sum) - 1) * 100:.2f}%\n"
            f"\tBase: {format_func(base_sum)}, New: {format_func(new_sum)}\n"
        )
        _print_and_log("Info", msg, log_dir)


def compare_perf_files(
    new_perf_path: Path, base_perf_path: Path, report_path: Path, regression: float, log_dir: Path
) -> None:
    base_data = parse_perf_file(base_perf_path)
    new_data = parse_perf_file(new_perf_path)

    if not new_data:
        raise RuntimeError("New perf data is empty")
    if not base_data:
        raise RuntimeError("Base perf data is empty")

    is_static = base_perf_path.name.find("-max.txt") != -1
    _check_regression("Time", base_data, new_data, regression, new_perf_path.name, log_dir, is_static)
    _check_regression("Memory", base_data, new_data, regression, new_perf_path.name, log_dir, is_static)
    results = []
    all_phases = sorted(list(set(base_data.keys()) | set(new_data.keys())))

    for phase in all_phases:
        label, time_str, mem_str = _get_phase_comparison(phase, base_data.get(phase), new_data.get(phase))
        if label:
            results.append({"phase": label, "time_str": time_str, "mem_str": mem_str})

    _write_report(report_path, base_perf_path.name, new_perf_path.name, results)
