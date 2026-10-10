/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#include "binaryExpression.h"

#include <compiler/core/pandagen.h>
#include <ir/astDump.h>
#include <ir/expressions/privateIdentifier.h>

namespace panda::es2panda::ir {

void BinaryExpression::Iterate(const NodeTraverser &cb) const
{
    cb(left_);
    cb(right_);
}

void BinaryExpression::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", IsLogical() ? "LogicalExpression" : "BinaryExpression"},
                 {"operator", operator_},
                 {"left", left_},
                 {"right", right_}});
}

void BinaryExpression::CompileLogical(compiler::PandaGen *pg) const
{
    compiler::RegScope rs(pg);
    compiler::VReg lhs = pg->AllocReg();

    ASSERT(operator_ == lexer::TokenType::PUNCTUATOR_LOGICAL_AND ||
           operator_ == lexer::TokenType::PUNCTUATOR_LOGICAL_OR ||
           operator_ == lexer::TokenType::PUNCTUATOR_NULLISH_COALESCING);
    auto *skipRight = pg->AllocLabel();
    auto *endLabel = pg->AllocLabel();

    // left -> acc -> lhs -> toboolean -> acc -> bool_lhs
    left_->Compile(pg);
    pg->StoreAccumulator(this, lhs);

    if (operator_ == lexer::TokenType::PUNCTUATOR_LOGICAL_AND) {
        pg->BranchIfFalse(this, skipRight);
    } else if (operator_ == lexer::TokenType::PUNCTUATOR_LOGICAL_OR) {
        pg->BranchIfTrue(this, skipRight);
    } else {
        ASSERT(operator_ == lexer::TokenType::PUNCTUATOR_NULLISH_COALESCING);
        // non-nullish lhs (loose != null; no [[IsHTMLDDA]] on ArkRuntime) keeps the left
        // value, a nullish lhs falls through to the right operand.
        pg->BranchIfNotNullish(this, lhs, skipRight);
    }

    // left is true/false(and/or) then right -> acc
    right_->Compile(pg);
    pg->Branch(this, endLabel);

    // left is false/true(and/or) then lhs -> acc
    pg->SetLabel(this, skipRight);
    pg->LoadAccumulator(this, lhs);
    pg->SetLabel(this, endLabel);
}

void BinaryExpression::CompilePrivateIn(compiler::PandaGen *pg) const
{
    ASSERT(operator_ == lexer::TokenType::KEYW_IN);
    auto name = left_->AsPrivateIdentifier()->Name();
    auto result = pg->Scope()->FindPrivateName(name);

    right_->Compile(pg);
    if (!result.result.isMethod) {
        pg->TestIn(this, result.lexLevel, result.result.slot);
        return;
    }
    // Instance private method check symbol("#method")
    if (!result.result.isStatic) {
        pg->TestIn(this, result.lexLevel, result.result.validateMethodSlot);
        return;
    }
    // Static private method check whether equals the class object
    compiler::RegScope rs(pg);
    compiler::VReg rhs = pg->AllocReg();
    pg->StoreAccumulator(right_, rhs);
    pg->LoadLexicalVar(this, result.lexLevel, result.result.validateMethodSlot);
    pg->Equal(this, rhs);
}

void BinaryExpression::Compile(compiler::PandaGen *pg) const
{
    if (left_->IsPrivateIdentifier()) {
        CompilePrivateIn(pg);
        return;
    }

    if (IsLogical()) {
        CompileLogical(pg);
        return;
    }

    compiler::RegScope rs(pg);
    compiler::VReg lhs = pg->AllocReg();

    left_->Compile(pg);
    pg->StoreAccumulator(right_, lhs);
    right_->Compile(pg);

    pg->Binary(right_, operator_, lhs);
}


void BinaryExpression::UpdateSelf(const NodeUpdater &cb, [[maybe_unused]] binder::Binder *binder)
{
    left_ = std::get<ir::AstNode *>(cb(left_))->AsExpression();
    right_ = std::get<ir::AstNode *>(cb(right_))->AsExpression();
}

}  // namespace panda::es2panda::ir
