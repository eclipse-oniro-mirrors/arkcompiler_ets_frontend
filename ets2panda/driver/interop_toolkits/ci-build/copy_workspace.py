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

"""Copy the complete interop toolkits source directory into a build output."""

import argparse
import os
import shutil


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, help="interop toolkits source directory")
    parser.add_argument("--destination", required=True, help="destination directory")
    return parser.parse_args()


def copy_contents(source, destination):
    shutil.rmtree(destination, ignore_errors=True)
    os.makedirs(destination, exist_ok=True)

    for name in os.listdir(source):
        source_path = os.path.join(source, name)
        destination_path = os.path.join(destination, name)
        if os.path.isdir(source_path):
            shutil.copytree(source_path, destination_path, symlinks=True)
        else:
            shutil.copy2(source_path, destination_path, follow_symlinks=False)


def main():
    options = parse_args()
    copy_contents(options.source, options.destination)


if __name__ == "__main__":
    main()
