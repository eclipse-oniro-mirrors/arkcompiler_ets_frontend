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
import os
import json
import subprocess


def ensure_exists(path):
    if not os.path.exists(path):
        raise RuntimeError(f'The file {path} cannot be found')


def es2panda_command(es2panda_path, stdlib_path, arktsconfig_path, target_path):
    return [
        *str(es2panda_path).split(),
        '--opt-level=2',
        '--thread=0', 
        '--extension=ets',
        '--stdlib', stdlib_path,
        '--arktsconfig', arktsconfig_path,
        '--ets-unnamed',
        target_path
    ]


def compare_output(lhs, rhs):
    for k in rhs:
        attr = getattr(lhs, k)
        if attr != rhs[k]:
            message = "\n".join([f'In {k} field',
                                 f'Expected: {rhs[k]}',
                                 f'Got: {attr}'])
            raise RuntimeError(message)


parser = argparse.ArgumentParser()
parser.add_argument('--es2panda', required=True,
                    help='Path to es2panda executable, could be prefixed')
parser.add_argument('--arktsconfig', required=True, help='Path to project arktsconfig')
parser.add_argument('--stdlib', required=True, help='Path to es2panda stdlib')
parser.add_argument('--target', required=True, help='Path to .sts to compile it to .abc')

args = parser.parse_args()

project_dir = os.path.dirname(args.target)
expected_path = os.path.join(project_dir, 'expected.json')

[ensure_exists(f) for f in [
    str(args.es2panda).split()[-1], args.arktsconfig, expected_path]]

cmd = es2panda_command(args.es2panda, args.stdlib, args.arktsconfig, args.target)


actual = subprocess.run(cmd,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        encoding='utf-8')

with open(expected_path, "r", encoding="utf-8") as expected_file:
    expected = json.load(expected_file)
    compare_output(actual, expected)
