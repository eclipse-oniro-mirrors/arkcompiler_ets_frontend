/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "etsDestructuring.h"

#include "checker/ETSchecker.h"
#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {

void ETSDestructuring::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    auto const &elements = Elements();
    for (size_t ix = 0; ix < elements.size(); ix++) {
        if (auto *transformedNode = cb(elements[ix]); elements[ix] != transformedNode) {
            elements[ix]->SetTransformedNode(transformationName, transformedNode);
            SetValueTypes(static_cast<Expression *>(transformedNode), ix);
        }
    }
}

void ETSDestructuring::Iterate(const NodeTraverser &cb) const
{
    for (auto *expr : GetHistoryNodeAs<ETSDestructuring>()->elements_) {
        cb(expr);
    }
}

void ETSDestructuring::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ETSDestructuring"}, {"elements", Elements()}});
}

void ETSDestructuring::Dump(ir::SrcDumper *dumper) const
{
    dumper->Add("[");
    for (auto *param : Elements()) {
        param->Dump(dumper);
        if (param != Elements().back()) {
            dumper->Add(", ");
        }
    }
    dumper->Add("]");
}

void ETSDestructuring::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void ETSDestructuring::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ETSDestructuring::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType ETSDestructuring::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

ETSDestructuring *ETSDestructuring::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    ArenaVector<Expression *> varList(allocator->Adapter());

    for (auto *const param : Elements()) {
        varList.emplace_back(param->Clone(allocator, nullptr)->AsExpression());
    }

    auto *const clone = allocator->New<ETSDestructuring>(varList);

    for (auto *param : clone->Elements()) {
        param->SetParent(clone);
    }

    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    return clone;
}
}  // namespace ark::es2panda::ir
