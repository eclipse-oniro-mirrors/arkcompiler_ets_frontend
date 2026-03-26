#!/usr/bin/env python3
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

import os
import subprocess
import sys
from pathlib import Path
from typing import List


def usage() -> str:
    return """Usage:
  run_lsp_benchmarks.sh [options] [-- <benchmark args...>]

Options:
  --out-dir <path>   Build output directory like runtime_core/static_core/out
                     (benchmarks are resolved from <out-dir>/bin)
  --bin-dir <path>   Benchmark binary directory
  --benchmark <name> Run only one benchmark binary (e.g. lsp_get_definition_at_position_perf
                     or get_definition_at_position)
  --result-file <path>
                     Save all benchmark outputs into one file
  --list             List discovered benchmark binaries and exit
  --fail-fast        Stop immediately when one benchmark fails
  -h, --help         Show this help message

Examples:
  ./run_lsp_benchmarks.sh --out-dir /path/to/runtime_core/static_core/out
  ./run_lsp_benchmarks.sh --out-dir /path/to/runtime_core/static_core/out --benchmark get_definition_at_position
  ./run_lsp_benchmarks.sh --out-dir /path/to/runtime_core/static_core/out --list
  ./run_lsp_benchmarks.sh --out-dir /path/to/out --result-file /tmp/lsp_bench.log
  ./run_lsp_benchmarks.sh --out-dir /path/to/runtime_core/static_core/out -- --benchmark_min_time=0.3s
  ./run_lsp_benchmarks.sh --bin-dir /path/to/bin -- --benchmark_filter=.*Rename.*
"""


def print_err(msg: str) -> None:
    print(msg, file=sys.stderr)


def discover_binaries(bin_dir: Path) -> List[str]:
    bins = []
    for entry in bin_dir.iterdir():
        if not entry.is_file():
            continue
        name = entry.name
        if name.startswith("lsp_") and name.endswith("_perf"):
            bins.append(name)
    bins.sort()
    return bins


def run_and_tee(cmd: List[str]) -> tuple[int, str]:
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )
    if proc.stdout is None:
        proc.kill()
        proc.wait()
        msg = "error: failed to capture benchmark output"
        print_err(msg)
        return 1, msg + "\n"
    chunks: List[str] = []
    for line in proc.stdout:
        print(line, end="")
        chunks.append(line)
    proc.wait()
    return proc.returncode, "".join(chunks)


def main(argv: List[str]) -> int:
    bin_dir = ""
    bin_dir_set_explicitly = False
    result_file = ""
    list_only = False
    fail_fast = False
    benchmark_name = ""
    extra_args: List[str] = []

    i = 0
    while i < len(argv):
        arg = argv[i]
        if arg == "--out-dir":
            if i + 1 >= len(argv):
                print_err("error: --out-dir requires a value")
                return 2
            if not bin_dir_set_explicitly:
                bin_dir = str(Path(argv[i + 1]) / "bin")
            i += 2
            continue
        if arg == "--bin-dir":
            if i + 1 >= len(argv):
                print_err("error: --bin-dir requires a value")
                return 2
            bin_dir = argv[i + 1]
            bin_dir_set_explicitly = True
            i += 2
            continue
        if arg == "--result-file":
            if i + 1 >= len(argv):
                print_err("error: --result-file requires a value")
                return 2
            result_file = argv[i + 1]
            i += 2
            continue
        if arg == "--benchmark":
            if i + 1 >= len(argv):
                print_err("error: --benchmark requires a value")
                return 2
            benchmark_name = argv[i + 1]
            i += 2
            continue
        if arg == "--list":
            list_only = True
            i += 1
            continue
        if arg == "--fail-fast":
            fail_fast = True
            i += 1
            continue
        if arg in ("-h", "--help"):
            print(usage(), end="")
            return 0
        if arg == "--":
            extra_args = argv[i + 1 :]
            break

        print_err(f"error: unknown option '{arg}'")
        print(usage(), end="", file=sys.stderr)
        return 2

    if not bin_dir:
        print_err("error: please specify --out-dir or --bin-dir")
        print(usage(), end="", file=sys.stderr)
        return 2

    bin_dir_path = Path(bin_dir)
    if not bin_dir_path.is_dir():
        print_err(f"error: benchmark bin directory does not exist: {bin_dir}")
        return 2

    if result_file:
        result_path = Path(result_file)
        result_path.parent.mkdir(parents=True, exist_ok=True)
        result_path.write_text("", encoding="utf-8")

    benchmark_bins = discover_binaries(bin_dir_path)
    if not benchmark_bins:
        print_err(f"error: no benchmark binaries found under {bin_dir}")
        return 1

    if benchmark_name:
        candidates = [
            benchmark_name,
            f"lsp_{benchmark_name}",
            f"{benchmark_name}_perf",
            f"lsp_{benchmark_name}_perf",
        ]
        filtered = [name for name in benchmark_bins if name in candidates]
        if not filtered:
            print_err(f"error: benchmark not found: {benchmark_name}")
            print_err("available benchmarks:")
            for name in benchmark_bins:
                print_err(f"  {name}")
            return 1
        benchmark_bins = filtered

    if list_only:
        for name in benchmark_bins:
            print(name)
        return 0

    print(f"Benchmark bin dir: {bin_dir}")
    print(f"Benchmark count: {len(benchmark_bins)}")
    if extra_args:
        print(f"Extra args: {' '.join(extra_args)}")
    if result_file:
        print(f"Result file: {result_file}")

    passed = 0
    failed = 0
    failed_list: List[str] = []

    for index, bin_name in enumerate(benchmark_bins, start=1):
        bin_path = str(bin_dir_path / bin_name)
        print()
        print(f"==> [{index}/{len(benchmark_bins)}] {bin_name}")
        retcode, output = run_and_tee([bin_path, *extra_args])

        if retcode == 0:
            if "ERROR OCCURRED" in output:
                failed += 1
                failed_list.append(bin_name)
            else:
                passed += 1
        else:
            failed += 1
            failed_list.append(bin_name)

        if result_file:
            with open(result_file, "a", encoding="utf-8") as fp:
                fp.write(f"==> [{index}/{len(benchmark_bins)}] {bin_name}\n")
                fp.write(output)
                fp.write("\n")

        if failed > 0 and fail_fast:
            print("fail-fast enabled, stopping.")
            break

    print()
    print("===== Summary =====")
    print(f"Passed: {passed}")
    print(f"Failed: {failed}")
    if result_file:
        print(f"Log file: {result_file}")

    if failed > 0:
        print("Failed benchmarks:")
        for name in failed_list:
            print(f"  {name}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
