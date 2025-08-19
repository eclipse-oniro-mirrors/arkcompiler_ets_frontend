/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "annotationDeclaration.h"
#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"

namespace ark::es2panda::ir {

void AnnotationDeclaration::SetInternalName(util::StringView internalName)
{
    this->GetOrCreateHistoryNodeAs<AnnotationDeclaration>()->internalName_ = internalName;
}

void AnnotationDeclaration::SetExpr(Expression *expr)
{
    this->GetOrCreateHistoryNodeAs<AnnotationDeclaration>()->expr_ = expr;
}

void AnnotationDeclaration::EmplaceProperties(AstNode *properties)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<AnnotationDeclaration>();
    newNode->properties_.emplace_back(properties);
}

void AnnotationDeclaration::ClearProperties()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<AnnotationDeclaration>();
    newNode->properties_.clear();
}

void AnnotationDeclaration::SetValueProperties(AstNode *properties, size_t index)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<AnnotationDeclaration>();
    auto &arenaVector = newNode->properties_;
    ES2PANDA_ASSERT(arenaVector.size() > index);
    arenaVector[index] = properties;
}

[[nodiscard]] const ArenaVector<AstNode *> &AnnotationDeclaration::Properties()
{
    auto newNode = this->GetHistoryNodeAs<AnnotationDeclaration>();
    return newNode->properties_;
}

[[nodiscard]] ArenaVector<AstNode *> &AnnotationDeclaration::PropertiesForUpdate()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<AnnotationDeclaration>();
    return newNode->properties_;
}

void AnnotationDeclaration::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    auto const &properties = Properties();
    for (size_t ix = 0; ix < properties.size(); ix++) {
        if (auto *transformedNode = cb(properties[ix]); properties[ix] != transformedNode) {
            properties[ix]->SetTransformedNode(transformationName, transformedNode);
            SetValueProperties(transformedNode->AsTSClassImplements(), ix);
        }
    }

    auto const expr = Expr();
    if (expr != nullptr) {
        if (auto *transformedNode = cb(expr); expr != transformedNode) {
            expr->SetTransformedNode(transformationName, transformedNode);
            SetExpr(transformedNode->AsIdentifier());
        }
    }

    TransformAnnotations(cb, transformationName);
}
void AnnotationDeclaration::Iterate(const NodeTraverser &cb) const
{
    auto const expr = GetHistoryNodeAs<AnnotationDeclaration>()->expr_;
    if (expr != nullptr) {
        cb(expr);
    }

    for (auto *it : VectorIterationGuard(Properties())) {
        cb(it);
    }

    IterateAnnotations(cb);
}

void AnnotationDeclaration::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"Expr", Expr()}, {"properties", Properties()}, {"annotations", AstDumper::Optional(Annotations())}});
}

void AnnotationDeclaration::Dump(ir::SrcDumper *dumper) const
{  // re-understand
    DumpAnnotations(dumper);
    ES2PANDA_ASSERT(Expr() != nullptr);
    dumper->Add("@interface ");
    Expr()->Dump(dumper);
    dumper->Add(" {");

    auto const properties = Properties();
    if (!properties.empty()) {
        dumper->IncrIndent();
        dumper->Endl();
        for (auto elem : properties) {
            elem->Dump(dumper);
            if (elem == properties.back()) {
                dumper->DecrIndent();
            }
        }
    }
    dumper->Add("}");
    dumper->Endl();
}
void AnnotationDeclaration::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void AnnotationDeclaration::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *AnnotationDeclaration::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType AnnotationDeclaration::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

Identifier *AnnotationDeclaration::GetBaseName() const
{
    if (Expr()->IsIdentifier()) {
        return GetHistoryNodeAs<AnnotationDeclaration>()->expr_->AsIdentifier();
    }
    return expr_->AsETSTypeReference()->Part()->GetIdent();
}
AnnotationDeclaration *AnnotationDeclaration::Construct(ArenaAllocator *allocator)
{
    return allocator->New<AnnotationDeclaration>(nullptr, allocator);
}

void AnnotationDeclaration::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsAnnotationDeclaration();

    otherImpl->internalName_ = internalName_;
    otherImpl->scope_ = scope_;
    otherImpl->expr_ = expr_;
    otherImpl->properties_ = properties_;
    otherImpl->policy_ = policy_;

    AnnotationAllowed<Statement>::CopyTo(other);
}
}  // namespace ark::es2panda::ir
