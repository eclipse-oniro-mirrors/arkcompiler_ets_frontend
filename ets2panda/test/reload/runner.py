#!/usr/bin/env python3
# coding=utf-8
#
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

"""
ETS Cold/Hot Reload Patch Test Runner.

Usage:
  python3 runner.py <build_dir> [--coldreload] [--hotreload]

  build_dir: path to the ets2panda build directory (contains bin/es2panda).

Each test case is a subdirectory containing:
  base.ets       — original source (Phase 1: dump symbol table)
  base_mod.ets   — modified source (Phase 2: cold/hot reload)
  expected.txt   — expected stderr from reload phase (may be empty)

The runner:
  1. Runs es2panda --dump-symbol-table=<tmp>.st base.ets
  2. Runs es2panda --cold-reload/--hot-reload --input-symbol-table=<tmp>.st base_mod.ets
  3. Compares the reload stderr against expected.txt
"""

import argparse
import glob
import os
import shutil
import subprocess
import sys
import tempfile

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))


def find_test_cases(mode):
    """Find all test case directories under coldreload/ or hotreload/.

    Requires base.ets and base_mod.ets. If expected.txt exists, it defines the
    expected reload stderr (fail test). If absent, the test is positive (no errors
    expected).
    """
    mode_dir = os.path.join(SCRIPT_DIR, mode)
    if not os.path.isdir(mode_dir):
        return []
    cases = []
    for name in sorted(os.listdir(mode_dir)):
        case_dir = os.path.join(mode_dir, name)
        if os.path.isdir(case_dir):
            base = os.path.join(case_dir, 'base.ets')
            mod = os.path.join(case_dir, 'base_mod.ets')
            if not os.path.isfile(base) or not os.path.isfile(mod):
                continue
            expected = os.path.join(case_dir, 'expected.txt')
            expected = expected if os.path.isfile(expected) else None
            cases.append((name, case_dir, base, mod, expected))
    return cases


def read_expected(path):
    """Read expected.txt content."""
    with open(path, 'r') as f:
        return f.read().strip()


def _dump_phase(es2panda, opt_level, tmpdir, case_dir, base, st_file, work_name):
    """Run Phase 1: dump or use pre-made symbol table.
    Returns (output, error_label) — error_label is None on success."""
    pre_st = os.path.join(case_dir, 'base.txt')
    if os.path.isfile(pre_st):
        shutil.copy(pre_st, st_file)
        return ("", None)

    shutil.copy(base, os.path.join(tmpdir, work_name))
    dump_cmd = [es2panda, f'--opt-level={opt_level}',
                '--dump-symbol-table=' + st_file, work_name]
    result = subprocess.run(dump_cmd, capture_output=True, text=True, timeout=60,
                            cwd=tmpdir)
    if result.returncode != 0:
        return (f"DUMP FAILED (exit={result.returncode})\n{result.stderr}", "DUMP FAILED")

    with open(st_file, 'r') as f:
        if not f.read().strip():
            return ("DUMP FAILED: empty symbol table", "DUMP FAILED")
    return ("", None)


def _reload_phase(es2panda, opt_level, mode_flag, tmpdir, st_file, out_abc, work_name, mod):
    """Run Phase 2: reload. Returns (output, None) on success, or (name, False, error, expected) on crash."""
    shutil.copy(mod, os.path.join(tmpdir, work_name))
    reload_cmd = [es2panda, f'--opt-level={opt_level}', '--output=' + out_abc,
                  mode_flag, '--input-symbol-table=' + st_file, work_name]
    result = subprocess.run(reload_cmd, capture_output=True, text=True, timeout=60,
                            cwd=tmpdir)
    if result.returncode < 0:
        return (f"RELOAD CRASHED (signal={-result.returncode})\n{result.stderr}",
                "RELOAD CRASHED")

    output = result.stderr.strip()
    output = output.replace(tmpdir + '/', '')
    if output.endswith(tmpdir):
        output = output[:-len(tmpdir)]
    output = '\n'.join(
        line for line in output.split('\n')
        if not line.startswith('[Warning] Reload mode forces --opt-level=0')
    ).strip()
    return (output, None)


