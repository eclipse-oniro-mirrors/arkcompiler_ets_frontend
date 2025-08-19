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

#include "tsUnionType.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"

namespace ark::es2panda::ir {
void TSUnionType::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    for (auto *&it : VectorIterationGuard(types_)) {
        if (auto *transformedNode = cb(it); it != transformedNode) {
            it->SetTransformedNode(transformationName, transformedNode);
            it = static_cast<TypeNode *>(transformedNode);
        }
    }

    TransformAnnotations(cb, transformationName);
}

void TSUnionType::Iterate(const NodeTraverser &cb) const
{
    for (auto *it : VectorIterationGuard(types_)) {
        cb(it);
    }

    IterateAnnotations(cb);
}

void TSUnionType::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "TSUnionType"}, {"types", types_}, {"annotations", AstDumper::Optional(Annotations())}});
}

void TSUnionType::Dump(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);
    dumper->Add("TSUnionType");
}

void TSUnionType::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void TSUnionType::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *TSUnionType::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType TSUnionType::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

checker::Type *TSUnionType::GetType(checker::TSChecker *checker)
{
    if (TsType() != nullptr) {
        return TsType();
    }

    ArenaVector<checker::Type *> types(checker->Allocator()->Adapter());

    for (auto *it : types_) {
        types.push_back(it->GetType(checker));
    }

    SetTsType(checker->CreateUnionType(std::move(types)));
    return TsType();
}

TSUnionType *TSUnionType::Clone(ArenaAllocator *allocator, AstNode *parent)
{
    // Clone all type nodes in the union
    ArenaVector<TypeNode *> clonedTypes(allocator->Adapter());
    for (auto *type : types_) {
        if (type != nullptr) {
            clonedTypes.push_back(type->Clone(allocator, nullptr)->AsTypeNode());
        }
    }

    auto *clone = allocator->New<TSUnionType>(std::move(clonedTypes), allocator);

    // Set parent for all cloned types
    for (auto *clonedType : clone->Types()) {
        if (clonedType != nullptr) {
            clonedType->SetParent(clone);
        }
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
