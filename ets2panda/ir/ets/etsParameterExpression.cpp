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

#include "etsParameterExpression.h"

#include "checker/ETSchecker.h"
#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {

void ETSParameterExpression::SetRequiredParams(size_t extraValue)
{
    this->GetOrCreateHistoryNodeAs<ETSParameterExpression>()->extraValue_ = extraValue;
}

void ETSParameterExpression::SetLexerSaved(util::StringView savedLexer)
{
    this->GetOrCreateHistoryNodeAs<ETSParameterExpression>()->savedLexer_ = savedLexer;
}

void ETSParameterExpression::SetSpread(SpreadElement *spread)
{
    this->GetOrCreateHistoryNodeAs<ETSParameterExpression>()->spread_ = spread;
}

ETSParameterExpression::ETSParameterExpression(AnnotatedExpression *const identOrSpread, bool isOptional,
                                               ArenaAllocator *const allocator)
    : ETSParameterExpression(identOrSpread, isOptional, allocator, nullptr)
{
}

ETSParameterExpression::ETSParameterExpression(AnnotatedExpression *const identOrSpread, bool isOptional,
                                               ArenaAllocator *const allocator, AstNodeHistory *history)
    : AnnotationAllowed<Expression>(AstNodeType::ETS_PARAMETER_EXPRESSION, allocator)
{
    SetOptional(isOptional);

    if (identOrSpread == nullptr) {
        return;
    }
    identOrSpread->SetParent(this);
    SetRange(identOrSpread->Range());
    if (identOrSpread->IsIdentifier()) {
        ident_ = identOrSpread->AsIdentifier();
    } else if (identOrSpread->IsRestElement()) {
        spread_ = identOrSpread->AsRestElement();
        ES2PANDA_ASSERT(spread_->Argument()->IsIdentifier());
        ident_ = spread_->Argument()->AsIdentifier();
        ident_->SetParent(spread_);
    } else {
        ES2PANDA_UNREACHABLE();
    }

    if (history != nullptr) {
        history_ = history;
    } else {
        InitHistory();
    }
}

ETSParameterExpression::ETSParameterExpression(AnnotatedExpression *const identOrSpread, ir::Expression *initializer,
                                               ArenaAllocator *const allocator)
    : ETSParameterExpression(identOrSpread, initializer, allocator, nullptr)
{
}

ETSParameterExpression::ETSParameterExpression(AnnotatedExpression *const identOrSpread, ir::Expression *initializer,
                                               ArenaAllocator *const allocator, AstNodeHistory *history)
    : ETSParameterExpression(identOrSpread, true, allocator)
{
    SetInitializer(initializer);

    if (history != nullptr) {
        history_ = history;
    } else {
        InitHistory();
    }
}

const util::StringView &ETSParameterExpression::Name() const noexcept
{
    return Ident()->Name();
}

const Identifier *ETSParameterExpression::Ident() const noexcept
{
    return GetHistoryNodeAs<ETSParameterExpression>()->ident_;
}

Identifier *ETSParameterExpression::Ident() noexcept
{
    return GetHistoryNodeAs<ETSParameterExpression>()->ident_;
}

const SpreadElement *ETSParameterExpression::RestParameter() const noexcept
{
    return GetHistoryNodeAs<ETSParameterExpression>()->spread_;
}

SpreadElement *ETSParameterExpression::RestParameter() noexcept
{
    return GetHistoryNodeAs<ETSParameterExpression>()->spread_;
}

const Expression *ETSParameterExpression::Initializer() const noexcept
{
    return GetHistoryNodeAs<ETSParameterExpression>()->initializer_;
}

Expression *ETSParameterExpression::Initializer() noexcept
{
    return GetHistoryNodeAs<ETSParameterExpression>()->initializer_;
}

varbinder::Variable *ETSParameterExpression::Variable() const noexcept
{
    return Ident()->Variable();
}

TypeNode const *ETSParameterExpression::TypeAnnotation() const noexcept
{
    return !IsRestParameter() ? Ident()->TypeAnnotation() : Spread()->TypeAnnotation();
}

TypeNode *ETSParameterExpression::TypeAnnotation() noexcept
{
    return !IsRestParameter() ? Ident()->TypeAnnotation() : Spread()->TypeAnnotation();
}

void ETSParameterExpression::SetTypeAnnotation(TypeNode *typeNode) noexcept
{
    !IsRestParameter() ? Ident()->SetTsTypeAnnotation(typeNode) : Spread()->SetTsTypeAnnotation(typeNode);
}

void ETSParameterExpression::SetVariable(varbinder::Variable *const variable) noexcept
{
    Ident()->SetVariable(variable);
}

util::StringView ETSParameterExpression::LexerSaved() const noexcept
{
    return GetHistoryNodeAs<ETSParameterExpression>()->savedLexer_;
}

