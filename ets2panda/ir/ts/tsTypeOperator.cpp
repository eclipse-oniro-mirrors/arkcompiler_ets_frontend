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

#include "tsTypeOperator.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"

namespace ark::es2panda::ir {
void TSTypeOperator::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    if (auto *transformedNode = cb(type_); type_ != transformedNode) {
        type_->SetTransformedNode(transformationName, transformedNode);
        type_ = static_cast<TypeNode *>(transformedNode);
    }

    TransformAnnotations(cb, transformationName);
}

void TSTypeOperator::Iterate(const NodeTraverser &cb) const
{
    cb(type_);

    IterateAnnotations(cb);
}

void TSTypeOperator::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "TSTypeOperator"},
                 {"operator", AstDumper::TypeOperatorToString(operatorType_)},
                 {"typeAnnotation", type_},
                 {"annotations", AstDumper::Optional(Annotations())}});
}

void TSTypeOperator::Dump(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);
    dumper->Add("TSTypeOperator");
}

void TSTypeOperator::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}
void TSTypeOperator::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *TSTypeOperator::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::Type *TSTypeOperator::GetType([[maybe_unused]] checker::TSChecker *checker)
{
    return nullptr;
}

checker::VerifiedType TSTypeOperator::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

TSTypeOperator *TSTypeOperator::Clone(ArenaAllocator *allocator, AstNode *parent)
{
    // Clone the type annotation
    TypeNode *clonedType = nullptr;
    if (type_ != nullptr) {
        clonedType = type_->Clone(allocator, nullptr)->AsTypeNode();
    }

    auto *clone = allocator->New<TSTypeOperator>(clonedType, operatorType_, allocator);

    // Set parent for cloned type
    if (clonedType != nullptr) {
        clonedType->SetParent(clone);
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
