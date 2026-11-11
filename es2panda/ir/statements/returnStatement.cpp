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

#include "returnStatement.h"

#include <ir/base/methodDefinition.h>
#include <ir/base/scriptFunction.h>
#include <compiler/core/pandagen.h>

#include <ir/astDump.h>
#include <ir/typeNode.h>

namespace panda::es2panda::ir {

void ReturnStatement::Iterate(const NodeTraverser &cb) const
{
    if (argument_) {
        cb(argument_);
    }
}

void ReturnStatement::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ReturnStatement"}, {"argument", AstDumper::Nullable(argument_)}});
}

void ReturnStatement::Compile(compiler::PandaGen *pg) const
{
    if (argument_) {
        argument_->Compile(pg);
    } else {
        pg->LoadConst(this, compiler::Constant::JS_UNDEFINED);
    }

    if (pg->CheckControlFlowChange()) {
        compiler::RegScope rs(pg);
        compiler::VReg res = pg->AllocReg();

        pg->StoreAccumulator(this, res);
        pg->ControlFlowChangeReturn();
        pg->LoadAccumulator(this, res);
    }

    if (argument_) {
        pg->ValidateClassDirectReturn(this);
        pg->ExplicitReturn(this);
    } else {
        pg->ImplicitReturn(this, true);
    }
}

void ReturnStatement::UpdateSelf(const NodeUpdater &cb, [[maybe_unused]] binder::Binder *binder)
{
    if (argument_) {
        argument_ = std::get<ir::AstNode *>(cb(argument_))->AsExpression();
    }
}

}  // namespace panda::es2panda::ir