void ETSParameterExpression::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    auto const spread = Spread();
    auto const ident = Ident();
    auto newNode = GetOrCreateHistoryNodeAs<ETSParameterExpression>();
    if (IsRestParameter()) {
        if (auto *transformedNode = cb(spread); spread != transformedNode) {
            spread->SetTransformedNode(transformationName, transformedNode);
            SetSpread(transformedNode->AsRestElement());
        }
        newNode->ident_ = Spread()->Argument()->AsIdentifier();
    } else {
        if (auto *transformedNode = cb(ident); ident != transformedNode) {
            ident->SetTransformedNode(transformationName, transformedNode);
            SetIdent(transformedNode->AsIdentifier());
        }
    }

    auto const initializer = Initializer();
    if (initializer != nullptr) {
        if (auto *transformedNode = cb(initializer); initializer != transformedNode) {
            initializer->SetTransformedNode(transformationName, transformedNode);
            SetInitializer(transformedNode->AsExpression());
        }
    }

    TransformAnnotations(cb, transformationName);
}

void ETSParameterExpression::Iterate(const NodeTraverser &cb) const
{
    if (IsRestParameter()) {
        auto const spread = GetHistoryNode()->AsETSParameterExpression()->spread_;
        cb(spread);
    } else {
        auto const ident = GetHistoryNode()->AsETSParameterExpression()->ident_;
        cb(ident);
    }

    auto const initializer = GetHistoryNode()->AsETSParameterExpression()->initializer_;
    if (initializer != nullptr) {
        cb(initializer);
    }

    IterateAnnotations(cb);
}

void ETSParameterExpression::Dump(ir::AstDumper *const dumper) const
{
    if (!IsRestParameter()) {
        dumper->Add({{"type", "ETSParameterExpression"},
                     {"name", Ident()},
                     {"initializer", AstDumper::Optional(Initializer())},
                     {"annotations", AstDumper::Optional(Annotations())}});
    } else {
        dumper->Add({{"type", "ETSParameterExpression"},
                     {"rest parameter", Spread()},
                     {"annotations", AstDumper::Optional(Annotations())}});
    }
}

void ETSParameterExpression::Dump(ir::SrcDumper *const dumper) const
{
    DumpAnnotations(dumper);

    if (IsRestParameter()) {
        Spread()->Dump(dumper);
    } else {
        auto const ident = Ident();
        auto const initializer = Initializer();
        if (ident != nullptr) {
            ES2PANDA_ASSERT(ident_->IsAnnotatedExpression());
            ident->Dump(dumper);
            if (IsOptional() && initializer == nullptr) {
                dumper->Add("?");
            }
            auto typeAnnotation = ident->AsAnnotatedExpression()->TypeAnnotation();
            if (typeAnnotation != nullptr) {
                dumper->Add(": ");
                typeAnnotation->Dump(dumper);
            }
        }
        if (initializer != nullptr) {
            dumper->Add(" = ");
            initializer->Dump(dumper);
        }
    }
}

void ETSParameterExpression::Compile(compiler::PandaGen *const pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void ETSParameterExpression::Compile(compiler::ETSGen *const etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ETSParameterExpression::Check(checker::TSChecker *const checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType ETSParameterExpression::Check(checker::ETSChecker *const checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

ETSParameterExpression *ETSParameterExpression::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    AnnotatedExpression *identOrSpread = nullptr;
    if (Spread() != nullptr) {
        auto spreadClone = Spread()->Clone(allocator, nullptr);
        ES2PANDA_ASSERT(spreadClone != nullptr);
        identOrSpread = spreadClone->AsAnnotatedExpression();
    } else {
        auto identClone = Ident()->Clone(allocator, nullptr);
        ES2PANDA_ASSERT(identClone != nullptr);
        identOrSpread = identClone->AsAnnotatedExpression();
    }
    auto *const initializer =
        Initializer() != nullptr ? Initializer()->Clone(allocator, nullptr)->AsExpression() : nullptr;

    auto *const clone = Initializer() != nullptr
                            ? allocator->New<ETSParameterExpression>(identOrSpread, initializer, allocator)
                            : allocator->New<ETSParameterExpression>(identOrSpread, IsOptional(), allocator);
    ES2PANDA_ASSERT(identOrSpread != nullptr);
    // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
    identOrSpread->SetParent(clone);

    if (initializer != nullptr) {
        initializer->SetParent(clone);
    }

    ES2PANDA_ASSERT(clone != nullptr);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    clone->SetRequiredParams(GetRequiredParams());

    if (HasAnnotations()) {
        clone->SetAnnotations(Annotations());
    }

    return clone;
}

ETSParameterExpression *ETSParameterExpression::Construct(ArenaAllocator *allocator)
{
    return allocator->New<ETSParameterExpression>(nullptr, false, allocator);
}

void ETSParameterExpression::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsETSParameterExpression();

    otherImpl->ident_ = ident_;
    otherImpl->initializer_ = initializer_;
    otherImpl->spread_ = spread_;
    otherImpl->savedLexer_ = savedLexer_;
    otherImpl->extraValue_ = extraValue_;
    otherImpl->isOptional_ = isOptional_;

    AnnotationAllowed<Expression>::CopyTo(other);
}

}  // namespace ark::es2panda::ir
