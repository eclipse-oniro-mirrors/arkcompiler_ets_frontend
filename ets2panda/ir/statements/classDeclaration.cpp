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

namespace ark::es2panda::ir {

void ClassDeclaration::SetDefinition(ClassDefinition *def)
{
    this->GetOrCreateHistoryNodeAs<ClassDeclaration>()->def_ = def;
}

ClassDeclaration *ClassDeclaration::Construct(ArenaAllocator *allocator)
{
    return allocator->New<ClassDeclaration>(nullptr, allocator);
}

void ClassDeclaration::CopyTo(AstNode *other) const
{
    auto otherImpl = reinterpret_cast<ClassDeclaration *>(other);

    otherImpl->def_ = def_;

    Statement::CopyTo(other);
}

void ClassDeclaration::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    auto const def = Definition();
    if (auto *transformedNode = cb(def); def != transformedNode) {
        def->SetTransformedNode(transformationName, transformedNode);
        SetDefinition(transformedNode->AsClassDefinition());
    }
}

void ClassDeclaration::Iterate(const NodeTraverser &cb) const
{
    auto def = GetHistoryNodeAs<ClassDeclaration>()->def_;
    cb(def);
}

void ClassDeclaration::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ClassDeclaration"}, {"definition", Definition()}});
}

void ClassDeclaration::Dump(ir::SrcDumper *dumper) const
{
    if (dumper->IsDeclgen() && OriginalNode() != nullptr && OriginalNode()->IsTSEnumDeclaration()) {
        OriginalNode()->Dump(dumper);
    } else if (Definition() != nullptr) {
        Definition()->Dump(dumper);
    }
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
