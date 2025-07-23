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

#include "tsTupleType.h"

#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "util/helpers.h"
#include "varbinder/scope.h"
#include "checker/TSchecker.h"
#include "checker/types/ts/indexInfo.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"
#include "ir/expressions/identifier.h"
#include "ir/ts/tsNamedTupleMember.h"

namespace ark::es2panda::ir {
void TSTupleType::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    for (auto *&it : VectorIterationGuard(elementTypes_)) {
        if (auto *transformedNode = cb(it); it != transformedNode) {
            it->SetTransformedNode(transformationName, transformedNode);
            it = static_cast<TypeNode *>(transformedNode);
        }
    }

    TransformAnnotations(cb, transformationName);
}

void TSTupleType::Iterate(const NodeTraverser &cb) const
{
    for (auto *it : VectorIterationGuard(elementTypes_)) {
        cb(it);
    }

    IterateAnnotations(cb);
}

void TSTupleType::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "TSTupleType"},
                 {"elementTypes", elementTypes_},
                 {"annotations", AstDumper::Optional(Annotations())}});
}

void TSTupleType::Dump(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);
    dumper->Add("TSTupleType");
}

void TSTupleType::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void TSTupleType::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *GetNumberIndexType(ArenaVector<checker::Type *> numberIndexTypes, checker::TSChecker *checker)
{
    checker::Type *numberIndexType = nullptr;
    if (numberIndexTypes.empty()) {
        numberIndexType = checker->GlobalNeverType();
    } else if (numberIndexTypes.size() == 1) {
        numberIndexType = numberIndexTypes[0];
    } else {
        numberIndexType = checker->CreateUnionType(std::move(numberIndexTypes));
    }
    return numberIndexType;
}

static void SetMemberVarType(checker::Type *memberType, varbinder::LocalVariable *memberVar)
{
    ES2PANDA_ASSERT(memberType != nullptr);
    memberType->SetVariable(memberVar);
    memberVar->SetTsType(memberType);
}

checker::Type *TSTupleType::GetType(checker::TSChecker *checker)
{
    if (TsType() != nullptr) {
        return TsType();
    }

    checker::ObjectDescriptor *desc = checker->Allocator()->New<checker::ObjectDescriptor>(checker->Allocator());
    ES2PANDA_ASSERT(desc != nullptr);
    checker::NamedTupleMemberPool namedMembers(checker->Allocator()->Adapter());
    ArenaVector<checker::ElementFlags> elementFlags(checker->Allocator()->Adapter());
    checker::ElementFlags combinedFlags = checker::ElementFlags::NO_OPTS;
    uint32_t minLength = 0;
    uint32_t index = 0;
    ArenaVector<checker::Type *> numberIndexTypes(checker->Allocator()->Adapter());
    for (auto *it : elementTypes_) {
        util::StringView memberIndex = util::Helpers::ToStringView(checker->Allocator(), index);

        auto *memberVar =
            varbinder::Scope::CreateVar(checker->Allocator(), memberIndex, varbinder::VariableFlags::PROPERTY, it);
        ES2PANDA_ASSERT(memberVar != nullptr);
        checker::ElementFlags memberFlag;
        if (it->IsTSNamedTupleMember()) {
            auto *namedMember = it->AsTSNamedTupleMember();
            checker::Type *memberType = namedMember->ElementType()->GetType(checker);

            if (namedMember->IsOptional()) {
                memberVar->AddFlag(varbinder::VariableFlags::OPTIONAL);
                memberFlag = checker::ElementFlags::OPTIONAL;
            } else {
                memberFlag = checker::ElementFlags::REQUIRED;
                minLength++;
            }

            SetMemberVarType(memberType, memberVar);
            numberIndexTypes.push_back(memberType);
            namedMembers.insert({memberVar, namedMember->Label()->AsIdentifier()->Name()});
        } else {
            checker::Type *memberType = it->GetType(checker);
            SetMemberVarType(memberType, memberVar);
            memberFlag = checker::ElementFlags::REQUIRED;
            numberIndexTypes.push_back(memberType);
            minLength++;
        }

        combinedFlags |= memberFlag;

        elementFlags.push_back(memberFlag);
        desc->properties.push_back(memberVar);
        index++;
    }

    uint32_t fixedLength = desc->properties.size();

    checker::Type *numberIndexType = GetNumberIndexType(numberIndexTypes, checker);
    desc->numberIndexInfo = checker->Allocator()->New<checker::IndexInfo>(numberIndexType, "x", false);

    const checker::TupleTypeInfo tupleTypeInfo = {combinedFlags, minLength, fixedLength, false};
    SetTsType(checker->CreateTupleType(desc, std::move(elementFlags), tupleTypeInfo, std::move(namedMembers)));
    return TsType();
}

checker::Type *TSTupleType::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType TSTupleType::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

TSTupleType *TSTupleType::Clone(ArenaAllocator *allocator, AstNode *parent)
{
    ArenaVector<TypeNode *> clonedElementTypes(allocator->Adapter());
    for (auto *elementType : elementTypes_) {
        clonedElementTypes.push_back(elementType->Clone(allocator, nullptr)->AsTypeNode());
    }

    auto *clone = allocator->New<TSTupleType>(std::move(clonedElementTypes), allocator);

    // Set parent relationships for cloned element types
    for (auto *elementType : clone->elementTypes_) {
        elementType->SetParent(clone);
    }

    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    clone->SetRange(Range());

    // Clone annotations if any
    if (HasAnnotations()) {
        clone->SetAnnotations(Annotations());
    }

    return clone;
}
}  // namespace ark::es2panda::ir
