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

"""Stage the native gluegen executable in the wrapper package."""

import argparse
import os
import shutil


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, help="built gluegen executable")
    parser.add_argument("--destination", required=True, help="package bin destination")
    return parser.parse_args()


def main():
    options = parse_args()
    os.makedirs(os.path.dirname(options.destination), exist_ok=True)
    shutil.copy2(options.source, options.destination)


if __name__ == "__main__":
    main()