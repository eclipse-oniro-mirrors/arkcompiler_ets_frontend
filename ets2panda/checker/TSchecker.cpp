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

#include "TSchecker.h"

#include "ir/statements/blockStatement.h"
#include "parser/program/program.h"
#include "util/options.h"

namespace ark::es2panda::checker {

bool TSChecker::StartChecker([[maybe_unused]] varbinder::VarBinder *varbinder, const util::Options &options)
{
    varbinder->IdentifierAnalysis();
    SetProgram(varbinder->Program());

    if (options.IsDumpAst()) {
        std::cout << Program()->Dump() << std::endl;
    }

    if (options.IsDumpAstOnlySilent()) {
        Program()->DumpSilent();
        return false;
    }

    if (options.IsParseOnly()) {
        return false;
    }

    ES2PANDA_ASSERT(Program()->Ast()->IsProgram());
    Program()->Ast()->Check(this);

    return false;
}

Type *TSChecker::CheckTypeCached(ir::Expression *expr)
{
    if (expr->TsType() == nullptr) {
        expr->SetTsType(expr->Check(this));
    }

    return expr->TsType();
}

}  // namespace ark::es2panda::checker
