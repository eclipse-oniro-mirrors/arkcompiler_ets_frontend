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

"""Simultaneous multi-file compile: both units must report ESE0280."""

import argparse
import json
import os
import subprocess
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
from common_output import get_common_output, remove_common_part  # noqa: E402


def ensure_exists(path):
    if not os.path.exists(path):
        raise RuntimeError(f'The file {path} cannot be found')


def es2panda_command(es2panda_path, stdlib_path, arktsconfig_path, output_path):
    return [
        *str(es2panda_path).split(),
        '--opt-level=2',
        '--thread=0',
        '--extension=ets',
        '--simultaneous=true',
        '--stdlib', stdlib_path,
        '--arktsconfig', arktsconfig_path
    ]


def compare_output(lhs, rhs, ignore_parts):
    for k in rhs:
        attr = getattr(lhs, k)
        attr = remove_common_part(attr, ignore_parts, k)
        if attr != rhs[k]:
            message = "\n".join([f'In {k} field',
                                 f'Expected: {rhs[k]!r}',
                                 f'Got: {attr!r}'])
            raise RuntimeError(message)


parser = argparse.ArgumentParser()
parser.add_argument('--es2panda', required=True,
                    help='Path to es2panda executable, could be prefixed')
parser.add_argument('--arktsconfig', required=True, help='Path to arktsconfig')
parser.add_argument('--stdlib', required=True, help='Path to es2panda stdlib')
parser.add_argument('--output', required=True, help='Path to output abc')

args = parser.parse_args()

project_dir = os.path.dirname(os.path.abspath(__file__))
expected_path = os.path.join(project_dir, 'expected.json')

for f in [str(args.es2panda).split()[-1], args.arktsconfig, expected_path]:
    ensure_exists(f)

cmd = es2panda_command(args.es2panda, args.stdlib, args.arktsconfig, args.output)

actual = subprocess.run(cmd,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        encoding='utf-8',
                        check=False)

with open(expected_path, "r", encoding="utf-8") as expected_file:
    expected = json.load(expected_file)
    common_parts = get_common_output(expected_path)
    compare_output(actual, expected, common_parts)
