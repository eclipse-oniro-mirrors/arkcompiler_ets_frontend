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

#include "updateExpression.h"

#include "compiler/core/pandagen.h"
#include "compiler/core/ETSGen.h"
#include "checker/TSchecker.h"
#include "checker/ETSchecker.h"

namespace ark::es2panda::ir {
void UpdateExpression::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    if (auto *transformedNode = cb(argument_); argument_ != transformedNode) {
        argument_->SetTransformedNode(transformationName, transformedNode);
        argument_ = transformedNode->AsExpression();
    }
}

void UpdateExpression::Iterate(const NodeTraverser &cb) const
{
    cb(argument_);
}

void UpdateExpression::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "UpdateExpression"}, {"operator", operator_}, {"prefix", prefix_}, {"argument", argument_}});
}

void UpdateExpression::Dump(ir::SrcDumper *dumper) const
{
    ES2PANDA_ASSERT(argument_);
    dumper->Add("(");
    if (prefix_) {
        dumper->Add(TokenToString(operator_));
        argument_->Dump(dumper);
    } else {
        argument_->Dump(dumper);
        dumper->Add(TokenToString(operator_));
    }
    dumper->Add(")");
}

void UpdateExpression::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void UpdateExpression::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *UpdateExpression::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType UpdateExpression::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

UpdateExpression *UpdateExpression::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const argument = argument_ != nullptr ? argument_->Clone(allocator, nullptr)->AsExpression() : nullptr;
    auto *const clone = allocator->New<UpdateExpression>(argument, operator_, prefix_);
    ES2PANDA_ASSERT(clone != nullptr);

    if (argument != nullptr) {
        argument->SetParent(clone);
    }

    ES2PANDA_ASSERT(clone != nullptr);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    clone->SetRange(Range());
    return clone;
}
}  // namespace ark::es2panda::ir
