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

#include "classDeclaration.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"
#include <type_traits>

namespace ark::es2panda::ir {

void ClassDeclaration::SetDefinition(ClassDefinition *def)
{
    this->GetOrCreateHistoryNodeAs<ClassDeclaration>()->def_ = def;
}

ClassDeclaration *ClassDeclaration::Construct(ArenaAllocator *allocator)
{
    return allocator->New<ClassDeclaration>(nullptr, allocator);
}

void ClassDeclaration::EmplaceDecorators(Decorator *decorators)
{
    this->GetOrCreateHistoryNodeAs<ClassDeclaration>()->decorators_.emplace_back(decorators);
}

void ClassDeclaration::ClearDecorators()
{
    this->GetOrCreateHistoryNodeAs<ClassDeclaration>()->decorators_.clear();
}

void ClassDeclaration::SetValueDecorators(Decorator *decorators, size_t index)
{
    auto &arenaVector = this->GetOrCreateHistoryNodeAs<ClassDeclaration>()->decorators_;
    ES2PANDA_ASSERT(arenaVector.size() > index);
    arenaVector[index] = decorators;
}

[[nodiscard]] const ArenaVector<Decorator *> &ClassDeclaration::Decorators()
{
    return this->GetHistoryNodeAs<ClassDeclaration>()->decorators_;
}

[[nodiscard]] ArenaVector<Decorator *> &ClassDeclaration::DecoratorsForUpdate()
{
    return this->GetOrCreateHistoryNodeAs<ClassDeclaration>()->decorators_;
}

void ClassDeclaration::CopyTo(AstNode *other) const
{
    auto otherImpl = reinterpret_cast<ClassDeclaration *>(other);

    otherImpl->def_ = def_;
    otherImpl->decorators_ = decorators_;

    Statement::CopyTo(other);
}

void ClassDeclaration::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    auto const &decorators = Decorators();
    for (size_t ix = 0; ix < decorators.size(); ix++) {
        if (auto *transformedNode = cb(decorators[ix]); decorators[ix] != transformedNode) {
            decorators[ix]->SetTransformedNode(transformationName, transformedNode);
            SetValueDecorators(transformedNode->AsDecorator(), ix);
        }
    }

    auto const def = Definition();
    if (auto *transformedNode = cb(def); def != transformedNode) {
        def->SetTransformedNode(transformationName, transformedNode);
        SetDefinition(transformedNode->AsClassDefinition());
    }
}

void ClassDeclaration::Iterate(const NodeTraverser &cb) const
{
    for (auto *it : VectorIterationGuard(Decorators())) {
        cb(it);
    }

    auto def = GetHistoryNodeAs<ClassDeclaration>()->def_;
    cb(def);
}

void ClassDeclaration::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ClassDeclaration"},
                 {"definition", Definition()},
                 {"decorators", AstDumper::Optional(Decorators())}});
}

void ClassDeclaration::Dump(ir::SrcDumper *dumper) const
{
    if (Definition() != nullptr) {
        Definition()->Dump(dumper);
    }
    // NOTE(nsizov): support decorators when supported in ArkTS
    ES2PANDA_ASSERT(Decorators().empty());
}

void ClassDeclaration::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void ClassDeclaration::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ClassDeclaration::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType ClassDeclaration::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}
}  // namespace ark::es2panda::ir
