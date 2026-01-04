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

#include "templateLiteral.h"

#include "compiler/core/pandagen.h"
#include "compiler/core/ETSGen.h"
#include "checker/TSchecker.h"
#include "checker/ETSchecker.h"

namespace ark::es2panda::ir {
TemplateLiteral::TemplateLiteral([[maybe_unused]] Tag const tag, TemplateLiteral const &other,
                                 ArenaAllocator *const allocator)
    : Expression(static_cast<Expression const &>(other)),
      quasis_(allocator->Adapter()),
      expressions_(allocator->Adapter())
{
    for (auto *quasy : other.quasis_) {
        quasis_.emplace_back(quasy->Clone(allocator, this));
    }

    for (auto *expression : other.expressions_) {
        expressions_.emplace_back(expression->Clone(allocator, this)->AsExpression());
    }

    multilineString_ = util::StringView(other.multilineString_);
}

TemplateLiteral *TemplateLiteral::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const clone = allocator->New<TemplateLiteral>(Tag {}, *this, allocator);
    ES2PANDA_ASSERT(clone != nullptr);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }
    clone->SetRange(Range());
    return clone;
}

void TemplateLiteral::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    for (auto *&it : VectorIterationGuard(expressions_)) {
        if (auto *transformedNode = cb(it); it != transformedNode) {
            it->SetTransformedNode(transformationName, transformedNode);
            it = transformedNode->AsExpression();
        }
    }

    for (auto *&it : VectorIterationGuard(quasis_)) {
        if (auto *transformedNode = cb(it); it != transformedNode) {
            it->SetTransformedNode(transformationName, transformedNode);
            it = transformedNode->AsTemplateElement();
        }
    }
}

void TemplateLiteral::Iterate(const NodeTraverser &cb) const
{
    for (auto *it : VectorIterationGuard(expressions_)) {
        cb(it);
    }

    for (auto *it : VectorIterationGuard(quasis_)) {
        cb(it);
    }
}

void TemplateLiteral::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "TemplateLiteral"}, {"expressions", expressions_}, {"quasis", quasis_}});
}

void TemplateLiteral::Dump(ir::SrcDumper *dumper) const
{
    dumper->Add("`");
    auto const num = std::max(expressions_.size(), quasis_.size());

    for (std::size_t i = 0U; i < num; i++) {
        if (i < quasis_.size()) {
            quasis_[i]->Dump(dumper);
        }
        if (i < expressions_.size()) {
            dumper->Add("${");
            expressions_[i]->Dump(dumper);
            dumper->Add("}");
        }
    }
    dumper->Add("`");
}

void TemplateLiteral::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

checker::Type *TemplateLiteral::Check([[maybe_unused]] checker::TSChecker *checker)
{
    // NOTE: aszilagyi.
    return checker->GlobalAnyType();
}

void TemplateLiteral::Compile([[maybe_unused]] compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::VerifiedType TemplateLiteral::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

util::StringView TemplateLiteral::GetMultilineString() const
{
    return multilineString_;
}

void TemplateLiteral::CleanCheckInformation()
{
    SetVariable(nullptr);
    SetPreferredType(nullptr);
    SetTsType(nullptr);
    for (auto *it : VectorIterationGuard(expressions_)) {
        it->CleanCheckInformation();
    }

    for (auto *it : VectorIterationGuard(quasis_)) {
        it->CleanCheckInformation();
    }
}
}  // namespace ark::es2panda::ir
