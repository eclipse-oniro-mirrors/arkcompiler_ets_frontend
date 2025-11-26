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

#include "functionExpression.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {
void FunctionExpression::SetFunction(ScriptFunction *func)
{
    func_ = func;

    if (func != nullptr) {
        func->SetParent(this);
    }
}

void FunctionExpression::SetId(ir::Identifier *id)
{
    exprName_ = id;

    if (id != nullptr) {
        id->SetParent(this);
    }
}

void FunctionExpression::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    if (auto *transformedNode = cb(func_); func_ != transformedNode) {
        func_->SetTransformedNode(transformationName, transformedNode);
        func_ = transformedNode->AsScriptFunction();
    }
}

void FunctionExpression::Iterate(const NodeTraverser &cb) const
{
    cb(func_);
}

void FunctionExpression::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "FunctionExpression"}, {"function", func_}});
}

void FunctionExpression::Dump(ir::SrcDumper *dumper) const
{
    func_->Dump(dumper);
}

void FunctionExpression::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void FunctionExpression::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *FunctionExpression::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType FunctionExpression::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

FunctionExpression *FunctionExpression::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    ES2PANDA_ASSERT(func_->Clone(allocator, nullptr) != nullptr);
    auto *const func = func_->Clone(allocator, nullptr)->AsScriptFunction();
    auto *const clone = allocator->New<FunctionExpression>(func);
    ES2PANDA_ASSERT(clone != nullptr);
    func->SetParent(clone);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }
    clone->SetRange(Range());
    return clone;
}

AstNode *FunctionExpression::Construct(ArenaAllocator *allocator)
{
    return allocator->New<FunctionExpression>(nullptr);
}

void FunctionExpression::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsFunctionExpression();

    otherImpl->func_ = func_;
    otherImpl->exprName_ = exprName_;

    Expression::CopyTo(other);
}
}  // namespace ark::es2panda::ir
