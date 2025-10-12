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

import argparse
import os
import shutil
import subprocess
import sys
import tarfile


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
    build_cmd = [options.npm, 'run', 'run']
    run_cmd(build_cmd, options.source_path)


def copy_output(options):
    run_cmd(['rm', '-rf', options.output_path])
    copy_files(os.path.join(options.source_path, 'dist'),
               os.path.join(options.output_path, 'dist'))

    copy_files(os.path.join(options.source_path, 'node_modules', 'json5'),
               os.path.join(options.output_path, 'node_modules', 'json5'))

    copy_files(os.path.join(options.source_path, 'package.json'),
               os.path.join(options.output_path, 'package.json'), True)

    if options.current_os == "mingw" :
        copy_files(os.path.join(options.root_out_dir, 'libts_bindings.dll'),
                os.path.join(options.output_path, 'ts_bindings.node'), True)
        copy_files(os.path.join(options.root_out_dir, 'libpublic.dll'),
                os.path.join(options.output_path, 'public.node'), True)

    if options.current_os == "linux" or options.current_os == "mac":
        copy_files(os.path.join(options.root_out_dir, 'ts_bindings.node'),
                os.path.join(options.output_path, 'ts_bindings.node'), True)
        copy_files(os.path.join(options.root_out_dir, 'public.node'),
                os.path.join(options.output_path, 'public.node'), True)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--npm', help='path to a npm exetuable')
    parser.add_argument('--source-path', help='path to build system source')
    parser.add_argument('--output-path', help='path to output')
    parser.add_argument('--current-os', help='current os')
    parser.add_argument('--root-out-dir', help='root_out_dir')

    options = parser.parse_args()
    return options


def main():
    options = parse_args()

    build(options)
    copy_output(options)


if __name__ == '__main__':
    sys.exit(main())
