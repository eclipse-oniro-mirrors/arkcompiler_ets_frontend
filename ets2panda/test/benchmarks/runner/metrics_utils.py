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


import re
from pathlib import Path
from typing import List, Dict, Callable


TIME_UNITS_NS = {"s": 1e9, "ms": 1e6, "us": 1e3, "ns": 1}
MEM_UNITS_BYTES = {"MB": 1024 ** 2, "KB": 1024, "B": 1}
ALL_UNITS = {**TIME_UNITS_NS, **MEM_UNITS_BYTES}


def parse_metric_value(value_str: str) -> float:
    match = re.match(r"(-?[\d\.]+)([a-zA-Z]+)", value_str)
    if not match:
        return 0
    value, unit = match.groups()
    return float(value) * ALL_UNITS.get(unit, 0)


def format_time_ms(ns: float) -> str:
    return f"{ns / 1e6:.2f}ms"


def format_mem_mb(bytes_val: float) -> str:
    return f"{bytes_val / (1024**2):.2f}MB"


def format_diff(diff: float, base: float, formatter_func: Callable[[float], str]) -> str:
    if base == 0 and diff == 0:
        return f"{formatter_func(0)} (0.0%)"

    sign = "+" if diff >= 0 else "-"
    formatted_abs_diff = formatter_func(abs(diff))

    if base != 0:
        percentage = (diff / base) * 100
        return f"{sign}{formatted_abs_diff} ({sign}{abs(percentage):.1f}%)"

    return f"{sign}{formatted_abs_diff} (+inf%)"


def find_ets_files(folder_path: Path) -> List[Path]:
    return list(folder_path.glob("*.ets"))


def get_max_path(file_path: Path) -> Path:
    return file_path.with_name(f"{file_path.stem}-max.txt")


def parse_perf_file(filepath: Path) -> Dict:
    if not filepath.is_file():
        print(f"Warning: Base performance file not found at {filepath}")
        return {}

    data: Dict[str, Dict] = {}
    line_regex = re.compile(r":@(?P<phase>[\w\/-]+)\s*:\s*time=(?P<time_str>[\d\.\w]+)\s*maxrss=(?P<mem_str>[\d\.\w]+)")
    for line in filepath.read_text(encoding="utf-8").splitlines():
        match = line_regex.search(line)
        if match:
            parts = match.groupdict()
            phase_name = parts["phase"]
            data[phase_name] = {
                "time_ns": parse_metric_value(parts["time_str"]),
                "mem_bytes": parse_metric_value(parts["mem_str"]),
            }
    return data


def get_run_count_from_file(filepath: Path) -> int:
    content = filepath.read_text(encoding="utf-8")
    match = re.search(r"Averaged over (\d+) runs", content)
    return int(match.group(1)) if match else 1
