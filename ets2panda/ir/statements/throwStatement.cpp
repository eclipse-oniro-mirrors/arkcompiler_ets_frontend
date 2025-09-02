/**
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "throwStatement.h"

#include "checker/TSchecker.h"
#include "compiler/core/pandagen.h"
#include "compiler/core/ETSGen.h"

namespace ark::es2panda::ir {
void ThrowStatement::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    if (auto *transformedNode = cb(argument_); argument_ != transformedNode) {
        argument_->SetTransformedNode(transformationName, transformedNode);
        argument_ = transformedNode->AsExpression();
    }
}

void ThrowStatement::Iterate(const NodeTraverser &cb) const
{
    cb(argument_);
}

void ThrowStatement::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ThrowStatement"}, {"argument", argument_}});
}

void ThrowStatement::Dump(ir::SrcDumper *dumper) const
{
    ES2PANDA_ASSERT(Argument() != nullptr);
    dumper->Add("throw ");
    Argument()->Dump(dumper);
    dumper->Add(";");
}

void ThrowStatement::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void ThrowStatement::Compile([[maybe_unused]] compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ThrowStatement::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType ThrowStatement::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

ThrowStatement *ThrowStatement::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const expression = argument_->Clone(allocator, nullptr)->AsExpression();
    auto *const clone = allocator->New<ThrowStatement>(expression);
    expression->SetParent(clone);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }
    clone->SetRange(range_.GetRange());
    return clone;
}
}  // namespace ark::es2panda::ir
