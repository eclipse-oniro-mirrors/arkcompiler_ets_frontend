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

#include "annotationUsage.h"
#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {
void AnnotationUsage::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    if (auto *transformedNode = cb(expr_); expr_ != transformedNode) {
        expr_->SetTransformedNode(transformationName, transformedNode);
        expr_ = transformedNode->AsExpression();
    }

    for (auto *&it : VectorIterationGuard(properties_)) {
        if (auto *transformedNode = cb(it); it != transformedNode) {
            it->SetTransformedNode(transformationName, transformedNode);
            it = transformedNode;
        }
    }
}
void AnnotationUsage::Iterate(const NodeTraverser &cb) const
{
    if (expr_ != nullptr) {
        cb(expr_);
    }

    for (auto *it : VectorIterationGuard(properties_)) {
        cb(it);
    }
}

void AnnotationUsage::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"expr_", expr_}, {"properties", properties_}});
}
void AnnotationUsage::Dump(ir::SrcDumper *dumper) const
{
    ES2PANDA_ASSERT(expr_ != nullptr);
    dumper->Add("@");
    expr_->Dump(dumper);
    dumper->Add("(");

    if (!properties_.empty()) {
        dumper->Add("{");
        for (auto elem : properties_) {
            ES2PANDA_ASSERT(elem->AsClassProperty()->Id() != nullptr);
            dumper->Add(elem->AsClassProperty()->Id()->Name().Mutf8());
            dumper->Add(":");
            elem->AsClassProperty()->Value()->Dump(dumper);
            if (elem != properties_.back()) {
                dumper->Add(",");
            }
        }
        dumper->Add("}");
    }
    dumper->Add(") ");
}

AnnotationUsage *AnnotationUsage::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const expr = expr_ != nullptr ? expr_->Clone(allocator, nullptr)->AsExpression() : nullptr;
    auto *const clone = allocator->New<AnnotationUsage>(expr, allocator);
    ES2PANDA_ASSERT(clone != nullptr);

    if (expr != nullptr) {
        expr->SetParent(clone);
    }

    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    for (auto *property : properties_) {
        clone->AddProperty(property->Clone(allocator, clone));
    }

    clone->SetRange(range_.GetRange());
    clone->SetScope(propertiesScope_);

    return clone;
}

void AnnotationUsage::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void AnnotationUsage::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *AnnotationUsage::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType AnnotationUsage::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

Identifier *AnnotationUsage::GetBaseName() const
{
    if (expr_->IsIdentifier()) {
        return expr_->AsIdentifier();
    }
    return expr_->AsETSTypeReference()->Part()->GetIdent();
}
}  // namespace ark::es2panda::ir
