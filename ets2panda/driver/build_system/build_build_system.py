#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (c) 2025 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import time
import subprocess
import shutil
import argparse
import sys


def copy_files(source_path, dest_path, is_file=False):
    try:
        if is_file:
            shutil.copy(source_path, dest_path)
        else:
            shutil.copytree(source_path, dest_path, dirs_exist_ok=True,
                            symlinks=True)
    except Exception as err:
        raise Exception("Copy files failed. Error: " + str(err)) from err


def run_cmd(cmd, execution_path=None):
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                            stdin=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            cwd=execution_path)
    stdout, stderr = proc.communicate(timeout=300)
    if proc.returncode != 0:
        raise Exception(stderr.decode())


def build(options):
    options.temp_dir = os.path.join(options.source_path, f"temp{time.time_ns()}")
    options.build_out = os.path.join(options.temp_dir, "dist")
    os.makedirs(options.temp_dir, exist_ok=True)

    build_cmd = [options.npm, 'run', 'build', '--', '--outDir', options.build_out]
    run_cmd(build_cmd, options.source_path)


def copy_output(options):
    run_cmd(['rm', '-rf', options.output_path])

    copy_files(options.build_out,
               os.path.join(options.output_path, 'dist'))

    copy_files(os.path.join(options.source_path, 'node_modules'),
               os.path.join(options.output_path, 'node_modules'))

    copy_files(os.path.join(options.source_path, 'package.json'),
               os.path.join(options.output_path, 'package.json'), True)

    shutil.rmtree(options.temp_dir, ignore_errors=True)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--npm', required=True, help='path to a npm executable')
    parser.add_argument('--source_path', required=True, help='path to build system source')
    parser.add_argument('--output_path', required=True, help='path to output')
    return parser.parse_args()


def main():
    options = parse_args()
    build(options)
    copy_output(options)


if __name__ == '__main__':
    sys.exit(main())
