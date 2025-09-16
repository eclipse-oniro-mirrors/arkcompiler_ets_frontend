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

#include "tsTypeParameterDeclaration.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"
#include "ir/ts/tsTypeParameter.h"

namespace ark::es2panda::ir {

void TSTypeParameterDeclaration::SetScope(varbinder::LocalScope *source)
{
    this->GetOrCreateHistoryNodeAs<TSTypeParameterDeclaration>()->scope_ = source;
}

void TSTypeParameterDeclaration::SetRequiredParams(size_t source)
{
    this->GetOrCreateHistoryNodeAs<TSTypeParameterDeclaration>()->requiredParams_ = source;
}

void TSTypeParameterDeclaration::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    auto const params = Params();
    for (size_t ix = 0; ix < params.size(); ix++) {
        if (auto *transformedNode = cb(params[ix]); params[ix] != transformedNode) {
            params[ix]->SetTransformedNode(transformationName, transformedNode);
            SetValueParams(transformedNode->AsTSTypeParameter(), ix);
        }
    }
}

void TSTypeParameterDeclaration::Iterate(const NodeTraverser &cb) const
{
    for (auto *it : VectorIterationGuard(Params())) {
        cb(it);
    }
}

void TSTypeParameterDeclaration::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "TSTypeParameterDeclaration"}, {"params", Params()}});
}

void TSTypeParameterDeclaration::Dump(ir::SrcDumper *dumper) const
{
    for (auto param : Params()) {
        param->Dump(dumper);
        if (param != Params().back()) {
            dumper->Add(", ");
        }
    }
}

void TSTypeParameterDeclaration::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}
void TSTypeParameterDeclaration::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *TSTypeParameterDeclaration::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType TSTypeParameterDeclaration::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

TSTypeParameterDeclaration *TSTypeParameterDeclaration::Construct(ArenaAllocator *allocator)
{
    ArenaVector<TSTypeParameter *> params(allocator->Adapter());
    return allocator->New<TSTypeParameterDeclaration>(std::move(params), 0);
}

void TSTypeParameterDeclaration::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsTSTypeParameterDeclaration();

    otherImpl->params_ = params_;
    otherImpl->scope_ = scope_;
    otherImpl->requiredParams_ = requiredParams_;

    Expression::CopyTo(other);
}

}  // namespace ark::es2panda::ir
