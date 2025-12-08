#!/usr/bin/env python3
# coding: utf-8
# Copyright (c) 2026 Huawei Device Co., Ltd.
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


import argparse
import shutil
import sys
import zipfile
from collections import defaultdict
from pathlib import Path
from typing import List, Dict, DefaultDict

import benchmark_runner
from metrics_utils import find_ets_files, parse_perf_file, get_run_count_from_file


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Update static performance values from CI artifacts")
    parser.add_argument("--test-dir", type=str, required=True, help="Path to test directory")
    parser.add_argument("--zips-from-ci", type=str, required=True, help="Path to folder with CI zip artifacts")
    return parser.parse_args()


def unpack_archives(zips_dir: Path, output_dir: Path) -> List[Path]:
    if output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir()

    zip_files = list(zips_dir.glob("*.zip"))
    if not zip_files:
        raise RuntimeError(f"No zip files found in {zips_dir}")

    for zip_path in zip_files:
        extract_path = output_dir / zip_path.stem
        extract_path.mkdir()
        try:
            with zipfile.ZipFile(zip_path, "r") as z:
                z.extractall(extract_path)
        except zipfile.BadZipFile as e:
            raise RuntimeError(f"Failed to extract {zip_path}: {e}") from e

    return zip_files


def aggregate_files(files: List[Path]) -> Dict[str, Dict[str, float]]:
    aggregated: DefaultDict[str, Dict] = defaultdict(lambda: {"time_ns": 0.0, "mem_bytes": 0.0})
    total_runs = 0

    for file_path in files:
        runs = get_run_count_from_file(file_path)
        total_runs += runs
        metrics = parse_perf_file(file_path)

        for phase, data in metrics.items():
            aggregated[phase]["time_ns"] += data["time_ns"] * runs
            aggregated[phase]["mem_bytes"] += data["mem_bytes"] * runs

    result: Dict = {"metrics": [], "total_runs": total_runs}

    if total_runs > 0:
        for phase, sums in aggregated.items():
            result["metrics"].append(
                {
                    "phase": phase,
                    "avg_time_ns": sums["time_ns"] / total_runs,
                    "avg_mem_bytes": sums["mem_bytes"] / total_runs,
                }
            )
        result["metrics"].sort(key=lambda x: x["avg_time_ns"], reverse=True)

    return result


def process_target(target_name: str, unarchived_dir: Path, expected_count: int, output_path: Path) -> None:
    perf_filename = f"{target_name}-current-perf.txt"
    found_files = list(unarchived_dir.rglob(perf_filename))

    if len(found_files) != expected_count:
        raise RuntimeError(f"Mismatch for {target_name}: expected {expected_count} files, found {len(found_files)}")

    print(f"Processing {target_name}: found {len(found_files)} files.")
    data: Dict = aggregate_files(found_files)

    benchmark_runner.save_averaged_results(data["metrics"], output_path, data["total_runs"])


def main() -> None:
    args = parse_args()
    test_dir = Path(args.test_dir)
    zips_dir = Path(args.zips_from_ci)
    unarchived_dir = zips_dir / "unarchived"

    if not test_dir.is_dir():
        raise RuntimeError(f"Test directory not found: {test_dir}")
    if not zips_dir.is_dir():
        raise RuntimeError(f"Zips directory not found: {zips_dir}")

    print("Unpacking archives...")
    zip_files = unpack_archives(zips_dir, unarchived_dir)
    expected_count = len(zip_files)

    targets = [("etsstdlib", test_dir / "etsstdlib-max.txt")]
    for ets_file in find_ets_files(test_dir):
        targets.append((ets_file.stem, ets_file.with_name(f"{ets_file.stem}-max.txt")))

    success = True
    for target_name, max_file_path in targets:
        try:
            process_target(target_name, unarchived_dir, expected_count, max_file_path)
        except RuntimeError as e:
            print(f"Error processing {target_name}: {e}")
            success = False

    shutil.rmtree(unarchived_dir)

    if not success:
        sys.exit(1)
    print("\n✅ All static values updated successfully.")


if __name__ == "__main__":
    main()
