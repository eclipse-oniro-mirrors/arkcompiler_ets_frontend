/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LSP_PERF_H
#define LSP_PERF_H

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static std::vector<std::string> CreateTempFile(const std::vector<std::string> &files)
{
    std::vector<std::string> result;
    for (const auto &file : files) {
        std::ifstream inStream(file);
        if (inStream.fail()) {
            std::cerr << "Failed to open input file: " << file << std::endl;
            return result;
        }
        result.push_back(file);
    }
    return result;
}

#endif  // LSP_PERF_H