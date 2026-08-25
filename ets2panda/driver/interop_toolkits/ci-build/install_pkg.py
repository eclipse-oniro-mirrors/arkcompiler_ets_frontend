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

"""Install a local npm tarball into a package's node_modules offline.

The tarball produced by ``npm pack`` extracts to a ``package/`` directory.
This script reads the package name from that tarball and puts the extracted
content under ``<source-path>/node_modules/<name>`` so that the target package
can resolve it without running ``npm install`` (no network needed).
"""

import argparse
import json
import os
import shutil
import sys
import tarfile

COMMAND_TIMEOUT = 600


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--source-path",
        required=True,
        help="target package directory whose node_modules receives the tarball",
    )
    parser.add_argument(
        "--pkg",
        required=True,
        help="path to the npm tarball (.tgz) to install",
    )
    return parser.parse_args()


def read_package_name(pkg_path):
    with tarfile.open(pkg_path, "r:gz") as tar:
        member = tar.getmember("package/package.json")
        extracted = tar.extractfile(member)
        return json.load(extracted)["name"]


def install(source_path, pkg_path):
    package_name = read_package_name(pkg_path)
    install_dir = os.path.join(
        source_path, "node_modules", *package_name.split("/")
    )
    os.makedirs(os.path.dirname(install_dir), exist_ok=True)
    shutil.rmtree(install_dir, ignore_errors=True)

    staging = install_dir + ".staging"
    shutil.rmtree(staging, ignore_errors=True)
    with tarfile.open(pkg_path, "r:gz") as tar:
        tar.extractall(staging)
    shutil.move(os.path.join(staging, "package"), install_dir)
    shutil.rmtree(staging, ignore_errors=True)


def main():
    options = parse_args()
    install(options.source_path, options.pkg)
    return 0


if __name__ == "__main__":
    sys.exit(main())
