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

#include "importDeclaration.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"
#include "libarkbase/utils/arena_containers.h"

namespace ark::es2panda::ir {

void ImportDeclaration::SetSource(StringLiteral *source)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ImportDeclaration>();
    newNode->source_ = source;

    if (source) {
        source->SetParent(newNode);
    }
}

void ImportDeclaration::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    auto const source = Source();
    if (auto *transformedNode = cb(source); source != transformedNode) {
        source->SetTransformedNode(transformationName, transformedNode);
        SetSource(transformedNode->AsStringLiteral());
    }

    auto const &specifiers = Specifiers();
    for (size_t index = 0; index < specifiers.size(); ++index) {
        if (auto *transformedNode = cb(specifiers[index]); specifiers[index] != transformedNode) {
            specifiers[index]->SetTransformedNode(transformationName, transformedNode);
            SetValueSpecifiers(transformedNode, index);
        }
    }
}

void ImportDeclaration::Iterate(const NodeTraverser &cb) const
{
    auto source = GetHistoryNodeAs<ImportDeclaration>()->source_;
    cb(source);

    for (auto *it : VectorIterationGuard(Specifiers())) {
        cb(it);
    }
}

void ImportDeclaration::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ImportDeclaration"}, {"source", Source()}, {"specifiers", Specifiers()}});
}

void ImportDeclaration::Dump(ir::SrcDumper *dumper) const
{
    if (dumper->IsDeclgen()) {
        dumper->GetDeclgen()->CollectImport(this);
        return;
    }
    dumper->Add("import ");
    auto const &specifiers = Specifiers();
    if (specifiers.size() == 1 &&
        (specifiers[0]->IsImportNamespaceSpecifier() || specifiers[0]->IsImportDefaultSpecifier())) {
        specifiers[0]->Dump(dumper);
    } else {
        dumper->Add("{ ");
        for (auto specifier : specifiers) {
            specifier->Dump(dumper);
            if (specifier != specifiers.back()) {
                dumper->Add(", ");
            }
        }
        dumper->Add(" }");
    }

    dumper->Add(" from ");

    Source()->Dump(dumper);

    dumper->Add(";");
    dumper->Endl();
}

void ImportDeclaration::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void ImportDeclaration::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ImportDeclaration::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType ImportDeclaration::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

ImportDeclaration *ImportDeclaration::Construct(ArenaAllocator *allocator)
{
    ArenaVector<AstNode *> specifiers(allocator->Adapter());
    return allocator->New<ImportDeclaration>(nullptr, std::move(specifiers));
}

void ImportDeclaration::CopyTo(AstNode *other) const
{
    auto otherImpl = static_cast<ImportDeclaration *>(other);

    otherImpl->source_ = source_;
    otherImpl->specifiers_ = specifiers_;
    otherImpl->importKinds_ = importKinds_;

    Statement::CopyTo(other);
}

void ImportDeclaration::EmplaceSpecifiers(AstNode *source)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ImportDeclaration>();
    newNode->specifiers_.emplace_back(source);
}

void ImportDeclaration::ClearSpecifiers()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ImportDeclaration>();
    newNode->specifiers_.clear();
}

void ImportDeclaration::SetValueSpecifiers(AstNode *source, size_t index)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ImportDeclaration>();
    auto &arenaVector = newNode->specifiers_;
    ES2PANDA_ASSERT(arenaVector.size() > index);
    arenaVector[index] = source;
}

[[nodiscard]] ArenaVector<AstNode *> &ImportDeclaration::SpecifiersForUpdate()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ImportDeclaration>();
    return newNode->specifiers_;
}

}  // namespace ark::es2panda::ir