def _check_result(output, expected_path, name, work_name, out_abc):
    """Compare output to expected and check bytecode generation."""
    if expected_path is None:
        passed = (output == '')
        expected = '(no error)'
    else:
        expected = read_expected(expected_path).replace('{NAME}', name)\
                                               .replace('{FILE}', work_name)
        passed = (output == expected)

    if expected_path is None and not os.path.isfile(out_abc):
        passed = False
        output += '\n[ERROR] .abc not generated on success'
    if expected_path is not None and os.path.isfile(out_abc):
        passed = False
        output += '\n[ERROR] .abc generated on failure'
    return (passed, output, expected)


def run_test(es2panda, opt_level, mode, case_dir, base, mod, expected_path):
    name = os.path.basename(case_dir)
    mode_flag = '--cold-reload' if mode == 'coldreload' else '--hot-reload'

    with tempfile.TemporaryDirectory() as tmpdir:
        st_file = os.path.join(tmpdir, 'base.st')
        out_abc = os.path.join(tmpdir, 'out.abc')
        work_name = name + '.ets'

        for extra in glob.glob(os.path.join(case_dir, '*.ets')):
            extra_name = os.path.basename(extra)
            if extra_name not in (os.path.basename(base), os.path.basename(mod)):
                shutil.copy(extra, os.path.join(tmpdir, extra_name))

        dump_out, dump_err = _dump_phase(es2panda, opt_level, tmpdir, case_dir, base, st_file, work_name)
        if dump_err is not None:
            return (name, False, dump_out, dump_err)

        output, reload_err = _reload_phase(es2panda, opt_level, mode_flag, tmpdir,
                                            st_file, out_abc, work_name, mod)
        if reload_err is not None:
            return (name, False, output, reload_err)

        passed, output, expected = _check_result(output, expected_path, name, work_name, out_abc)
        return (name, passed, output, expected)


def resolve_es2panda(args):
    """Resolve the es2panda binary path from CLI arguments."""
    if args.es2panda:
        # CMake may pass a space-separated prefix + path: "prefix /path/to/es2panda"
        path = args.es2panda.split()[-1]
    elif args.build_dir:
        path = os.path.join(args.build_dir, 'bin', 'es2panda')
    else:
        print("ERROR: either --es2panda or build_dir must be specified")
        sys.exit(1)
    if not os.path.isfile(path):
        print(f"ERROR: es2panda binary not found: {path}")
        sys.exit(1)
    return path


def run_suite(es2panda, opt_level, label, mode):
    """Run all test cases for a single mode (coldreload or hotreload).

    Returns (pass_count, fail_count).
    """
    cases = find_test_cases(mode)
    print(f"\n{'='*60}")
    print(f"  {label} ({len(cases)} tests)")
    print(f"{'='*60}")

    npass = 0
    nfail = 0
    for name, case_dir, base, mod, expected_path in cases:
        name, passed, output, expected = run_test(
            es2panda, opt_level, mode, case_dir, base, mod, expected_path)
        status = "PASS" if passed else "FAIL"
        print(f"  [{status}] {name}")
        if passed:
            npass += 1
        else:
            nfail += 1
            if expected:
                print(f"         expected: {repr(expected)}")
            print(f"         got:      {repr(output)}")
    return npass, nfail


def main():
    """Parse arguments and run the selected test suites."""
    parser = argparse.ArgumentParser(description='ETS Cold/Hot Reload Test Runner')
    parser.add_argument('build_dir', nargs='?', default=None,
                        help='ets2panda build directory (contains bin/es2panda)')
    parser.add_argument('--es2panda', default=None, help='Path to es2panda binary')
    parser.add_argument('--opt-level', type=int, default=0,
                        help='Optimization level for es2panda (default: 0)')
    parser.add_argument('--coldreload', action='store_true', help='run cold reload tests')
    parser.add_argument('--hotreload', action='store_true', help='run hot reload tests')
    args = parser.parse_args()

    if not args.coldreload and not args.hotreload:
        args.coldreload = True
        args.hotreload = True

    es2panda = resolve_es2panda(args)

    modes = []
    if args.coldreload:
        modes.append(('Cold Reload', 'coldreload'))
    if args.hotreload:
        modes.append(('Hot Reload', 'hotreload'))

    total_pass = 0
    total_fail = 0
    for label, mode in modes:
        npass, nfail = run_suite(es2panda, args.opt_level, label, mode)
        total_pass += npass
        total_fail += nfail

    print(f"\n{'='*60}")
    print(f"  Results: {total_pass} passed, {total_fail} failed")
    print(f"{'='*60}")
    sys.exit(0 if total_fail == 0 else 1)


if __name__ == '__main__':
    main()
