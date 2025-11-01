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


import sys
from pathlib import Path
import argparse
from arg_parser import parse_arguments, check_arguments
import benchmark_runner
import benchmark_comparator
from metrics_utils import find_ets_files, get_max_path


def run_tests_benchmarks(args: argparse.Namespace, work_dir: Path) -> None:
    test_dir = Path(args.test_dir)
    regression_limit = args.static_regression if args.mode == "static" else args.dynamic_regression

    for test_file in find_ets_files(test_dir):
        print(f"\n{'='*20} Processing: {test_file.name} {'='*20}")

        new_perf_path = work_dir / f"{test_file.stem}-current-perf.txt"
        output = work_dir / f"{test_file.stem}-current.abc"
        es2panda_args = ["--dump-perf-metrics", "--extension=ets", f"--output={output}", f"{test_file}"]

        benchmark_runner.run_benchmark_for_file([f"{args.es2panda}"] + es2panda_args, args.runs, new_perf_path)

        if args.mode == "static":
            base_perf_path = get_max_path(test_file)
        else:
            base_perf_path = work_dir / f"{test_file.stem}-pre_merge-perf.txt"
            output = work_dir / f"{test_file.stem}-pre_merge.abc"
            command = [f"{args.es2panda_pre_merge}"] + es2panda_args
            benchmark_runner.run_benchmark_for_file(command, args.runs, base_perf_path)

        report_path = work_dir / f"{test_file.stem}-report.txt"
        benchmark_comparator.compare_perf_files(new_perf_path, base_perf_path, report_path, regression_limit, work_dir)


def run_stdlib_benchmark(args: argparse.Namespace, work_dir: Path) -> None:
    regression_limit = args.static_regression if args.mode == "static" else args.dynamic_regression

    new_perf_path = work_dir / "etsstdlib-current-perf.txt"
    output = work_dir / "etsstdlib-current.abc"

    parts = Path(args.test_dir).parts
    arktsconfig = Path(*parts[: parts.index("static_core") + 1]) / "plugins" / "ets" / "stdlib" / "stdconfig.json"

    es2panda_args = [
        "--dump-perf-metrics",
        "--extension=ets",
        "--gen-stdlib=true",
        f"--output={output}",
        "etsstdlib",
        f"--arktsconfig={arktsconfig}",
    ]

    benchmark_runner.run_benchmark_for_file([f"{args.es2panda}"] + es2panda_args, args.runs, new_perf_path)

    if args.mode == "static":
        base_perf_path = Path(args.test_dir) / "etsstdlib-max.txt"
    else:
        base_perf_path = work_dir / "etsstdlib-pre_merge-perf.txt"
        output = work_dir / "etsstdlib-pre_merge.abc"
        command = [f"{args.es2panda_pre_merge}"] + es2panda_args
        benchmark_runner.run_benchmark_for_file(command, args.runs, base_perf_path)

    report_path = work_dir / "etsstdlib-report.txt"
    benchmark_comparator.compare_perf_files(new_perf_path, base_perf_path, report_path, regression_limit, work_dir)


def report_log_file(level: str, path: Path) -> None:
    print(
        f"\n{'=' * 20} [{level.upper()}S DETECTED] {'=' * 20}\n"
        f"{level} log found at: {path}\n\n"
        f"{path.read_text()}\n"
        f"{'=' * 70}\n"
        "Hint: Download the CI artifacts to see the performance report.\n"
        "For static (pre-merge) benchmarks check the *-current-perf.txt files with "
        "current run's results and compare them against the maximum values in <ets2panda>/test/benchmarks/*-max.txt."
    )


if __name__ == "__main__":
    arguments = check_arguments(parse_arguments())

    artifacts_dir = Path(arguments.work_dir)
    artifacts_dir.mkdir(parents=True, exist_ok=True)

    error_log_path = artifacts_dir / "error_log.txt"
    warning_log_path = artifacts_dir / "warning_log.txt"
    error_log_path.unlink(missing_ok=True)
    warning_log_path.unlink(missing_ok=True)

    run_stdlib_benchmark(arguments, artifacts_dir)
    run_tests_benchmarks(arguments, artifacts_dir)

    PASSED = True
    if warning_log_path.is_file():
        report_log_file("Warning", warning_log_path)
        if arguments.werror:
            PASSED = False

    if error_log_path.is_file():
        report_log_file("Error", error_log_path)
        PASSED = False

    if not PASSED:
        print("\n❌ Benchmarks failed.")
        sys.exit(1)

    print("\n✅ All benchmarks completed successfully without performance regressions.")
