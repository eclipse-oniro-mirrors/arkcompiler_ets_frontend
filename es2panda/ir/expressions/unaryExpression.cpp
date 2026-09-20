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

#include "unaryExpression.h"

#include <compiler/core/pandagen.h>
#include <ir/astDump.h>
#include <ir/expressions/identifier.h>
#include <ir/expressions/literals/bigIntLiteral.h>
#include <ir/expressions/literals/numberLiteral.h>
#include <ir/expressions/memberExpression.h>

namespace panda::es2panda::ir {

void UnaryExpression::Iterate(const NodeTraverser &cb) const
{
    cb(argument_);
}

void UnaryExpression::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "UnaryExpression"}, {"operator", operator_}, {"prefix", true}, {"argument", argument_}});
}

void UnaryExpression::Compile(compiler::PandaGen *pg) const
{
    switch (operator_) {
        case lexer::TokenType::KEYW_DELETE: {
            if (argument_->IsIdentifier()) {
                binder::ScopeFindResult result = pg->Scope()->Find(argument_->AsIdentifier()->Name());
                if (!result.variable || (result.scope->IsGlobalScope() && result.variable->IsGlobalVariable())) {
                    compiler::RegScope rs(pg);
                    compiler::VReg variable = pg->AllocReg();
                    compiler::VReg global = pg->AllocReg();

                    pg->LoadConst(this, compiler::Constant::JS_GLOBAL);
                    pg->StoreAccumulator(this, global);

                    pg->LoadAccumulatorString(this, argument_->AsIdentifier()->Name());
                    pg->StoreAccumulator(this, variable);

                    pg->DeleteObjProperty(this, global, variable);
                } else {
                    // Otherwise it is a local variable which can't be deleted and we just
                    // return false.
                    pg->LoadConst(this, compiler::Constant::JS_FALSE);
                }
            } else if (argument_->IsMemberExpression()) {
                compiler::RegScope rs(pg);
                compiler::VReg object = pg->AllocReg();

                argument_->AsMemberExpression()->CompileObject(pg, object);
                compiler::Operand prop = argument_->AsMemberExpression()->CompileKey(pg);

                pg->DeleteObjProperty(this, object, prop);
            } else {
                // compile the delete operand.
                argument_->Compile(pg);
                // Deleting any value or a result of an expression returns True.
                pg->LoadConst(this, compiler::Constant::JS_TRUE);
            }
            break;
        }
        case lexer::TokenType::KEYW_TYPEOF: {
            if (argument_->IsIdentifier()) {
                const ir::Identifier *ident = argument_->AsIdentifier();

                binder::ScopeFindResult res = pg->Scope()->Find(ident->Name());
                if (!res.variable && !pg->isDebuggerEvaluateExpressionMode()) {
                    compiler::RegScope rs(pg);
                    compiler::VReg global = pg->AllocReg();

                    pg->LoadConst(this, compiler::Constant::JS_GLOBAL);
                    pg->StoreAccumulator(this, global);
                    pg->LoadObjByName(this, global, ident->Name());
                } else if (!res.variable && pg->isDebuggerEvaluateExpressionMode()) {
                    // false: typeof an undeclared variable will return undefined
                    pg->LoadObjByNameViaDebugger(this, ident->Name(), false);
                } else {
                    pg->LoadVar(ident, res);
                }
            } else {
                argument_->Compile(pg);
            }

            pg->TypeOf(this);
            break;
        }
        case lexer::TokenType::KEYW_VOID: {
            argument_->Compile(pg);
            pg->LoadConst(this, compiler::Constant::JS_UNDEFINED);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK: {
            // ISFALSE is exactly the negation operation; emitting it directly
            // avoids the dead operand store and the branch-based boolean
            // materialization of the generic unary path.
            argument_->Compile(pg);
            pg->IsFalse(this);
            break;
        }
        default: {
            argument_->Compile(pg);

            compiler::RegScope rs(pg);
            compiler::VReg operandReg = pg->AllocReg();
            pg->StoreAccumulator(this, operandReg);
            pg->Unary(this, operator_, operandReg);
            break;
        }
    }
}


void UnaryExpression::UpdateSelf(const NodeUpdater &cb, [[maybe_unused]] binder::Binder *binder)
{
    argument_ = std::get<ir::AstNode *>(cb(argument_))->AsExpression();
}

}  // namespace panda::es2panda::ir
