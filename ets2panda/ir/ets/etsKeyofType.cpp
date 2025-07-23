/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "etsKeyofType.h"

#include "checker/ETSchecker.h"

namespace ark::es2panda::ir {
void ETSKeyofType::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    if (auto *transformedNode = cb(type_); type_ != transformedNode) {
        type_->SetTransformedNode(transformationName, transformedNode);
        type_ = static_cast<TypeNode *>(transformedNode);
    }
}

void ETSKeyofType::Iterate(const NodeTraverser &cb) const
{
    cb(type_);
}

void ETSKeyofType::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ETSKeyofType"}, {"types", type_}, {"annotations", AstDumper::Optional(Annotations())}});
}

void ETSKeyofType::Dump(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);
    dumper->Add("keyof ");
    type_->Dump(dumper);
}

void ETSKeyofType::Compile([[maybe_unused]] compiler::PandaGen *pg) const {}

checker::Type *ETSKeyofType::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return nullptr;
}

checker::VerifiedType ETSKeyofType::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

checker::Type *ETSKeyofType::GetType(checker::ETSChecker *checker)
{
    if (TsType() != nullptr) {
        return TsType();
    }

    auto *typeReference = type_->GetType(checker);
    ES2PANDA_ASSERT(typeReference);

    if (typeReference->IsETSPrimitiveType()) {
        typeReference = checker->MaybeBoxType(typeReference);
    }

    if (!typeReference->IsETSObjectType()) {
        checker->LogError(diagnostic::KEYOF_REFERENCE_TYPE, {}, Start());
        SetTsType(checker->GlobalTypeError());
        return checker->GlobalTypeError();
    }

    checker::TypeStackElement tse(checker, GetHistoryNode(), {{diagnostic::CYCLIC_TYPE_OF, {}}}, Start());
    if (tse.HasTypeError()) {
        return checker->GlobalTypeError();
    }

    SetTsType(checker->CreateUnionFromKeyofType(typeReference->AsETSObjectType()));
    return TsType();
}

ETSKeyofType *ETSKeyofType::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    TypeNode *type = type_->Clone(allocator, nullptr);
    ETSKeyofType *clone = allocator->New<ir::ETSKeyofType>(type, allocator);
    ES2PANDA_ASSERT(clone);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }
    clone->type_->SetParent(clone);
    return clone;
}
}  // namespace ark::es2panda::ir
