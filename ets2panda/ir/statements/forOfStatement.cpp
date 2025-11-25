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

#include "forOfStatement.h"

#include "checker/TSchecker.h"
#include "compiler/core/pandagen.h"
#include "compiler/core/ETSGen.h"

namespace ark::es2panda::ir {

checker::Type *ForOfStatement::CreateUnionIteratorTypes(checker::ETSChecker *checker, checker::Type *exprType)
{
    std::vector<checker::Type *> types;

    for (auto it : exprType->AsETSUnionType()->ConstituentTypes()) {
        if (it->IsETSStringType()) {
            types.emplace_back(checker->GlobalCharBuiltinType());
        } else if (it->IsETSObjectType()) {
            types.emplace_back(this->CheckIteratorMethodForObject(checker, it->AsETSObjectType()));
        } else if (it->IsETSArrayType()) {
            types.emplace_back(it->AsETSArrayType()->ElementType()->Clone(checker));
            types.back()->RemoveTypeFlag(checker::TypeFlag::CONSTANT);
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
    auto const &position = right_->Start();

    checker::PropertySearchFlags searchFlag = checker::PropertySearchFlags::SEARCH_METHOD;
    searchFlag |= checker::PropertySearchFlags::SEARCH_IN_BASE | checker::PropertySearchFlags::SEARCH_IN_INTERFACES;
    // NOTE: maybe we need to exclude static methods: search_flag &= ~(checker::PropertySearchFlags::SEARCH_STATIC)

    if (sourceType->HasTypeFlag(checker::TypeFlag::GENERIC)) {
        searchFlag |= checker::PropertySearchFlags::SEARCH_ALL;
    }

    auto *const method = sourceType->GetProperty(compiler::Signatures::ITERATOR_METHOD, searchFlag);
    if (method == nullptr || !method->HasFlag(varbinder::VariableFlags::METHOD)) {
        checker->LogError(diagnostic::MISSING_ITERATOR_METHOD, {}, position);
        return checker->GlobalTypeError();
    }
    if (method->TsType() != nullptr && method->TsType()->IsETSFunctionType()) {
        sourceType = method->TsType()->AsETSFunctionType()->CallSignatures()[0]->Owner();
    }
    ArenaVector<Expression *> arguments {checker->Allocator()->Adapter()};
    auto &signatures = checker->GetTypeOfVariable(method)->AsETSFunctionType()->CallSignatures();
    checker::Signature *signature =
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        checker->MatchOrderSignatures(signatures, arguments, right_, checker::TypeRelationFlag::NO_THROW, "iterator");
    if (signature == nullptr) {
        checker->LogError(diagnostic::MISSING_ITERATOR_METHOD_WITH_SIG, {}, position);
        return checker->GlobalTypeError();
    }
    checker->ValidateSignatureAccessibility(sourceType, signature, position, {{diagnostic::INVISIBLE_ITERATOR, {}}});

    ES2PANDA_ASSERT(signature->Function() != nullptr);

    if (!CheckReturnTypeOfIteratorMethod(checker, sourceType, signature, position)) {
        return checker->GlobalTypeError();
    }

    if (checker->IsClassStaticMethod(sourceType, signature)) {
        checker->LogError(diagnostic::PROP_IS_STATIC, {compiler::Signatures::ITERATOR_METHOD, sourceType->Name()},
                          position);
        return checker->GlobalTypeError();
    }

    auto *const nextMethod =
        signature->ReturnType()->AsETSObjectType()->GetProperty(ITERATOR_INTERFACE_METHOD, searchFlag);
    if (nextMethod == nullptr || !nextMethod->HasFlag(varbinder::VariableFlags::METHOD)) {
        checker->LogError(diagnostic::ITERATOR_MISSING_NEXT, {}, position);
        return checker->GlobalTypeError();
    }

    auto &nextSignatures = checker->GetTypeOfVariable(nextMethod)->AsETSFunctionType()->CallSignatures();
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto const *const nextSignature = checker->MatchOrderSignatures(nextSignatures, arguments, right_,
                                                                    checker::TypeRelationFlag::NO_THROW, "iterator");
    if (nextSignature != nullptr && nextSignature->ReturnType()->IsETSObjectType()) {
        if (auto const *const resultType = nextSignature->ReturnType()->AsETSObjectType();
            resultType->Name().Is(ITERATOR_RESULT_NAME)) {
            return resultType->TypeArguments()[0];
        }
    }

    return checker->GlobalTypeError();
}

bool ForOfStatement::CheckReturnTypeOfIteratorMethod(checker::ETSChecker *checker, checker::ETSObjectType *sourceType,
                                                     checker::Signature *signature,
                                                     const lexer::SourcePosition &position)
{
    if ((signature->ReturnType() == nullptr || signature->ReturnType() == checker->GlobalVoidType()) &&
        signature->Function()->HasBody() && signature->Function()->Body()->IsBlockStatement()) {
        for (auto *const it : signature->Function()->Body()->AsBlockStatement()->Statements()) {
            if (it->IsReturnStatement()) {
                checker::SavedCheckerContext savedContext(checker, checker::CheckerStatus::IN_CLASS, sourceType);
                it->AsReturnStatement()->Check(checker);
                break;
            }
        }
    }

    if (signature->ReturnType() != nullptr && signature->ReturnType()->IsETSObjectType() &&
        ForOfStatement::CheckIteratorInterfaceForObject(checker, signature->ReturnType()->AsETSObjectType())) {
        return true;
    }

    checker->LogError(diagnostic::ITERATOR_DOESNT_RETURN_ITERABLE, {}, position);
    return false;
}

bool ForOfStatement::CheckIteratorInterfaceForObject(checker::ETSChecker *checker, checker::ETSObjectType *obj)
{
    if (obj->Name().Is(ITERATOR_INTERFACE_NAME)) {
        return true;
    }

    for (auto *const it : obj->Interfaces()) {
        if (it->Name().Is(ITERATOR_INTERFACE_NAME)) {
            return true;
        }
    }

    return obj->SuperType() != nullptr && obj->SuperType()->IsETSObjectType() &&
           CheckIteratorInterfaceForObject(checker, obj->SuperType()->AsETSObjectType());
}

checker::Type *ForOfStatement::CheckIteratorMethod(checker::ETSChecker *const checker)
{
    if (auto *exprType = right_->TsType(); exprType != nullptr) {
        if (exprType->IsETSObjectType()) {
            return CheckIteratorMethodForObject(checker, exprType->AsETSObjectType());
        }

        if (exprType->IsETSUnionType()) {
            return this->CreateUnionIteratorTypes(checker, exprType);
        }
    }

    return checker->GlobalTypeError();
}
}  // namespace ark::es2panda::ir
