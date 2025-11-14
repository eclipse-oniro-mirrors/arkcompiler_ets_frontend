/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "util/es2pandaMacros.h"
#include "util/diagnosticEngine.h"
#include "util/options.h"
#include "parser/program/program.h"

namespace ark::es2panda {
lexer::SourcePosition GetPositionForDiagnostic()
{
    return lexer::SourcePosition {};
}

void CompilerBugAction(const lexer::SourcePosition &position)
{
    if (g_diagnosticEngine != nullptr) {
        g_diagnosticEngine->FlushDiagnostic();
    }

    std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~";
    std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~";
    std::cerr << std::endl;

    std::cerr << "es2panda unexpectedly terminated";
    auto program = position.Program();
    if (program != nullptr) {
        auto loc = position.ToLocation();
        std::cerr << " during processing [" << util::BaseName(program->SourceFilePath().Utf8());
        std::cerr << ":" << loc.line << ":" << loc.col << "]";
    }
    std::cerr << "." << std::endl;

    std::cerr << "PLEASE submit a bug report to ";
    std::cerr << "https://gitcode.com/openharmony/arkcompiler_ets_frontend/issues";
    std::cerr << " and include the crash backtrace, source code and associated run script. ";
    std::cerr << std::endl;
}
}  // namespace ark::es2panda
