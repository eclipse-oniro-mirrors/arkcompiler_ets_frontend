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

#include "etsNonNullishTypeNode.h"
#include "checker/ETSchecker.h"

namespace ark::es2panda::ir {
void ETSNonNullishTypeNode::TransformChildren([[maybe_unused]] const NodeTransformer &cb,
                                              [[maybe_unused]] std::string_view const transformationName)
{
    if (auto *transformedNode = cb(typeNode_); typeNode_ != transformedNode) {
        typeNode_->SetTransformedNode(transformationName, transformedNode);
        typeNode_ = static_cast<TypeNode *>(transformedNode);
    }
}

void ETSNonNullishTypeNode::Iterate([[maybe_unused]] const NodeTraverser &cb) const
{
    cb(typeNode_);
}

void ETSNonNullishTypeNode::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ETSNonNullishType"}, {"typeNode", typeNode_}});
}

void ETSNonNullishTypeNode::Dump(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);
    dumper->Add("NonNullable<");
    typeNode_->Dump(dumper);
    dumper->Add(">");
}

void ETSNonNullishTypeNode::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSNonNullishTypeNode::Check([[maybe_unused]] checker::TSChecker *checker)
{
    ES2PANDA_UNREACHABLE();
}

checker::VerifiedType ETSNonNullishTypeNode::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

checker::Type *ETSNonNullishTypeNode::GetType([[maybe_unused]] checker::ETSChecker *checker)
{
    return TsType() != nullptr ? TsType() : Check(checker);
}

ETSNonNullishTypeNode *ETSNonNullishTypeNode::Clone(ArenaAllocator *allocator, AstNode *parent)
{
    TypeNode *typeNode = typeNode_->Clone(allocator, nullptr);
    ETSNonNullishTypeNode *clone = allocator->New<ir::ETSNonNullishTypeNode>(typeNode, allocator);
    ES2PANDA_ASSERT(clone);
    clone->SetParent(parent);
    clone->typeNode_->SetParent(clone);
    return clone;
}
}  // namespace ark::es2panda::ir
