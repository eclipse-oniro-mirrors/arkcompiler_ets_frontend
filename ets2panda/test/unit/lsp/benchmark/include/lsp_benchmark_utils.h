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

#ifndef LSP_BENCHMARK_UTILS_H
#define LSP_BENCHMARK_UTILS_H

#include <cstddef>
#include <string>
#include "lspMemoryManager.h"

#ifndef CASES_DIR
#define CASES_DIR "./cases"
#endif

// CC-OFFNXT(G.NAM.01) false positive
std::string MakeCasePath(const std::string &relativeFileName);

// CC-OFFNXT(G.NAM.01) false positive
std::string ReadCaseFile(const std::string &filePath);

inline bool TryResolveAnchorOffset(const std::string &source, const std::string &anchorText, size_t anchorOffset,
                                   size_t &resolvedOffset)
{
    const size_t anchorPos = source.find(anchorText);
    if (anchorPos == std::string::npos) {
        return false;
    }
    resolvedOffset = anchorPos + anchorOffset;
    return true;
}

template <class T, size_t N>
constexpr int LastCaseIndex(const T (&)[N])
{
    static_assert(N > 0U, "Benchmark test case array cannot be empty");
    return static_cast<int>(N - 1U);
}

#endif  // LSP_BENCHMARK_UTILS_H
