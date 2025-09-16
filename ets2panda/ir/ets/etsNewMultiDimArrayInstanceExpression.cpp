/*
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

#include "etsNewMultiDimArrayInstanceExpression.h"

#include "checker/ets/typeRelationContext.h"
#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {
void ETSNewMultiDimArrayInstanceExpression::TransformChildren(const NodeTransformer &cb,
                                                              std::string_view const transformationName)
{
    if (auto *transformedNode = cb(typeReference_); typeReference_ != transformedNode) {
        typeReference_->SetTransformedNode(transformationName, transformedNode);
        typeReference_ = static_cast<TypeNode *>(transformedNode);
    }

    for (auto *&dim : dimensions_) {
        if (auto *transformedNode = cb(dim); dim != transformedNode) {
            dim->SetTransformedNode(transformationName, transformedNode);
            dim = transformedNode->AsExpression();
        }
    }
}

void ETSNewMultiDimArrayInstanceExpression::Iterate(const NodeTraverser &cb) const
{
    cb(typeReference_);
    for (auto *dim : dimensions_) {
        cb(dim);
    }
}

void ETSNewMultiDimArrayInstanceExpression::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ETSNewMultiDimArrayInstanceExpression"},
                 {"typeReference", typeReference_},
                 {"dimensions", dimensions_}});
}

void ETSNewMultiDimArrayInstanceExpression::Dump(ir::SrcDumper *dumper) const
{
    dumper->Add("new ");
    ES2PANDA_ASSERT(typeReference_);
    typeReference_->Dump(dumper);
    for (auto dim : dimensions_) {
        dumper->Add("[");
        dim->Dump(dumper);
        dumper->Add("]");
    }
}

void ETSNewMultiDimArrayInstanceExpression::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}
void ETSNewMultiDimArrayInstanceExpression::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ETSNewMultiDimArrayInstanceExpression::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType ETSNewMultiDimArrayInstanceExpression::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

void ETSNewMultiDimArrayInstanceExpression::ClearPreferredType()
{
    SetPreferredType(nullptr);
    SetTsType(nullptr);
}

void ETSNewMultiDimArrayInstanceExpression::CleanCheckInformation()
{
    SetPreferredType(nullptr);
    SetTsType(nullptr);
    for (auto *dim : dimensions_) {
        dim->CleanCheckInformation();
    }
}

ETSNewMultiDimArrayInstanceExpression::ETSNewMultiDimArrayInstanceExpression(
    ETSNewMultiDimArrayInstanceExpression const &other, ArenaAllocator *const allocator)
    : Expression(static_cast<Expression const &>(other)),
      dimensions_(allocator->Adapter()),
      signature_(other.signature_)
{
    typeReference_ = other.typeReference_->Clone(allocator, this);

    for (auto *const dimension : other.dimensions_) {
        dimensions_.emplace_back(dimension->Clone(allocator, this)->AsExpression());
    }
}

ETSNewMultiDimArrayInstanceExpression *ETSNewMultiDimArrayInstanceExpression::Clone(ArenaAllocator *const allocator,
                                                                                    AstNode *const parent)
{
    auto *const clone = allocator->New<ETSNewMultiDimArrayInstanceExpression>(*this, allocator);
    ES2PANDA_ASSERT(clone);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }
    clone->SetRange(Range());
    return clone;
}

void ETSNewMultiDimArrayInstanceExpression::SetPreferredTypeBasedOnFuncParam(checker::ETSChecker *checker,
                                                                             checker::Type *param,
                                                                             checker::TypeRelationFlag flags)
{
    // NOTE (mmartin): This needs a complete solution
    if (PreferredType() != nullptr) {
        return;
    }
    if (!param->IsETSArrayType()) {
        return;
    }
    checker::Type *elementType = param->AsETSArrayType();

    for (size_t i = 0; i < dimensions_.size(); i++) {
        if (!elementType->IsETSArrayType()) {
            return;
        }
        elementType = elementType->AsETSArrayType()->ElementType();
    }
    auto assignCtx =
        checker::AssignmentContext(checker->Relation(), typeReference_, typeReference_->GetType(checker), elementType,
                                   typeReference_->Start(), std::nullopt, checker::TypeRelationFlag::NO_THROW | flags);
    if (assignCtx.IsAssignable()) {
        SetPreferredType(param);
    }
}
}  // namespace ark::es2panda::ir
