/*
 * Copyright (c) 2021 - 2025 Huawei Device Co., Ltd.
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

#include "JSchecker.h"

#include "varbinder/varbinder.h"
#include "parser/program/program.h"
#include "util/options.h"

namespace ark::es2panda::checker {

bool JSChecker::StartChecker([[maybe_unused]] varbinder::VarBinder *varbinder, const util::Options &options)
{
    varbinder->IdentifierAnalysis();

    if (options.IsDumpAst()) {
        std::cout << Program()->Dump() << std::endl;
    }

    if (options.IsDumpAstOnlySilent()) {
        Program()->DumpSilent();
        return false;
    }

    return !options.IsParseOnly();
}

}  // namespace ark::es2panda::checker
