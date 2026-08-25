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

"""Build an npm package and optionally pack the result into a tarball."""

import argparse
import os
import shutil
import subprocess
import sys

COMMAND_TIMEOUT = 600


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--source-path",
        required=True,
        help="npm package source directory to build",
    )
    parser.add_argument(
        "--node-path",
        required=True,
        help="path to a node executable",
    )
    parser.add_argument(
        "--npm-path",
        required=True,
        help="path to the npm CLI (run via node)",
    )
    parser.add_argument(
        "--pack-path",
        default=None,
        help="final output file path for the npm pack tarball "
        "(e.g. /out/a.tgz); when omitted, no pack is performed",
    )
    return parser.parse_args()


def run_command(command, execution_path):
    process = subprocess.run(
        command,
        cwd=execution_path,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        timeout=COMMAND_TIMEOUT,
    )
    if process.returncode != 0:
        stdout = process.stdout.decode(errors="replace")
        stderr = process.stderr.decode(errors="replace")
        raise RuntimeError(
            "Command failed: {}\nstdout:\n{}\nstderr:\n{}".format(
                " ".join(command),
                stdout,
                stderr,
            )
        )
    return process.stdout.decode(errors="replace")


def find_tarball(pack_output):
    for line in reversed(pack_output.splitlines()):
        line = line.strip()
        if line.endswith(".tgz"):
            return line
    raise RuntimeError(
        "npm pack produced no tarball name:\n{}".format(pack_output)
    )


def build(source_path, node_path, npm_path):
    pack_output = run_command([node_path, npm_path, "run", "build"], source_path)
    print(pack_output)


def pack(source_path, node_path, npm_path, pack_path):
    pack_output = run_command([node_path, npm_path, "pack"], source_path)
    tarball_name = find_tarball(pack_output)
    parent_dir = os.path.dirname(pack_path)
    if parent_dir:
        os.makedirs(parent_dir, exist_ok=True)
    shutil.move(
        os.path.join(source_path, tarball_name),
        pack_path,
    )


def main():
    options = parse_args()
    build(options.source_path, options.node_path, options.npm_path)
    if options.pack_path is not None:
        pack(
            options.source_path,
            options.node_path,
            options.npm_path,
            options.pack_path,
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
