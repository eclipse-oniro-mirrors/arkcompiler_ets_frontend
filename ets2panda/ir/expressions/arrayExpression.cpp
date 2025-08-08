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

#include "arrayExpression.h"
#include <cstddef>

#include "checker/ETSchecker.h"
#include "checker/TSchecker.h"
#include "checker/ets/typeRelationContext.h"
#include "checker/ts/destructuringContext.h"
#include "checker/types/ets/etsTupleType.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {
ArrayExpression::ArrayExpression([[maybe_unused]] Tag const tag, ArrayExpression const &other,
                                 ArenaAllocator *const allocator)
    : AnnotatedExpression(static_cast<AnnotatedExpression const &>(other), allocator), elements_(allocator->Adapter())
{
    SetPreferredType(other.PreferredType());
    isDeclaration_ = other.isDeclaration_;
    trailingComma_ = other.trailingComma_;
    optional_ = other.optional_;

    for (auto *element : other.elements_) {
        elements_.emplace_back(element->Clone(allocator, this)->AsExpression());
    }
}

ArrayExpression *ArrayExpression::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const clone = allocator->New<ArrayExpression>(Tag {}, *this, allocator);
    ES2PANDA_ASSERT(clone);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }
    clone->SetRange(Range());
    return clone;
}

bool ArrayExpression::ConvertibleToArrayPattern()
{
    bool restFound = false;
    bool convResult = true;
    for (auto *it : elements_) {
        switch (it->Type()) {
            case AstNodeType::ARRAY_EXPRESSION: {
                convResult = it->AsArrayExpression()->ConvertibleToArrayPattern();
                break;
            }
            case AstNodeType::SPREAD_ELEMENT: {
                if (!restFound && it == elements_.back() && !trailingComma_) {
                    convResult = it->AsSpreadElement()->ConvertibleToRest(isDeclaration_);
                } else {
                    convResult = false;
                }
                restFound = true;
                break;
            }
            case AstNodeType::OBJECT_EXPRESSION: {
                convResult = it->AsObjectExpression()->ConvertibleToObjectPattern();
                break;
            }
            case AstNodeType::ASSIGNMENT_EXPRESSION: {
                convResult = it->AsAssignmentExpression()->ConvertibleToAssignmentPattern();
                break;
            }
            case AstNodeType::MEMBER_EXPRESSION:
            case AstNodeType::OMITTED_EXPRESSION:
            case AstNodeType::IDENTIFIER:
            case AstNodeType::ARRAY_PATTERN:
            case AstNodeType::OBJECT_PATTERN:
            case AstNodeType::ASSIGNMENT_PATTERN:
            case AstNodeType::REST_ELEMENT: {
                break;
            }
            default: {
                convResult = false;
                break;
            }
        }

        if (!convResult) {
            break;
        }
    }

    SetType(AstNodeType::ARRAY_PATTERN);
    return convResult;
}

void ArrayExpression::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    for (auto *&it : VectorIterationGuard(elements_)) {
        if (auto *transformedNode = cb(it); it != transformedNode) {
            it->SetTransformedNode(transformationName, transformedNode);
            it = transformedNode->AsExpression();
        }
    }

    if (auto *typeAnnotation = TypeAnnotation(); typeAnnotation != nullptr) {
        if (auto *transformedNode = cb(typeAnnotation); typeAnnotation != transformedNode) {
            typeAnnotation->SetTransformedNode(transformationName, transformedNode);
            SetTsTypeAnnotation(static_cast<TypeNode *>(transformedNode));
        }
    }
}

void ArrayExpression::Iterate(const NodeTraverser &cb) const
{
    for (auto *it : VectorIterationGuard(elements_)) {
        cb(it);
    }

    if (TypeAnnotation() != nullptr) {
        cb(TypeAnnotation());
    }
}

void ArrayExpression::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", type_ == AstNodeType::ARRAY_EXPRESSION ? "ArrayExpression" : "ArrayPattern"},
                 {"elements", elements_},
                 {"typeAnnotation", AstDumper::Optional(TypeAnnotation())},
                 {"optional", AstDumper::Optional(optional_)}});
}

void ArrayExpression::Dump(ir::SrcDumper *dumper) const
{
    dumper->Add("[");
    for (auto element : elements_) {
        element->Dump(dumper);
        if (element != elements_.back()) {
            dumper->Add(", ");
        }
    }
    dumper->Add("]");
}

