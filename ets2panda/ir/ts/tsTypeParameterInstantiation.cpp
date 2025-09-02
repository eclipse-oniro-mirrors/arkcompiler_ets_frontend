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

#include "tsTypeParameterInstantiation.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {
TSTypeParameterInstantiation::TSTypeParameterInstantiation([[maybe_unused]] Tag const tag,
                                                           TSTypeParameterInstantiation const &other,
                                                           ArenaAllocator *const allocator)
    : Expression(static_cast<Expression const &>(other)), params_(allocator->Adapter())
{
    for (auto *param : other.params_) {
        params_.emplace_back(param->Clone(allocator, this));
    }
}

TSTypeParameterInstantiation *TSTypeParameterInstantiation::Clone(ArenaAllocator *const allocator,
                                                                  AstNode *const parent)
{
    auto *const clone = allocator->New<TSTypeParameterInstantiation>(Tag {}, *this, allocator);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }
    clone->SetRange(range_.GetRange());
    return clone;
}

void TSTypeParameterInstantiation::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    for (auto *&it : VectorIterationGuard(params_)) {
        if (auto *transformedNode = cb(it); it != transformedNode) {
            it->SetTransformedNode(transformationName, transformedNode);
            it = static_cast<TypeNode *>(transformedNode);
        }
    }
}

void TSTypeParameterInstantiation::Iterate(const NodeTraverser &cb) const
{
    for (auto *it : VectorIterationGuard(params_)) {
        cb(it);
    }
}

void TSTypeParameterInstantiation::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "TSTypeParameterInstantiation"}, {"params", params_}});
}

void TSTypeParameterInstantiation::Dump(ir::SrcDumper *dumper) const
{
    dumper->Add("<");
    for (auto param : params_) {
        param->Dump(dumper);
        if (param != params_.back()) {
            dumper->Add(", ");
        }
    }
    dumper->Add(">");
}

void TSTypeParameterInstantiation::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}
void TSTypeParameterInstantiation::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *TSTypeParameterInstantiation::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType TSTypeParameterInstantiation::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

}  // namespace ark::es2panda::ir
