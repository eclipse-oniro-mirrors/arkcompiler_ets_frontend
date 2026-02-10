#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
    attempt = 0
    while attempt < 3:
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stdin=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=execution_path
        )
        stdout, stderr = proc.communicate(timeout=300)
        if proc.returncode == 0:
            return
        attempt += 1

    if proc.returncode != 0:
        raise Exception(stderr.decode())


def replace_symlink_with_absolute(symlink_path):
    if os.path.islink(symlink_path):
        target = os.readlink(symlink_path)
        symlink_dir = os.path.dirname(symlink_path)
        abs_target = os.path.abspath(os.path.join(symlink_dir, target))

        os.remove(symlink_path)
        os.symlink(abs_target, symlink_path)

        print(f"Replaced {symlink_path} with absolute target: {abs_target}")
    else:
        print("Path is not a symlink.")


def build(options):
    # Copy the source directory to the temporary directory to avoid pollution
    options.build_system_tmp_dir = os.path.join(options.work_dir, 'temp')
    os.makedirs(options.build_system_tmp_dir, exist_ok=True)
    copy_files(options.source_path, options.build_system_tmp_dir)

    # Build and install the bindings
    bindings_dir = os.path.join(options.source_path, '../../bindings/')
    bindings_out = os.path.join(options.build_system_tmp_dir, 'node_modules/@es2panda/bindings')
    run_cmd(['rm', '-rf', bindings_out])
    run_cmd([options.npm, 'run', 'build', '--', '--outDir', os.path.join(bindings_out, 'dist')], bindings_dir)
    copy_files(os.path.join(bindings_dir, 'package.json'), bindings_out, True)

    # Build build system
    build_cmd = [options.npm, 'run', 'build']
    run_cmd(build_cmd, options.build_system_tmp_dir)


def copy_output(options):
    run_cmd(['rm', '-rf', options.output_path])

    copy_files(os.path.join(options.build_system_tmp_dir, 'dist'),
               os.path.join(options.output_path, 'dist'))

    copy_files(os.path.join(options.build_system_tmp_dir, 'node_modules'),
               os.path.join(options.output_path, 'node_modules'))

    copy_files(os.path.join(options.build_system_tmp_dir, 'package.json'),
               os.path.join(options.output_path, 'package.json'), True)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--npm', required=True, help='path to a npm executable')
    parser.add_argument('--source_path', required=True, help='path to build system source')
    parser.add_argument('--work_dir', required=True, help='path to work directory')
    parser.add_argument('--output_path', required=True, help='path to output')
    return parser.parse_args()


def main():
    options = parse_args()
    build(options)
    copy_output(options)


if __name__ == '__main__':
    sys.exit(main())
