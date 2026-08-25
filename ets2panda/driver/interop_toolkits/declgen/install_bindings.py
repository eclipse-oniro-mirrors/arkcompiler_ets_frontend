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

"""Install @es2panda/bindings into a package's node_modules from source.

Mirrors the bindings install step of build_build_system.py: builds the bindings
TypeScript into <source>/node_modules/@es2panda/bindings/dist, copies
package.json, and copies public.node (produced by the build_bindings GN
target) when present.
"""

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
        help="target package directory whose node_modules receives the bindings",
    )
    parser.add_argument(
        "--bindings-path",
        required=True,
        help="@es2panda/bindings source directory",
    )
    parser.add_argument(
        "--npm",
        required=True,
        help="path to an npm executable",
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


def install(source_path, bindings_path, npm):
    bindings_out = os.path.join(
        source_path, "node_modules", "@es2panda", "bindings"
    )
    shutil.rmtree(bindings_out, ignore_errors=True)

    # Build the bindings TypeScript straight into the target node_modules.
    # tsc creates the output dirs, so bindings_out exists after this step.
    dist_dir = os.path.join(bindings_out, "dist")
    run_command(
        [npm, "run", "build", "--", "--outDir", dist_dir],
        bindings_path,
    )

    shutil.copy2(
        os.path.join(bindings_path, "package.json"),
        os.path.join(bindings_out, "package.json"),
    )

    # public.node is placed into the bindings source dir by the build_bindings
    # GN target; copy it next to the compiled JS so the addon loads at runtime.
    public_node = os.path.join(bindings_path, "public.node")
    if os.path.exists(public_node):
        shutil.copy2(public_node, os.path.join(bindings_out, "public.node"))


def main():
    options = parse_args()
    install(options.source_path, options.bindings_path, options.npm)
    return 0


if __name__ == "__main__":
    sys.exit(main())
