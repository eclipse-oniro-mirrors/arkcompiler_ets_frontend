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

#include "etsFunctionType.h"

#include "checker/TSchecker.h"
#include "checker/ETSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {
void ETSFunctionType::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    GetHistoryNodeAs<ETSFunctionType>()->signature_.TransformChildren(cb, transformationName);
    TransformAnnotations(cb, transformationName);
}

void ETSFunctionType::Iterate(const NodeTraverser &cb) const
{
    GetHistoryNodeAs<ETSFunctionType>()->signature_.Iterate(cb);
    IterateAnnotations(cb);
}

void ETSFunctionType::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ETSFunctionType"},
                 {"params", Params()},
                 {"typeParameters", AstDumper::Optional(TypeParams())},
                 {"returnType", ReturnType()},
                 {"annotations", AstDumper::Optional(Annotations())}});
}

void ETSFunctionType::Dump(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);
    dumper->Add("((");
    for (auto *param : Params()) {
        param->Dump(dumper);
        if (param != Params().back()) {
            dumper->Add(", ");
        }
    }
    dumper->Add(")");

    if (TypeParams() != nullptr) {
        TypeParams()->Dump(dumper);
    }

    if (ReturnType() != nullptr) {
        dumper->Add("=> ");
        ReturnType()->Dump(dumper);
    }

    dumper->Add(")");
}

void ETSFunctionType::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void ETSFunctionType::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ETSFunctionType::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::Type *ETSFunctionType::GetType([[maybe_unused]] checker::TSChecker *checker)
{
    return nullptr;
}

checker::VerifiedType ETSFunctionType::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

checker::Type *ETSFunctionType::GetType(checker::ETSChecker *checker)
{
    return Check(checker);
}

ETSFunctionType *ETSFunctionType::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    ArenaVector<Expression *> paramsClone(allocator->Adapter());

    for (auto *const param : Params()) {
        paramsClone.emplace_back(param->Clone(allocator, nullptr)->AsExpression());
    }

    auto *const typeParamsClone =
        TypeParams() != nullptr ? TypeParams()->Clone(allocator, nullptr)->AsTSTypeParameterDeclaration() : nullptr;
    auto *const returnTypeClone =
        ReturnType() != nullptr ? ReturnType()->Clone(allocator, nullptr)->AsTypeNode() : nullptr;

    auto *const clone = allocator->New<ETSFunctionType>(
        FunctionSignature(typeParamsClone, std::move(paramsClone), returnTypeClone), Flags(), allocator);

    if (typeParamsClone != nullptr) {
        typeParamsClone->SetParent(clone);
    }

    if (returnTypeClone != nullptr) {
        returnTypeClone->SetParent(clone);
    }

    for (auto *param : clone->Params()) {
        param->SetParent(clone);
    }

    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    if (HasAnnotations()) {
        clone->SetAnnotations(Annotations());
    }

    // If the scope is set to empty, it will result in the inability to retrieve the scope after clone,
    // and an error cannot find type will be reported
    clone->SetScope(Scope());

    return clone;
}

ETSFunctionType *ETSFunctionType::Construct(ArenaAllocator *allocator)
{
    auto adapter = allocator->Adapter();
    return allocator->New<ETSFunctionType>(FunctionSignature(nullptr, ArenaVector<Expression *>(adapter), nullptr),
                                           ScriptFunctionFlags::NONE, allocator);
}

void ETSFunctionType::CopyTo(AstNode *other) const
{
    auto otherImpl = reinterpret_cast<ETSFunctionType *>(other);

    otherImpl->scope_ = scope_;
    otherImpl->signature_.CopyFrom(signature_);
    otherImpl->functionalInterface_ = functionalInterface_;
    otherImpl->funcFlags_ = funcFlags_;

    TypeNode::CopyTo(other);
}

}  // namespace ark::es2panda::ir
