#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (c) 2026 Huawei Device Co., Ltd.
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

"""Build and package the gluegen wrapper."""

import argparse
import os
import shutil
import subprocess
import sys


def copy_tree(source_path, destination_path):
    shutil.copytree(
        source_path,
        destination_path,
        dirs_exist_ok=True,
        symlinks=True,
    )


def copy_file(source_path, destination_path):
    os.makedirs(os.path.dirname(destination_path), exist_ok=True)
    shutil.copy2(source_path, destination_path)


def run_command(command, execution_path):
    process = subprocess.run(
        command,
        cwd=execution_path,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        timeout=300,
    )
    if process.returncode == 0:
        return

    stdout = process.stdout.decode(errors="replace")
    stderr = process.stderr.decode(errors="replace")
    raise RuntimeError(
        "Command failed: {}\nstdout:\n{}\nstderr:\n{}".format(
            " ".join(command),
            stdout,
            stderr,
        )
    )


def clean_node_modules(node_modules_path):
    shutil.rmtree(
        os.path.join(node_modules_path, ".cache"),
        ignore_errors=True,
    )
    for current_path, _, _ in os.walk(node_modules_path, topdown=False):
        if current_path == node_modules_path:
            continue
        if not os.listdir(current_path):
            os.rmdir(current_path)


def build(options):
    work_path = os.path.join(options.work_dir, "gluegen_wrapper_work")
    shutil.rmtree(work_path, ignore_errors=True)
    copy_tree(options.source_path, work_path)
    copy_file(
        options.native_binary,
        os.path.join(work_path, "bin", os.path.basename(options.native_binary)),
    )
    run_command(
        [options.node, options.npm, "run", "build"],
        work_path,
    )
    run_command(
        [
            options.node,
            options.npm,
            "prune",
            "--omit=dev",
            "--ignore-scripts",
            "--no-audit",
            "--no-fund",
        ],
        work_path,
    )
    clean_node_modules(os.path.join(work_path, "node_modules"))
    return work_path


def copy_output(work_path, output_path):
    shutil.rmtree(output_path, ignore_errors=True)

    copy_tree(
        os.path.join(work_path, "dist"),
        os.path.join(output_path, "dist"),
    )
    copy_tree(
        os.path.join(work_path, "node_modules"),
        os.path.join(output_path, "node_modules"),
    )
    copy_file(
        os.path.join(work_path, "package.json"),
        os.path.join(output_path, "package.json"),
    )
    copy_tree(
        os.path.join(work_path, "bin"),
        os.path.join(output_path, "bin"),
    )


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--node", required=True, help="path to Node.js")
    parser.add_argument("--npm", required=True, help="path to the npm CLI")
    parser.add_argument(
        "--source-path",
        required=True,
        help="path to the gluegen wrapper source",
    )
    parser.add_argument(
        "--work-dir",
        required=True,
        help="path to the build work directory",
    )
    parser.add_argument(
        "--output-path",
        required=True,
        help="path to the packaged output",
    )
    parser.add_argument(
        "--native-binary",
        required=True,
        help="path to the staged gluegen native executable",
    )
    return parser.parse_args()


def main():
    options = parse_args()
    work_path = build(options)
    copy_output(work_path, options.output_path)


if __name__ == "__main__":
    sys.exit(main())
