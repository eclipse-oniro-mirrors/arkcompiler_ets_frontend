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

//
// desc: Object index access syntax is translated to the call of special setter (in case of assignment):
//       "obj[i] = val; => obj.S_set(i, val);"
//   	 or getter (in all the other cases):
//   	 "...obj[i]... => ...obj.S_get(i)..."
//      methods.
//

#include "objectIndexAccess.h"

#include "checker/ETSchecker.h"
#include "compiler/lowering/util.h"
#include "parser/ETSparser.h"

namespace ark::es2panda::compiler {
static ir::AstNode *ProcessIndexSetAccess(public_lib::Context *ctx, ir::AssignmentExpression *assignmentExpression)
{
    //  Note! We assume that parser and checker phase nave been already passed correctly, thus the class has
    //  required accessible index method[s] and all the types are properly resolved.

    auto *const parser = ctx->parser->AsETSParser();
    auto *const checker = ctx->GetChecker()->AsETSChecker();
    auto *const memberExpression = assignmentExpression->Left()->AsMemberExpression();

    static std::string const CALL_EXPRESSION =
        std::string {"@@E1."} + std::string {compiler::Signatures::SET_INDEX_METHOD} + "(@@E2, @@E3)";

    ir::Expression *loweringResult = parser->CreateFormattedExpression(
        CALL_EXPRESSION, memberExpression->Object(), memberExpression->Property(), assignmentExpression->Right());

    ES2PANDA_ASSERT(loweringResult != nullptr);
    loweringResult->SetParent(assignmentExpression->Parent());
    loweringResult->SetRange(assignmentExpression->Range());
    loweringResult->AddModifier(ir::ModifierFlags::ARRAY_SETTER);
    auto scope = varbinder::LexicalScope<varbinder::Scope>::Enter(checker->VarBinder(),
                                                                  NearestScope(assignmentExpression->Parent()));
    CheckLoweredNode(checker->VarBinder()->AsETSBinder(), checker, loweringResult);
    loweringResult->SetParent(assignmentExpression->Parent());
    loweringResult->AddModifier(ir::ModifierFlags::SETTER);
    return loweringResult;
}

static ir::AstNode *ProcessIndexGetAccess(public_lib::Context *ctx, ir::MemberExpression *memberExpression)
{
    auto *const parser = ctx->parser->AsETSParser();
    auto *const checker = ctx->GetChecker()->AsETSChecker();

    //  Note! We assume that parser and checker phase nave been already passed correctly, thus the class has
    //  required accessible index method[s] and all the types are properly resolved.
    static std::string const CALL_EXPRESSION =
        std::string {"@@E1."} + std::string {Signatures::GET_INDEX_METHOD} + "(@@E2)";

    // Parse ArkTS code string and create and process corresponding AST node(s)
    auto *const loweringResult =
        parser->CreateFormattedExpression(CALL_EXPRESSION, memberExpression->Object(), memberExpression->Property());
    loweringResult->AddModifier(ir::ModifierFlags::GETTER);
    loweringResult->SetParent(memberExpression->Parent());
    loweringResult->SetRange(memberExpression->Range());

    CheckLoweredNode(checker->VarBinder()->AsETSBinder(), checker, loweringResult);
    return loweringResult;
}

bool ObjectIndexLowering::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    const auto isGetSetExpression = [](const ir::MemberExpression *const memberExpr) {
        return memberExpr->Kind() == ir::MemberExpressionKind::ELEMENT_ACCESS && memberExpr->ObjType() != nullptr;
    };

    program->Ast()->TransformChildrenRecursively(
        [ctx, &isGetSetExpression](ir::AstNode *const ast) {
            if (ast->IsAssignmentExpression() && ast->AsAssignmentExpression()->Left()->IsMemberExpression()) {
                const auto *const memberExpr = ast->AsAssignmentExpression()->Left()->AsMemberExpression();
                if (isGetSetExpression(memberExpr)) {
                    return ProcessIndexSetAccess(ctx, ast->AsAssignmentExpression());
                }
            }
            return ast;
        },
        Name());

    program->Ast()->TransformChildrenRecursively(
        [ctx, &isGetSetExpression](ir::AstNode *const ast) {
            if (ast->IsMemberExpression()) {
                auto *const memberExpr = ast->AsMemberExpression();
                if (isGetSetExpression(memberExpr)) {
                    return ProcessIndexGetAccess(ctx, memberExpr);
                }
            }
            return ast;
        },
        Name());

    return true;
}

bool ObjectIndexLowering::PostconditionForModule([[maybe_unused]] public_lib::Context *ctx,
                                                 const parser::Program *program)
{
    return !program->Ast()->IsAnyChild([](const ir::AstNode *ast) {
        if (ast->IsMemberExpression() &&
            ast->AsMemberExpression()->Kind() == ir::MemberExpressionKind::ELEMENT_ACCESS) {
            if (auto const *const objectType = ast->AsMemberExpression()->ObjType(); objectType != nullptr) {
                return true;
            }
        }
        return false;
    });
}

}  // namespace ark::es2panda::compiler
