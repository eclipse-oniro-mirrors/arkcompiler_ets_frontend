/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_UTIL_GENERATEBIN_H
#define ES2PANDA_UTIL_GENERATEBIN_H

#include "parser/program/program.h"

namespace ark::es2panda::util {

class Options;
using ReporterFun = std::function<void(const diagnostic::DiagnosticKind &kind, const DiagnosticMessageParams &params)>;

int GenerateBinaryFile(ark::pandasm::Program *prog, const std::string &output, const util::Options &options,
                       const ReporterFun &reporter);
// CC-OFFNXT(G.NAM.03-CPP) project code style
int GenerateBinaryFiles(std::unordered_map<std::string, std::unique_ptr<ark::pandasm::Program>> &progs,
                        const util::Options &options, const ReporterFun &reporter);
}  // namespace ark::es2panda::util

#endif  // ES2PANDA_UTIL_GENERATEBIN_H
