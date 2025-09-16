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

#include "decorator.h"

#include "checker/ETSchecker.h"
#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {
void Decorator::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    if (auto *transformedNode = cb(expr_); expr_ != transformedNode) {
        expr_->SetTransformedNode(transformationName, transformedNode);
        expr_ = transformedNode->AsExpression();
    }
}

void Decorator::Iterate(const NodeTraverser &cb) const
{
    cb(expr_);
}

void Decorator::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "Decorator"}, {"expression", expr_}});
}

void Decorator::Dump(ir::SrcDumper *dumper) const
{
    dumper->Add("Decorator");
}

void Decorator::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void Decorator::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *Decorator::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType Decorator::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

Decorator *Decorator::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const expr = expr_ != nullptr ? expr_->Clone(allocator, nullptr)->AsExpression() : nullptr;
    auto *const clone = allocator->New<Decorator>(expr);
    ES2PANDA_ASSERT(clone != nullptr);

    if (expr != nullptr) {
        expr->SetParent(clone);
    }
    if (parent != nullptr) {
        clone->SetParent(parent);
    }
    return clone;
}
}  // namespace ark::es2panda::ir
