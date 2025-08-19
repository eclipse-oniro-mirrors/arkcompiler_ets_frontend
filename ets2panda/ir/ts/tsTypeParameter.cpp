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

#include "tsTypeParameter.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"
#include "ir/typeNode.h"
#include "ir/expressions/identifier.h"
#include "utils/arena_containers.h"

namespace ark::es2panda::ir {

void TSTypeParameter::SetConstraint(TypeNode *constraint)
{
    this->GetOrCreateHistoryNodeAs<TSTypeParameter>()->constraint_ = constraint;
}

void TSTypeParameter::SetDefaultType(TypeNode *defaultType)
{
    this->GetOrCreateHistoryNodeAs<TSTypeParameter>()->defaultType_ = defaultType;
}

void TSTypeParameter::SetName(Identifier *name)
{
    this->GetOrCreateHistoryNodeAs<TSTypeParameter>()->name_ = name;
}

void TSTypeParameter::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    auto const name = Name();
    if (auto *transformedNode = cb(name); name != transformedNode) {
        name->SetTransformedNode(transformationName, transformedNode);
        SetName(transformedNode->AsIdentifier());
    }

    auto const constraint = Constraint();
    if (constraint != nullptr) {
        if (auto *transformedNode = cb(constraint); constraint != transformedNode) {
            constraint->SetTransformedNode(transformationName, transformedNode);
            SetConstraint(static_cast<TypeNode *>(transformedNode));
        }
    }

    auto const defaultType = DefaultType();
    if (defaultType != nullptr) {
        if (auto *transformedNode = cb(defaultType); defaultType != transformedNode) {
            defaultType->SetTransformedNode(transformationName, transformedNode);
            SetDefaultType(static_cast<TypeNode *>(transformedNode));
        }
    }

    TransformAnnotations(cb, transformationName);
}

void TSTypeParameter::Iterate(const NodeTraverser &cb) const
{
    auto const name = GetHistoryNodeAs<TSTypeParameter>()->name_;
    cb(name);

    auto const constraint = GetHistoryNodeAs<TSTypeParameter>()->constraint_;
    if (constraint != nullptr) {
        cb(constraint);
    }

    auto const defaultType = GetHistoryNodeAs<TSTypeParameter>()->defaultType_;
    if (defaultType != nullptr) {
        cb(defaultType);
    }

    IterateAnnotations(cb);
}

void TSTypeParameter::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "TSTypeParameter"},
                 {"name", Name()},
                 {"constraint", AstDumper::Optional(Constraint())},
                 {"default", AstDumper::Optional(DefaultType())},
                 {"in", AstDumper::Optional(IsIn())},
                 {"out", AstDumper::Optional(IsOut())},
                 {"annotations", AstDumper::Optional(Annotations())}});
}

void TSTypeParameter::Dump(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);
    if (IsIn()) {
        dumper->Add("in ");
    }
    if (IsOut()) {
        dumper->Add("out ");
    }

    Name()->Dump(dumper);

    if (DefaultType() != nullptr) {
        dumper->Add(" = ");
        DefaultType()->Dump(dumper);
    }
    if (Constraint() != nullptr) {
        dumper->Add(" extends ");
        Constraint()->Dump(dumper);
    }
}

void TSTypeParameter::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}
void TSTypeParameter::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *TSTypeParameter::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType TSTypeParameter::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

TSTypeParameter *TSTypeParameter::Construct(ArenaAllocator *allocator)
{
    return allocator->New<TSTypeParameter>(nullptr, nullptr, nullptr, allocator);
}

void TSTypeParameter::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsTSTypeParameter();

    otherImpl->name_ = name_;
    otherImpl->constraint_ = constraint_;
    otherImpl->defaultType_ = defaultType_;

    AnnotationAllowed<Expression>::CopyTo(other);
}

}  // namespace ark::es2panda::ir
