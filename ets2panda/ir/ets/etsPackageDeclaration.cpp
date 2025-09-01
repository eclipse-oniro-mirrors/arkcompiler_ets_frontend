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

#include "etsPackageDeclaration.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {
void ETSPackageDeclaration::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    if (auto *transformedNode = cb(name_); name_ != transformedNode) {
        name_->SetTransformedNode(transformationName, transformedNode);
        name_ = transformedNode->AsExpression();
    }
}

void ETSPackageDeclaration::Iterate([[maybe_unused]] const NodeTraverser &cb) const
{
    cb(name_);
}

void ETSPackageDeclaration::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ETSPackageDeclaration"}, {"name", name_}});
}

void ETSPackageDeclaration::Dump(ir::SrcDumper *dumper) const
{
    if (dumper->IsDeclgen()) {
        return;
    }
    dumper->Add("package ");
    name_->Dump(dumper);
    dumper->Add(";");
    dumper->Endl();
}

void ETSPackageDeclaration::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}
void ETSPackageDeclaration::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ETSPackageDeclaration::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType ETSPackageDeclaration::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

ETSPackageDeclaration *ETSPackageDeclaration::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto const name = name_ != nullptr ? name_->Clone(allocator, nullptr)->AsExpression() : nullptr;
    auto *const clone = allocator->New<ETSPackageDeclaration>(name);
    ES2PANDA_ASSERT(clone);

    if (name != nullptr) {
        name->SetParent(clone);
    }
    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    return clone;
}
}  // namespace ark::es2panda::ir
