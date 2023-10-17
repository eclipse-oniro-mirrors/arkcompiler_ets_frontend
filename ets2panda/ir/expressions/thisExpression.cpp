/**
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "thisExpression.h"

#include "util/helpers.h"
#include "binder/binder.h"
#include "compiler/core/pandagen.h"
#include "compiler/core/ETSGen.h"
#include "checker/TSchecker.h"
#include "checker/ETSchecker.h"
#include "ir/base/classDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/base/methodDefinition.h"
#include "ir/statements/blockStatement.h"
#include "ir/astDump.h"
#include "ir/expressions/callExpression.h"

namespace panda::es2panda::ir {
void ThisExpression::TransformChildren([[maybe_unused]] const NodeTransformer &cb) {}
void ThisExpression::Iterate([[maybe_unused]] const NodeTraverser &cb) const {}

void ThisExpression::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ThisExpression"}});
}

void ThisExpression::Compile(compiler::PandaGen *pg) const
{
    auto res = pg->Scope()->Find(binder::Binder::MANDATORY_PARAM_THIS);

    ASSERT(res.variable && res.variable->IsLocalVariable());
    pg->LoadAccFromLexEnv(this, res);

    const ir::ScriptFunction *func = util::Helpers::GetContainingConstructor(this);

    if (func != nullptr) {
        pg->ThrowIfSuperNotCorrectCall(this, 0);
    }
}

void ThisExpression::Compile(compiler::ETSGen *etsg) const
{
    etsg->LoadThis(this);
}

checker::Type *ThisExpression::Check(checker::TSChecker *checker)
{
    // TODO(aszilagyi)
    return checker->GlobalAnyType();
}

checker::Type *ThisExpression::Check(checker::ETSChecker *checker)
{
    if (TsType() != nullptr) {
        return TsType();
    }

    SetTsType(checker->CheckThisOrSuperAccess(this, checker->Context().ContainingClass(), "this"));

    if (checker->HasStatus(checker::CheckerStatus::IN_LAMBDA)) {
        checker->Context().AddCapturedVar(checker->Context().ContainingClass()->Variable(), this->Start());
    }

    return TsType();
}
}  // namespace panda::es2panda::ir