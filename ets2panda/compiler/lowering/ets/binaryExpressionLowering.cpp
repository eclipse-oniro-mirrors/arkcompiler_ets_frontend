/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include <sstream>

#include "binaryExpressionLowering.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

using AstNodePtr = ir::AstNode *;

static ir::AstNode *ConvertExponentiation(ir::BinaryExpression *binaryExpr, public_lib::Context *ctx)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *parser = ctx->parser->AsETSParser();
    auto *varbinder = checker->VarBinder()->AsETSBinder();

    std::string const str = "Math.pow(@@E1, @@E2)";
    auto *parent = binaryExpr->Parent();

    ir::Expression *loweringResult = parser->CreateFormattedExpression(str, binaryExpr->Left(), binaryExpr->Right());
    ES2PANDA_ASSERT(loweringResult != nullptr);

    loweringResult->SetParent(parent);
    loweringResult->SetRange(binaryExpr->Range());

    auto *scope = NearestScope(parent);
    auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, scope);
    CheckLoweredNode(varbinder, checker, loweringResult);
    return loweringResult;
}

bool BinaryExpressionLowering::PerformForProgram(parser::Program *program)
{
    program->Ast()->TransformChildrenRecursivelyPostorder(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [ctx = Context()](ir::AstNode *ast) -> AstNodePtr {
            if (ast->IsBinaryExpression()) {
                ir::BinaryExpression *binaryExpr = ast->AsBinaryExpression();
                auto *left = binaryExpr->Left();
                bool leftIsBigInt =
                    (left->TsType() != nullptr && left->TsType()->IsETSBigIntType()) || left->IsBigIntLiteral();
                if (binaryExpr->OperatorType() == lexer::TokenType::PUNCTUATOR_EXPONENTIATION && !leftIsBigInt) {
                    return ConvertExponentiation(binaryExpr, ctx);
                }
            }

            return ast;
        },
        Name());

    return true;
}
}  // namespace ark::es2panda::compiler