void ArrayExpression::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void ArrayExpression::Compile(compiler::ETSGen *const etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ArrayExpression::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::Type *CheckAssignmentPattern(Expression *it, checker::TSChecker *checker, checker::Type *elementType,
                                      bool &addOptional, checker::ElementFlags &memberFlag)
{
    auto *assignmentPattern = it->AsAssignmentPattern();
    if (assignmentPattern->Left()->IsIdentifier()) {
        const ir::Identifier *ident = assignmentPattern->Left()->AsIdentifier();
        ES2PANDA_ASSERT(ident->Variable());
        varbinder::Variable *bindingVar = ident->Variable();
        checker::Type *initializerType = checker->GetBaseTypeOfLiteralType(assignmentPattern->Right()->Check(checker));
        bindingVar->SetTsType(initializerType);
        elementType = initializerType;
    } else if (assignmentPattern->Left()->IsArrayPattern()) {
        auto savedContext = checker::SavedCheckerContext(checker, checker::CheckerStatus::FORCE_TUPLE);
        auto destructuringContext = checker::ArrayDestructuringContext(
            {checker, assignmentPattern->Left()->AsArrayPattern(), false, true, nullptr, assignmentPattern->Right()});
        destructuringContext.Start();
        elementType = destructuringContext.InferredType();
    } else {
        ES2PANDA_ASSERT(assignmentPattern->Left()->IsObjectPattern());
        auto savedContext = checker::SavedCheckerContext(checker, checker::CheckerStatus::FORCE_TUPLE);
        auto destructuringContext = checker::ObjectDestructuringContext(
            {checker, assignmentPattern->Left()->AsObjectPattern(), false, true, nullptr, assignmentPattern->Right()});
        destructuringContext.Start();
        elementType = destructuringContext.InferredType();
    }

    if (addOptional) {
        memberFlag = checker::ElementFlags::OPTIONAL;
    } else {
        memberFlag = checker::ElementFlags::REQUIRED;
    }
    return elementType;
}

checker::Type *CheckElementPattern(Expression *it, checker::Type *elementType, checker::TSChecker *checker,
                                   bool &addOptional, checker::ElementFlags &memberFlag)
{
    switch (it->Type()) {
        case ir::AstNodeType::REST_ELEMENT: {
            elementType = checker->Allocator()->New<checker::ArrayType>(checker->GlobalAnyType());
            memberFlag = checker::ElementFlags::REST;
            addOptional = false;
            return elementType;
        }
        case ir::AstNodeType::OBJECT_PATTERN: {
            elementType = it->AsObjectPattern()->CheckPattern(checker);
            memberFlag = checker::ElementFlags::REQUIRED;
            addOptional = false;
            return elementType;
        }
        case ir::AstNodeType::ARRAY_PATTERN: {
            elementType = it->AsArrayPattern()->CheckPattern(checker);
            memberFlag = checker::ElementFlags::REQUIRED;
            addOptional = false;
            return elementType;
        }
        case ir::AstNodeType::ASSIGNMENT_PATTERN: {
            return CheckAssignmentPattern(it, checker, elementType, addOptional, memberFlag);
        }
        case ir::AstNodeType::OMITTED_EXPRESSION: {
            elementType = checker->GlobalAnyType();
            memberFlag = checker::ElementFlags::REQUIRED;
            addOptional = false;
            return elementType;
        }
        case ir::AstNodeType::IDENTIFIER: {
            const ir::Identifier *ident = it->AsIdentifier();
            ES2PANDA_ASSERT(ident->Variable());
            elementType = checker->GlobalAnyType();
            ident->Variable()->SetTsType(elementType);
            memberFlag = checker::ElementFlags::REQUIRED;
            addOptional = false;
            return elementType;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

checker::Type *ArrayExpression::CheckPattern(checker::TSChecker *checker)
{
    checker::ObjectDescriptor *desc = checker->Allocator()->New<checker::ObjectDescriptor>(checker->Allocator());
    ArenaVector<checker::ElementFlags> elementFlags(checker->Allocator()->Adapter());
    checker::ElementFlags combinedFlags = checker::ElementFlags::NO_OPTS;
    uint32_t minLength = 0;
    uint32_t index = elements_.size();
    bool addOptional = true;

    for (auto it = elements_.rbegin(); it != elements_.rend(); it++) {
        checker::Type *elementType = nullptr;
        checker::ElementFlags memberFlag = checker::ElementFlags::NO_OPTS;

        elementType = CheckElementPattern(*it, elementType, checker, addOptional, memberFlag);

        util::StringView memberIndex = util::Helpers::ToStringView(checker->Allocator(), index - 1);

        auto *memberVar =
            varbinder::Scope::CreateVar(checker->Allocator(), memberIndex, varbinder::VariableFlags::PROPERTY, *it);

        if (memberFlag == checker::ElementFlags::OPTIONAL) {
            memberVar->AddFlag(varbinder::VariableFlags::OPTIONAL);
        } else {
            minLength++;
        }

        memberVar->SetTsType(elementType);
        elementFlags.push_back(memberFlag);
        desc->properties.insert(desc->properties.begin(), memberVar);

        combinedFlags |= memberFlag;
        index--;
    }

    ES2PANDA_ASSERT(desc);
    const checker::TupleTypeInfo tupleTypeInfo = {combinedFlags, minLength,
                                                  static_cast<uint32_t>(desc->properties.size()), false};
    return checker->CreateTupleType(desc, std::move(elementFlags), tupleTypeInfo);
}

void ArrayExpression::ClearPreferredType()
{
    SetPreferredType(nullptr);
    SetTsType(nullptr);
    for (auto element : Elements()) {
        element->SetTsType(nullptr);
        element->SetAstNodeFlags(ir::AstNodeFlags::NO_OPTS);
        if (element->IsArrayExpression()) {
            element->AsArrayExpression()->ClearPreferredType();
        }
    }
}

void ArrayExpression::CleanCheckInformation()
{
    SetPreferredType(nullptr);
    SetTsType(nullptr);
    for (auto *element : elements_) {
        element->SetAstNodeFlags(ir::AstNodeFlags::NO_OPTS);
        element->CleanCheckInformation();
    }
}

bool ArrayExpression::TrySetPreferredTypeForNestedArrayExpr(checker::ETSChecker *const checker,
                                                            ArrayExpression *const nestedArrayExpr,
                                                            const std::size_t idx) const
{
    auto doesArrayExprFitInTuple = [&checker, &nestedArrayExpr](const checker::Type *const possibleTupleType) {
        return !possibleTupleType->IsETSTupleType() ||
               checker->IsArrayExprSizeValidForTuple(nestedArrayExpr, possibleTupleType->AsETSTupleType());
    };

    if (PreferredType()->IsETSTupleType()) {
        if (idx >= PreferredType()->AsETSTupleType()->GetTupleSize()) {
            return false;
        }
        auto *const typeInTupleAtIdx = PreferredType()->AsETSTupleType()->GetTypeAtIndex(idx);
        nestedArrayExpr->SetPreferredType(typeInTupleAtIdx);

        return doesArrayExprFitInTuple(typeInTupleAtIdx);
    }

    if (PreferredType()->IsETSArrayType()) {
        auto *const arrayElementType = PreferredType()->AsETSArrayType()->ElementType();
        nestedArrayExpr->SetPreferredType(arrayElementType);

        return doesArrayExprFitInTuple(arrayElementType);
    }

    if (PreferredType()->IsETSResizableArrayType()) {
        auto *const arrayElementType = PreferredType()->AsETSObjectType()->TypeArguments()[0];
        if (!doesArrayExprFitInTuple(arrayElementType)) {
            return false;
        }
        nestedArrayExpr->SetPreferredType(arrayElementType);
        return true;
    }

    if (nestedArrayExpr->PreferredType() == nullptr) {
        nestedArrayExpr->SetPreferredType(PreferredType());
    }

    return true;
}

checker::VerifiedType ArrayExpression::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

std::optional<checker::Type *> ArrayExpression::ExtractPossiblePreferredType(checker::Type *type)
{
    ES2PANDA_ASSERT(type);
    if (type->IsETSArrayType() || type->IsETSTupleType() || type->IsETSResizableArrayType()) {
        return std::make_optional(type);
    }

    if (type->IsETSUnionType()) {
        for (checker::Type *const typeOfUnion : type->AsETSUnionType()->ConstituentTypes()) {
            auto possiblePreferredType = ExtractPossiblePreferredType(typeOfUnion);
            if (possiblePreferredType.has_value()) {
                return std::make_optional(possiblePreferredType.value());
            }
        }
    }

    return std::nullopt;
}

void ArrayExpression::SetPreferredTypeBasedOnFuncParam(checker::ETSChecker *checker, checker::Type *param,
                                                       checker::TypeRelationFlag flags)
{
    // NOTE (mmartin): This needs a complete solution
    if (PreferredType() != nullptr) {
        return;
    }

    auto possiblePreferredType = ExtractPossiblePreferredType(param);
    if (!possiblePreferredType.has_value()) {
        return;
    }

    param = possiblePreferredType.value();
    if (param->IsETSTupleType()) {
        SetPreferredType(param);
        return;
    }

    checker::Type *elementType = nullptr;
    if (param->IsETSArrayType()) {
        elementType = param->AsETSArrayType()->ElementType();
    } else {
        elementType = param->AsETSResizableArrayType()->ElementType();
    }

    bool isAssignable = true;

    for (auto *const elem : elements_) {
        elem->SetPreferredType(elementType);
        checker::AssignmentContext assignCtx(checker->Relation(), elem, elem->Check(checker), elementType,
                                             elem->Start(), std::nullopt, checker::TypeRelationFlag::NO_THROW | flags);
        isAssignable &= assignCtx.IsAssignable();
    }

    if (isAssignable) {
        SetPreferredType(param);
    }
}

}  // namespace ark::es2panda::ir
