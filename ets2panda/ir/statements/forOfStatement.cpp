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

#include "forOfStatement.h"

#include "checker/TSchecker.h"
#include "checker/types/ets/etsTupleType.h"
#include "checker/types/ets/etsUnionType.h"
#include "compiler/core/pandagen.h"
#include "compiler/core/ETSGen.h"

namespace ark::es2panda::ir {

checker::Type *ForOfStatement::CreateUnionIteratorTypes(checker::ETSChecker *checker, checker::ETSUnionType *exprType)
{
    std::vector<checker::Type *> types {};

    for (checker::Type *&it : const_cast<ArenaVector<checker::Type *> &>(exprType->ConstituentTypes())) {
        if (it->IsETSStringType()) {
            types.emplace_back(checker->GlobalBuiltinETSStringType());
        } else if (it->IsETSObjectType()) {
            types.emplace_back(this->CheckIteratorMethodForObject(checker, it->AsETSObjectType()));
        } else if (it->IsETSArrayType()) {
            types.emplace_back(it->AsETSArrayType()->ElementType()->Clone(checker));
            types.back()->RemoveTypeFlag(checker::TypeFlag::CONSTANT);
        } else if (it->IsETSTupleType()) {
            types.emplace_back(checker->GlobalETSAnyType());
            it = it->AsETSTupleType()->GetWrapperType();
        } else {
            return checker->GlobalTypeError();
        }
    }

    return checker->CreateETSUnionType(std::move(types));
}

void ForOfStatement::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    if (auto *transformedNode = cb(left_); left_ != transformedNode) {
        left_->SetTransformedNode(transformationName, transformedNode);
        left_ = transformedNode;
    }

    if (auto *transformedNode = cb(right_); right_ != transformedNode) {
        right_->SetTransformedNode(transformationName, transformedNode);
        right_ = transformedNode->AsExpression();
    }

    if (auto *transformedNode = cb(body_); body_ != transformedNode) {
        body_->SetTransformedNode(transformationName, transformedNode);
        body_ = transformedNode->AsStatement();
    }
}

void ForOfStatement::Iterate(const NodeTraverser &cb) const
{
    cb(left_);
    cb(right_);
    cb(body_);
}

void ForOfStatement::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ForOfStatement"}, {"await", isAwait_}, {"left", left_}, {"right", right_}, {"body", body_}});
}

void ForOfStatement::Dump(ir::SrcDumper *dumper) const
{
    ES2PANDA_ASSERT(left_ != nullptr);
    ES2PANDA_ASSERT(right_ != nullptr);
    dumper->Add("for ");
    if (isAwait_) {
        dumper->Add("await ");
    }
    dumper->Add("(");
    left_->Dump(dumper);
    dumper->Add(" of ");
    right_->Dump(dumper);
    dumper->Add(") {");
    if (body_ != nullptr) {
        dumper->IncrIndent();
        dumper->Endl();
        body_->Dump(dumper);
        dumper->DecrIndent();
        dumper->Endl();
    }
    dumper->Add("}");
}

void ForOfStatement::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void ForOfStatement::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ForOfStatement::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType ForOfStatement::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

ForOfStatement *ForOfStatement::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const left = left_ != nullptr ? left_->Clone(allocator, nullptr) : nullptr;
    auto *const right = right_ != nullptr ? right_->Clone(allocator, nullptr)->AsExpression() : nullptr;
    auto *const body = body_ != nullptr ? body_->Clone(allocator, nullptr)->AsStatement() : nullptr;
    auto *const clone = allocator->New<ForOfStatement>(left, right, body, isAwait_);
    ES2PANDA_ASSERT(clone != nullptr);

    if (left != nullptr) {
        left->SetParent(clone);
    }

    if (right != nullptr) {
        right->SetParent(clone);
    }

    if (body != nullptr) {
        body->SetParent(clone);
    }

    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    clone->SetRange(Range());
    return clone;
}

checker::Type *ForOfStatement::CheckIteratorMethodForObject(checker::ETSChecker *checker,
                                                            checker::ETSObjectType *sourceType)
{
    return checker->GetElementTypeOfIteratorMethod(sourceType, right_, true);
}

checker::Type *ForOfStatement::CheckIteratorMethod(checker::ETSChecker *const checker)
{
    if (checker::Type *exprType = right_->Check(checker); exprType != nullptr) {
        if (exprType->IsETSObjectType()) {
            return CheckIteratorMethodForObject(checker, exprType->AsETSObjectType());
        }

        if (exprType->IsETSUnionType()) {
            return this->CreateUnionIteratorTypes(checker, exprType->AsETSUnionType());
        }
    }

    return checker->GlobalTypeError();
}
}  // namespace ark::es2panda::ir
