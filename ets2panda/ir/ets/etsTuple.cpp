/**
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "etsTuple.h"

#include "checker/ETSchecker.h"
#include "checker/types/ets/etsTupleType.h"

namespace ark::es2panda::ir {

void ETSTuple::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    for (auto *&it : GetTupleTypeAnnotationsList()) {
        if (auto *transformedNode = cb(it); it != transformedNode) {
            it->SetTransformedNode(transformationName, transformedNode);
            it = static_cast<TypeNode *>(transformedNode);
        }
    }

    TransformAnnotations(cb, transformationName);
}

void ETSTuple::Iterate(const NodeTraverser &cb) const
{
    for (auto *const it : GetTupleTypeAnnotationsList()) {
        cb(it);
    }

    IterateAnnotations(cb);
}

void ETSTuple::Dump(ir::AstDumper *const dumper) const
{
    dumper->Add({{"type", "ETSTuple"},
                 {"types", AstDumper::Optional(typeAnnotationList_)},
                 {"annotations", AstDumper::Optional(Annotations())}});
}

void ETSTuple::Dump(ir::SrcDumper *const dumper) const
{
    DumpAnnotations(dumper);
    dumper->Add("[");
    for (const auto *const typeAnnot : typeAnnotationList_) {
        typeAnnot->Dump(dumper);
        if (typeAnnot != typeAnnotationList_.back()) {
            dumper->Add(", ");
        }
    }
    dumper->Add("]");
}

void ETSTuple::Compile([[maybe_unused]] compiler::PandaGen *const pg) const {}
void ETSTuple::Compile([[maybe_unused]] compiler::ETSGen *const etsg) const {}

checker::Type *ETSTuple::Check([[maybe_unused]] checker::TSChecker *const checker)
{
    return nullptr;
}

checker::VerifiedType ETSTuple::Check([[maybe_unused]] checker::ETSChecker *const checker)
{
    return {this, GetType(checker)};
}

checker::Type *ETSTuple::GetHolderTypeForTuple(checker::ETSChecker *const checker,
                                               ArenaVector<checker::Type *> &typeList)
{
    if (typeList.empty()) {
        return checker->GlobalETSObjectType();
    }

    const bool allElementsAreSame = std::all_of(typeList.begin(), typeList.end(), [&checker, &typeList](auto *element) {
        return checker->Relation()->IsIdenticalTo(typeList[0], element);
    });
    if (allElementsAreSame) {
        return typeList[0];
    }

    /* NOTE(gogabr): if we compute a union type, we'll lose smaller numeric types, so just return Object */
    return checker->GlobalETSAnyType();
}

checker::Type *ETSTuple::GetType(checker::ETSChecker *const checker)
{
    if (TsType() != nullptr) {
        return TsType();
    }
    checker->CheckAnnotations(this);

    // NOTE (smartin): Remove, when TupleN is handled in codegen
    constexpr uint8_t MAX_TUPLE_ARITY = 16;
    if (GetTupleTypeAnnotationsList().size() > MAX_TUPLE_ARITY) {
        checker->LogError(diagnostic::TUPLEN_NOT_IMPLEMENTED, {}, Start());
        return checker->InvalidateType(this);
    }

    ArenaVector<checker::Type *> typeList(checker->ProgramAllocator()->Adapter());

    bool isTypeError = false;
    for (auto *const typeAnnotation : GetTupleTypeAnnotationsList()) {
        auto *const checkedType = typeAnnotation->GetType(checker);
        if (!isTypeError && checkedType->IsTypeError()) {
            isTypeError = true;
        }
        typeList.emplace_back(checkedType);
    }

    if (isTypeError) {
        return SetTsType(checker->GlobalTypeError());
    }

    auto *tupleType = checker->ProgramAllocator()->New<checker::ETSTupleType>(checker, typeList);

    if (IsReadonlyType()) {
        ES2PANDA_ASSERT(checker->GetReadonlyType(tupleType));
        tupleType = checker->GetReadonlyType(tupleType)->AsETSTupleType();
    }

    SetTsType(tupleType);
    return TsType();
}

ETSTuple *ETSTuple::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const clone = allocator->New<ETSTuple>(allocator, size_);
    ES2PANDA_ASSERT(clone != nullptr);

    clone->AddModifier(flags_);

    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    ArenaVector<TypeNode *> typeList(allocator->Adapter());
    for (auto *const type : typeAnnotationList_) {
        auto *const t = type->Clone(allocator, clone);
        typeList.push_back(t);
    }

    if (HasAnnotations()) {
        clone->SetAnnotations(Annotations());
    }

    clone->SetTypeAnnotationsList(std::move(typeList));

    clone->SetRange(Range());
    return clone;
}

}  // namespace ark::es2panda::ir
