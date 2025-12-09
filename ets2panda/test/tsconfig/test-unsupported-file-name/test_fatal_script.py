#!/usr/bin/env python3
# coding=utf-8
#
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

import argparse
import logging.config
import os
import json
import subprocess
import logging
import sys
import re


def ensure_exists(path):
    if not os.path.exists(path):
        raise RuntimeError(f'The file {path} cannot be found')


def es2panda_command(es2panda_path, stdlib_path, config_path, file_path):
    return [
        *str(es2panda_path).split(),
        '--stdlib', stdlib_path,
        '--arktsconfig', config_path,
        file_path
    ]


def normalize_output_file_paths(text):
    if not text:
        return text
    
    path_pattern = r'(?:[a-zA-Z]:)?(?:[\\/][^\\/:\s]+)+(?:[\\/][^\\/:\s]+)*'
    
    def replace_absolute_path(match):
        path = match.group(0)
        filename = os.path.basename(path)
        if os.path.isabs(path) or ':' in path:
            return f"[ABS_PATH]/{filename}"
        return path
    
    normalized = re.sub(path_pattern, replace_absolute_path, text)
    return normalized


def compare_test_output(lhs, rhs):
    if lhs.returncode != rhs.get("returncode", 0):
        raise RuntimeError(f"Return code mismatch: expected {rhs.get('returncode', 0)}, got {lhs.returncode}")
    
    # Normalize stdout for comparison
    normalized_stdout = normalize_output_file_paths(lhs.stdout)
    
    if "stdout" in rhs:
        expected_stdout = rhs["stdout"]
        if normalized_stdout != expected_stdout:
            message = [
                "Stdout mismatch:",
                f"Expected: {repr(expected_stdout)}",
                f"Got: {repr(normalized_stdout)}",
                "",
                "Raw stdout for debugging:",
                f"{repr(lhs.stdout)}"
            ]
            raise RuntimeError("\n".join(message))
    
    if "stderr" in rhs:
        normalized_stderr = normalize_output_file_paths(lhs.stderr)
        if normalized_stderr != rhs["stderr"]:
            raise RuntimeError(f"Stderr mismatch\nExpected: {rhs['stderr']}\nGot: {normalized_stderr}")


parser = argparse.ArgumentParser()
parser.add_argument('--es2panda', required=True,
                    help='Path to es2panda executable, could be prefixed')
parser.add_argument('--config', required=True, help='Path to project config')
parser.add_argument('--stdlib', required=True, help='Path to es2panda stdlib')
parser.add_argument('--filepath', required=True, help='Path to ets file')

args = parser.parse_args()

project_dir = os.path.dirname(args.config)
expected_path = os.path.join(project_dir, 'expected.json')

[ensure_exists(f) for f in [
    str(args.es2panda).split()[-1], args.config, expected_path]]

cmd = es2panda_command(args.es2panda, args.stdlib, args.config, args.filepath)

actual = subprocess.run(cmd,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        encoding='utf-8')

with open(expected_path, "r", encoding="utf-8") as expected_file:
    expected = json.load(expected_file)
    compare_test_output(actual, expected)
