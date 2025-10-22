/*
 * Copyright (c) 22025 Huawei Device Co., Ltd.
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

#include "etsGenericInstantiatedNode.h"

#include "checker/ETSchecker.h"
#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {

void ETSGenericInstantiatedNode::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    if (auto *transformedNode = cb(expression_); expression_ != transformedNode) {
        expression_->SetTransformedNode(transformationName, transformedNode);
        expression_ = static_cast<TypeNode *>(transformedNode);
    }

    if (auto *transformedNode = cb(typeParams_); typeParams_ != transformedNode) {
        typeParams_->SetTransformedNode(transformationName, transformedNode);
        typeParams_ = transformedNode->AsTSTypeParameterInstantiation();
    }
}

void ETSGenericInstantiatedNode::Iterate(const NodeTraverser &cb) const
{
    cb(expression_);
    cb(typeParams_);
}

void ETSGenericInstantiatedNode::Dump(ir::AstDumper *const dumper) const
{
    dumper->Add({{"type", "ETSGenericInstantiatedNode"}, {"expression", expression_}, {"typeParams", typeParams_}});
}

void ETSGenericInstantiatedNode::Dump(ir::SrcDumper *const dumper) const
{
    expression_->Dump(dumper);
    typeParams_->Dump(dumper);
}

void ETSGenericInstantiatedNode::Compile(compiler::PandaGen *const pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void ETSGenericInstantiatedNode::Compile(compiler::ETSGen *const etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ETSGenericInstantiatedNode::Check(checker::TSChecker *const checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType ETSGenericInstantiatedNode::Check(checker::ETSChecker *const checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

ETSGenericInstantiatedNode *ETSGenericInstantiatedNode::Clone(ArenaAllocator *allocator, AstNode *parent)
{
    auto *const expressionClone =
        expression_ != nullptr ? expression_->Clone(allocator, nullptr)->AsExpression() : nullptr;
    auto *const typeParamsClone = typeParams_ != nullptr ? typeParams_->Clone(allocator, nullptr) : nullptr;
    auto *const clone = allocator->New<ETSGenericInstantiatedNode>(expressionClone, typeParamsClone);
    ES2PANDA_ASSERT(clone);

    if (expressionClone != nullptr) {
        expressionClone->SetParent(clone);
    }

    if (typeParamsClone != nullptr) {
        typeParamsClone->SetParent(clone);
    }

    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    clone->SetRange(Range());
    return clone;
};

}  // namespace ark::es2panda::ir
