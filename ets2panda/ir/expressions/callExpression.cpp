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

#include "callExpression.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {
void CallExpression::SetArguments(ArenaVector<Expression *> &&argumentsList)
{
    auto &arguments = this->Arguments();
    arguments = std::move(argumentsList);

    for (auto *argument : arguments) {
        argument->SetParent(this);
    }
}

void CallExpression::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    if (auto *transformedNode = cb(callee_); callee_ != transformedNode) {
        callee_->SetTransformedNode(transformationName, transformedNode);
        callee_ = transformedNode->AsExpression();
    }

    if (typeParams_ != nullptr) {
        if (auto *transformedNode = cb(typeParams_); typeParams_ != transformedNode) {
            typeParams_->SetTransformedNode(transformationName, transformedNode);
            typeParams_ = transformedNode->AsTSTypeParameterInstantiation();
        }
    }

    for (auto *&it : VectorIterationGuard(arguments_)) {
        if (auto *transformedNode = cb(it); it != transformedNode) {
            it->SetTransformedNode(transformationName, transformedNode);
            it = transformedNode->AsExpression();
        }
    }

    if (trailingLambdaInfo_.block != nullptr) {
        if (auto *transformedNode = cb(trailingLambdaInfo_.block); trailingLambdaInfo_.block != transformedNode) {
            trailingLambdaInfo_.block->SetTransformedNode(transformationName, transformedNode);
            trailingLambdaInfo_.block = transformedNode->AsBlockStatement();
        }
    }
}

void CallExpression::Iterate(const NodeTraverser &cb) const
{
    cb(callee_);

    if (typeParams_ != nullptr) {
        cb(typeParams_);
    }

    for (auto *it : VectorIterationGuard(arguments_)) {
        cb(it);
    }

    if (trailingLambdaInfo_.block != nullptr) {
        cb(trailingLambdaInfo_.block);
    }
}

void CallExpression::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "CallExpression"},
                 {"callee", callee_},
                 {"arguments", arguments_},
                 {"optional", IsOptional()},
                 {"typeParameters", AstDumper::Optional(typeParams_)}});
    // #22953: trailing block is not handled
}

void CallExpression::Dump(ir::SrcDumper *dumper) const
{
    ES2PANDA_ASSERT(callee_);
    callee_->Dump(dumper);
    if (IsOptional()) {
        dumper->Add("?.");
    }

    if (typeParams_ != nullptr) {
        typeParams_->Dump(dumper);
    }

    dumper->Add("(");
    for (auto arg : arguments_) {
        arg->Dump(dumper);
        if (arg != arguments_.back()) {
            dumper->Add(", ");
        }
    }
    dumper->Add(")");
    if (trailingLambdaInfo_.block != nullptr) {
        if (trailingLambdaInfo_.isBlockInNewLine) {
            dumper->Endl();
        }
        trailingLambdaInfo_.block->Dump(dumper);
    }
}

void CallExpression::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void CallExpression::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *CallExpression::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType CallExpression::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

CallExpression::CallExpression(CallExpression const &other, ArenaAllocator *const allocator)
    : MaybeOptionalExpression(static_cast<MaybeOptionalExpression const &>(other)),
      arguments_(allocator->Adapter()),
      signature_(other.signature_),
      trailingComma_(other.trailingComma_),
      trailingLambdaInfo_({other.trailingLambdaInfo_.block, other.trailingLambdaInfo_.isBlockInNewLine,
                           other.trailingLambdaInfo_.isTrailingCall})
{
    callee_ = other.callee_->Clone(allocator, this)->AsExpression();
    typeParams_ = other.typeParams_ != nullptr ? other.typeParams_->Clone(allocator, this) : nullptr;

    for (auto *const argument : other.arguments_) {
        arguments_.emplace_back(argument->Clone(allocator, this)->AsExpression());
    }

    trailingLambdaInfo_.block = other.trailingLambdaInfo_.block != nullptr
                                    ? other.trailingLambdaInfo_.block->Clone(allocator, this)->AsBlockStatement()
                                    : nullptr;
}

CallExpression *CallExpression::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const clone = allocator->New<CallExpression>(*this, allocator);
    ES2PANDA_ASSERT(clone != nullptr);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    clone->SetRange(Range());
    return clone;
}

void CallExpression::SetTypeParams(TSTypeParameterInstantiation *typeParams) noexcept
{
    typeParams_ = typeParams;
    if (typeParams_ != nullptr) {
        typeParams_->SetParent(this);
    }
}

void CallExpression::SetTrailingBlock(ir::BlockStatement *const block) noexcept
{
    trailingLambdaInfo_.block = block;
    if (trailingLambdaInfo_.block != nullptr) {
        trailingLambdaInfo_.block->SetParent(this);
    }
}

bool CallExpression::IsExtensionAccessorCall()
{
    return (Signature() != nullptr) && (Signature()->Function()->IsExtensionAccessor());
}

bool CallExpression::IsDynamicCall() const noexcept
{
    return Callee()->IsMemberExpression() && Callee()->AsMemberExpression()->PropVar() != nullptr &&
           (Callee()->AsMemberExpression()->Object()->TsType()->IsETSAnyType() ||
            Callee()->AsMemberExpression()->Object()->TsType()->IsGradualType());
}

}  // namespace ark::es2panda::ir
