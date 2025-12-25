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

#include "tsQualifiedName.h"

#include "checker/ETSchecker.h"
#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {
TSQualifiedName::TSQualifiedName(Expression *left, Identifier *right, [[maybe_unused]] ArenaAllocator *allocator)
    : Expression(AstNodeType::TS_QUALIFIED_NAME), left_(left), right_(right)
{
    ES2PANDA_ASSERT(left_ != nullptr && right_ != nullptr);
}

TSQualifiedName::TSQualifiedName([[maybe_unused]] Tag const tag, TSQualifiedName const &other,
                                 ArenaAllocator *allocator)
    : Expression(static_cast<Expression const &>(other))
{
    left_ = other.left_->Clone(allocator, this)->AsExpression();
    right_ = other.right_->Clone(allocator, this)->AsIdentifier();
}

void TSQualifiedName::Iterate(const NodeTraverser &cb) const
{
    cb(left_);
    cb(right_);
}

void TSQualifiedName::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    if (auto *transformedNode = cb(left_); left_ != transformedNode) {
        left_->SetTransformedNode(transformationName, transformedNode);
        left_ = transformedNode->AsExpression();
    }

    if (auto *transformedNode = cb(right_); right_ != transformedNode) {
        right_->SetTransformedNode(transformationName, transformedNode);
        right_ = transformedNode->AsIdentifier();
    }
}

void TSQualifiedName::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "TSQualifiedName"}, {"left", left_}, {"right", right_}});
}

void TSQualifiedName::Dump(ir::SrcDumper *dumper) const
{
    left_->Dump(dumper);
    dumper->Add(".");
    right_->Dump(dumper);
}

void TSQualifiedName::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}
void TSQualifiedName::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *TSQualifiedName::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType TSQualifiedName::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

util::StringView TSQualifiedName::Name() const
{
    EAllocator fakeAlloc;
    util::UString packageName(&fakeAlloc);

    const auto *iter = this;

    while (iter->Left()->IsTSQualifiedName()) {
        iter = iter->Left()->AsTSQualifiedName();
    }

    packageName.Append(iter->Left()->AsIdentifier()->Name());

    const ir::AstNode *parent = iter;

    while (parent != nullptr && parent->IsTSQualifiedName()) {
        packageName.Append('.');
        packageName.Append(parent->AsTSQualifiedName()->Right()->AsIdentifier()->Name());
        parent = parent->Parent();
    }

    return packageName.View();
}

template <typename T>
static T ResolveLeftMostQualifiedNameImpl(T self)
{
    auto *iter = self;

    while (iter->Left()->IsTSQualifiedName()) {
        iter = iter->Left()->AsTSQualifiedName();
    }

    return iter;
}

ir::TSQualifiedName *TSQualifiedName::ResolveLeftMostQualifiedName()
{
    return ResolveLeftMostQualifiedNameImpl(this);
}

const ir::TSQualifiedName *TSQualifiedName::ResolveLeftMostQualifiedName() const
{
    return ResolveLeftMostQualifiedNameImpl(this);
}

TSQualifiedName *TSQualifiedName::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const clone = allocator->New<TSQualifiedName>(Tag {}, *this, allocator);
    ES2PANDA_ASSERT(clone != nullptr);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }
    clone->SetRange(Range());
    return clone;
}
}  // namespace ark::es2panda::ir
