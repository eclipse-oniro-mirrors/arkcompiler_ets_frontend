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

#include "importExpression.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {
void ImportExpression::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    if (auto *transformedNode = cb(source_); source_ != transformedNode) {
        source_->SetTransformedNode(transformationName, transformedNode);
        source_ = transformedNode->AsExpression();
    }
}

void ImportExpression::Iterate(const NodeTraverser &cb) const
{
    cb(source_);
}

void ImportExpression::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ImportExpression"}, {"source", source_}});
}

void ImportExpression::Dump(ir::SrcDumper *dumper) const
{
    dumper->Add("ImportExpression");
}

void ImportExpression::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void ImportExpression::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}
checker::Type *ImportExpression::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType ImportExpression::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

ImportExpression *ImportExpression::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const source = source_ != nullptr ? source_->Clone(allocator, nullptr)->AsExpression() : nullptr;
    auto *const clone = allocator->New<ImportExpression>(source);
    ES2PANDA_ASSERT(clone != nullptr);

    if (source != nullptr) {
        source->SetParent(clone);
    }

    ES2PANDA_ASSERT(clone != nullptr);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    clone->SetRange(Range());
    return clone;
}
}  // namespace ark::es2panda::ir
