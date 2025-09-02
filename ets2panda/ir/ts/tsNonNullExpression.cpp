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

#include "tsNonNullExpression.h"

#include "checker/ETSchecker.h"
#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {
void TSNonNullExpression::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    if (auto *transformedNode = cb(expr_); expr_ != transformedNode) {
        expr_->SetTransformedNode(transformationName, transformedNode);
        expr_ = transformedNode->AsExpression();
    }
}

void TSNonNullExpression::Iterate(const NodeTraverser &cb) const
{
    cb(expr_);
}

void TSNonNullExpression::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "TSNonNullExpression"}, {"expression", expr_}});
}

void TSNonNullExpression::Dump(ir::SrcDumper *dumper) const
{
    ES2PANDA_ASSERT(expr_ != nullptr);
    expr_->Dump(dumper);
    dumper->Add("!");
}

void TSNonNullExpression::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void TSNonNullExpression::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *TSNonNullExpression::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType TSNonNullExpression::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

TSNonNullExpression *TSNonNullExpression::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const expr = expr_->Clone(allocator, nullptr)->AsExpression();
    auto *const clone = allocator->New<TSNonNullExpression>(expr);
    expr->SetParent(clone);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }
    clone->SetRange(range_.GetRange());
    return clone;
}

}  // namespace ark::es2panda::ir
