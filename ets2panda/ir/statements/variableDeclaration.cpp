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

#include "variableDeclaration.h"

#include "checker/TSchecker.h"
#include "checker/ETSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "utils/arena_containers.h"

namespace ark::es2panda::ir {

void VariableDeclaration::EmplaceDeclarators(VariableDeclarator *source)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<VariableDeclaration>();
    newNode->declarators_.emplace_back(source);
}

void VariableDeclaration::ClearDeclarators()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<VariableDeclaration>();
    newNode->declarators_.clear();
}

void VariableDeclaration::SetValueDeclarators(VariableDeclarator *source, size_t index)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<VariableDeclaration>();
    auto &arenaVector = newNode->declarators_;
    ES2PANDA_ASSERT(arenaVector.size() > index);
    arenaVector[index] = source;
}

[[nodiscard]] const ArenaVector<VariableDeclarator *> &VariableDeclaration::Declarators()
{
    auto newNode = this->GetHistoryNodeAs<VariableDeclaration>();
    return newNode->declarators_;
}

[[nodiscard]] ArenaVector<VariableDeclarator *> &VariableDeclaration::DeclaratorsForUpdate()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<VariableDeclaration>();
    return newNode->declarators_;
}

void VariableDeclaration::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    TransformAnnotations(cb, transformationName);

    auto const &declarators = Declarators();
    for (size_t index = 0; index < declarators.size(); ++index) {
        if (auto *transformedNode = cb(declarators[index]); declarators[index] != transformedNode) {
            declarators[index]->SetTransformedNode(transformationName, transformedNode);
            SetValueDeclarators(transformedNode->AsVariableDeclarator(), index);
        }
    }
}

void VariableDeclaration::Iterate(const NodeTraverser &cb) const
{
    IterateAnnotations(cb);

    for (auto *it : VectorIterationGuard(Declarators())) {
        cb(it);
    }
}

void VariableDeclaration::Dump(ir::AstDumper *dumper) const
{
    const char *kind = nullptr;

    switch (Kind()) {
        case VariableDeclarationKind::CONST: {
            kind = "const";
            break;
        }
        case VariableDeclarationKind::LET: {
            kind = "let";
            break;
        }
        case VariableDeclarationKind::VAR: {
            kind = "var";
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    dumper->Add({{"type", "VariableDeclaration"},
                 {"declarations", Declarators()},
                 {"kind", kind},
                 {"annotations", AstDumper::Optional(Annotations())},
                 {"declare", AstDumper::Optional(IsDeclare())}});
}

void VariableDeclaration::Dump(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);

    if (IsDeclare()) {
        dumper->Add("declare ");
    }

    switch (Kind()) {
        case VariableDeclarationKind::CONST:
            dumper->Add("const ");
            break;
        case VariableDeclarationKind::LET:
            dumper->Add("let ");
            break;
        case VariableDeclarationKind::VAR:
            dumper->Add("var ");
            break;
        default:
            ES2PANDA_UNREACHABLE();
    }

    for (auto declarator : Declarators()) {
        declarator->Dump(dumper);
        if (declarator != Declarators().back()) {
            dumper->Add(", ");
        }
    }

    auto const parent = Parent();
    if ((parent != nullptr) &&
        (parent->IsBlockStatement() || parent->IsBlockExpression() || parent->IsSwitchCaseStatement())) {
        dumper->Add(";");
    }
}

VariableDeclaration::VariableDeclaration([[maybe_unused]] Tag const tag, VariableDeclaration const &other,
                                         ArenaAllocator *const allocator)
    : AnnotationAllowed<Statement>(static_cast<AnnotationAllowed<Statement> const &>(other)),
      kind_(other.kind_),
      declarators_(allocator->Adapter())
{
    for (auto const &d : other.declarators_) {
        auto *dClone = d->Clone(allocator, nullptr);
        ES2PANDA_ASSERT(dClone != nullptr);
        declarators_.emplace_back(dClone->AsVariableDeclarator());
        declarators_.back()->SetParent(this);
    }

    InitHistory();
}

VariableDeclaration::VariableDeclaration([[maybe_unused]] Tag const tag, VariableDeclaration const &other,
                                         ArenaAllocator *const allocator, AstNodeHistory *history)
    : AnnotationAllowed<Statement>(static_cast<AnnotationAllowed<Statement> const &>(other)),
      kind_(other.kind_),
      declarators_(allocator->Adapter())
{
    for (auto const &d : other.declarators_) {
        declarators_.emplace_back(d->Clone(allocator, nullptr)->AsVariableDeclarator());
        declarators_.back()->SetParent(this);
    }

    if (history != nullptr) {
        history_ = history;
    } else {
        InitHistory();
    }
}

VariableDeclaration *VariableDeclaration::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const clone = allocator->New<VariableDeclaration>(Tag {}, *this, allocator);
    ES2PANDA_ASSERT(clone != nullptr);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    // Clone annotations if any
    if (HasAnnotations()) {
        clone->SetAnnotations(Annotations());
    }

    clone->SetRange(Range());
    return clone;
}

void VariableDeclaration::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void VariableDeclaration::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *VariableDeclaration::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType VariableDeclaration::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

VariableDeclaration *VariableDeclaration::Construct(ArenaAllocator *allocator)
{
    ArenaVector<VariableDeclarator *> declarators(allocator->Adapter());
    return allocator->New<VariableDeclaration>(VariableDeclarationKind::LET, allocator, std::move(declarators));
}

void VariableDeclaration::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsVariableDeclaration();

    otherImpl->kind_ = kind_;
    otherImpl->declarators_ = declarators_;
    AnnotationAllowed<Statement>::CopyTo(other);
}

}  // namespace ark::es2panda::ir
