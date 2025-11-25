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

#include "switchStatement.h"

#include "compiler/core/pandagen.h"
#include "compiler/core/ETSGen.h"
#include "checker/TSchecker.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"

namespace ark::es2panda::ir {
void SwitchStatement::SetCases(ArenaVector<SwitchCaseStatement *> &&casesList)
{
    cases_ = std::move(casesList);

    for (auto *cse : cases_) {
        cse->SetParent(this);
    }
}

void SwitchStatement::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    if (auto *transformedNode = cb(discriminant_); discriminant_ != transformedNode) {
        discriminant_->SetTransformedNode(transformationName, transformedNode);
        discriminant_ = transformedNode->AsExpression();
    }

    for (auto *&it : VectorIterationGuard(cases_)) {
        if (auto *transformedNode = cb(it); it != transformedNode) {
            it->SetTransformedNode(transformationName, transformedNode);
            it = transformedNode->AsSwitchCaseStatement();
        }
    }
}

void SwitchStatement::Iterate(const NodeTraverser &cb) const
{
    cb(discriminant_);

    for (auto *it : VectorIterationGuard(cases_)) {
        cb(it);
    }
}

void SwitchStatement::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "SwitchStatement"}, {"discriminant", discriminant_}, {"cases", cases_}});
}

void SwitchStatement::Dump(ir::SrcDumper *dumper) const
{
    ES2PANDA_ASSERT(discriminant_);
    dumper->Add("switch (");
    discriminant_->Dump(dumper);
    dumper->Add(") {");
    if (!cases_.empty()) {
        dumper->IncrIndent();
        dumper->Endl();
        for (auto cs : cases_) {
            cs->Dump(dumper);
            if (cs == cases_.back()) {
                dumper->DecrIndent();
            }
            dumper->Endl();
        }
    }
    dumper->Add("}");
}

void SwitchStatement::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void SwitchStatement::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *SwitchStatement::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType SwitchStatement::Check(checker::ETSChecker *const checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

SwitchStatement *SwitchStatement::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const discriminant = discriminant_->Clone(allocator, nullptr)->AsExpression();
    ArenaVector<SwitchCaseStatement *> cases(allocator->Adapter());

    for (auto currentCase : cases_) {
        cases.push_back(currentCase->Clone(allocator, nullptr));
    }

    auto clone = util::NodeAllocator::ForceSetParent<ir::SwitchStatement>(allocator, discriminant, std::move(cases));

    clone->SetParent(parent);
    clone->SetRange(Range());
    return clone;
}

}  // namespace ark::es2panda::ir
