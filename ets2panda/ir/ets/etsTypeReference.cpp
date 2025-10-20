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

#include "etsTypeReference.h"

#include "checker/ETSchecker.h"
#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {

void ETSTypeReference::SetPart(ETSTypeReferencePart *part)
{
    this->GetOrCreateHistoryNodeAs<ETSTypeReference>()->part_ = part;
}

void ETSTypeReference::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    auto const part = Part();
    if (auto *transformedNode = cb(part); part != transformedNode) {
        part->SetTransformedNode(transformationName, transformedNode);
        SetPart(transformedNode->AsETSTypeReferencePart());
    }

    TransformAnnotations(cb, transformationName);
}

void ETSTypeReference::Iterate(const NodeTraverser &cb) const
{
    auto const part = GetHistoryNodeAs<ETSTypeReference>()->part_;
    cb(part);
    IterateAnnotations(cb);
}

ir::Identifier *ETSTypeReference::BaseName() const
{
    ir::ETSTypeReferencePart *partIter = Part();

    while (partIter->Previous() != nullptr) {
        partIter = partIter->Previous();
    }

    ir::Expression *baseName = partIter->Name();

    if (baseName->IsIdentifier()) {
        return baseName->AsIdentifier();
    }

    if (baseName->IsIdentifier()) {
        return baseName->AsIdentifier();
    }

    if (baseName->IsTSQualifiedName()) {
        ir::TSQualifiedName *iter = baseName->AsTSQualifiedName();

        while (iter->Left()->IsTSQualifiedName()) {
            iter = iter->Left()->AsTSQualifiedName();
        }
        if (iter->Left()->IsMemberExpression()) {
            ES2PANDA_ASSERT(iter->Left()->AsMemberExpression()->ObjType()->HasObjectFlag(
                checker::ETSObjectFlags::LAZY_IMPORT_OBJECT));
            ir::MemberExpression *memberExprIter = iter->Left()->AsMemberExpression();
            while (memberExprIter->Property()->IsMemberExpression()) {
                memberExprIter = memberExprIter->Property()->AsMemberExpression();
            }
            return memberExprIter->Property()->AsIdentifier();
        }
        return iter->Left()->AsIdentifier();
    }

    if (baseName->IsMemberExpression()) {
        ir::MemberExpression *iter = baseName->AsMemberExpression();

        while (iter->Property()->IsMemberExpression()) {
            iter = iter->Property()->AsMemberExpression();
        }
        return iter->Property()->AsIdentifier();
    }

    if (baseName->IsLiteral()) {
        ES2PANDA_ASSERT(baseName->OriginalNode() != nullptr && baseName->OriginalNode()->IsIdentifier());
        return baseName->OriginalNode()->AsIdentifier();
    }

    ES2PANDA_UNREACHABLE();
    return nullptr;
}

void ETSTypeReference::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ETSTypeReference"}, {"part", Part()}, {"annotations", AstDumper::Optional(Annotations())}});
}

void ETSTypeReference::Dump(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);
    ES2PANDA_ASSERT(Part() != nullptr);
    Part()->Dump(dumper);
}

void ETSTypeReference::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}
void ETSTypeReference::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ETSTypeReference::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType ETSTypeReference::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}
checker::Type *ETSTypeReference::GetType(checker::ETSChecker *checker)
{
    if (TsType() != nullptr) {
        return TsType();
    }
    auto *type = Part()->GetType(checker);
    if (IsReadonlyType()) {
        type = checker->GetReadonlyType(type);
    }
    return SetTsType(type);
}

ETSTypeReference *ETSTypeReference::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    ETSTypeReferencePart *partClone = nullptr;
    if (Part() != nullptr) {
        auto *const clone = Part()->Clone(allocator, nullptr);
        ES2PANDA_ASSERT(clone != nullptr);
        partClone = clone->AsETSTypeReferencePart();
    }
    auto *const clone = allocator->New<ETSTypeReference>(partClone, allocator);
    ES2PANDA_ASSERT(clone != nullptr);
    clone->SetRange(Range());

    if (partClone != nullptr) {
        partClone->SetParent(clone);
    }

    clone->flags_ = Modifiers();

    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    if (HasAnnotations()) {
        clone->SetAnnotations(Annotations());
    }

    return clone;
}

ETSTypeReference *ETSTypeReference::Construct(ArenaAllocator *allocator)
{
    return allocator->New<ETSTypeReference>(nullptr, allocator);
}

void ETSTypeReference::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsETSTypeReference();

    otherImpl->part_ = part_;

    TypeNode::CopyTo(other);
}

}  // namespace ark::es2panda::ir
