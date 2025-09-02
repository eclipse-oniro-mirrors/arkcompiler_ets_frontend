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

#include "etsIntrinsicNode.h"

#include "checker/ETSchecker.h"
#include "compiler/core/ETSGen.h"

namespace ark::es2panda::ir {

ETSIntrinsicNode::ETSIntrinsicNode(ETSIntrinsicNode const &other, ArenaAllocator *const allocator)
    : Expression(static_cast<Expression const &>(other)), arguments_(allocator->Adapter())
{
    type_ = other.type_;
    for (auto *const arg : other.arguments_) {
        arguments_.emplace_back(arg->Clone(allocator, this)->AsExpression());
    }
}

void ETSIntrinsicNode::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    for (auto *&args : arguments_) {
        if (auto *transformedNode = cb(args); args != transformedNode) {
            args->SetTransformedNode(transformationName, transformedNode);
            args = static_cast<TypeNode *>(transformedNode);
        }
    }
}

void ETSIntrinsicNode::Iterate(const NodeTraverser &cb) const
{
    for (auto *arg : arguments_) {
        cb(arg);
    }
}

void ETSIntrinsicNode::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ETSIntrinsicNode"}, {"arguments", arguments_}});
}

void ETSIntrinsicNode::Dump([[maybe_unused]] ir::SrcDumper *dumper) const
{
    // Note (daizihan): #27074, make it more scalable when IntrinsicNodeType is extended.
    if (type_ == IntrinsicNodeType::TYPE_REFERENCE) {
        dumper->Add("__intrin_type_reference(");
        for (auto arg : arguments_) {
            arg->Dump(dumper);
            if (arg != arguments_.back()) {
                dumper->Add(", ");
            }
        }
        dumper->Add(")");
    }
}

void ETSIntrinsicNode::Compile([[maybe_unused]] compiler::PandaGen *pg) const {}

void ETSIntrinsicNode::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ETSIntrinsicNode::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return nullptr;
}

checker::VerifiedType ETSIntrinsicNode::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

ETSIntrinsicNode *ETSIntrinsicNode::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    ETSIntrinsicNode *clone = allocator->New<ir::ETSIntrinsicNode>(allocator);
    clone->type_ = type_;
    if (parent != nullptr) {
        clone->SetParent(parent);
    }
    clone->SetRange(Range());
    return clone;
}
}  // namespace ark::es2panda::ir