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

#include "awaitExpression.h"

#include "checker/TSchecker.h"
#include "compiler/core/pandagen.h"
#include "compiler/core/ETSGen.h"

namespace ark::es2panda::ir {
void AwaitExpression::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    if (argument_ != nullptr) {
        if (auto *transformedNode = cb(argument_); argument_ != transformedNode) {
            argument_->SetTransformedNode(transformationName, transformedNode);
            argument_ = transformedNode->AsExpression();
        }
    }
}

void AwaitExpression::Iterate(const NodeTraverser &cb) const
{
    if (argument_ != nullptr) {
        cb(argument_);
    }
}

void AwaitExpression::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "AwaitExpression"}, {"argument", AstDumper::Nullish(argument_)}});
}

void AwaitExpression::Dump(ir::SrcDumper *dumper) const
{
    if (argument_ != nullptr) {
        dumper->Add("(");
        dumper->Add("await ");
        argument_->Dump(dumper);
        dumper->Add(")");
    }
}

void AwaitExpression::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void AwaitExpression::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *AwaitExpression::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType AwaitExpression::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

AwaitExpression *AwaitExpression::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const argument = argument_ != nullptr ? argument_->Clone(allocator, nullptr)->AsExpression() : nullptr;
    auto *const clone = allocator->New<AwaitExpression>(argument);
    ES2PANDA_ASSERT(clone);

    if (argument != nullptr) {
        argument->SetParent(clone);
    }

    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    clone->SetRange(Range());
    return clone;
}
}  // namespace ark::es2panda::ir
