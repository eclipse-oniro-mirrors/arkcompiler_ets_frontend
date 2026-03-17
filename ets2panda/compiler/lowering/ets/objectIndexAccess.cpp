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

//
// desc: Object index access syntax is translated to the call of special setter (in case of assignment):
//       "obj[i] = val; => obj.S_set(i, val);"
//   	 or getter (in all the other cases):
//   	 "...obj[i]... => ...obj.S_get(i)..."
//      methods.
//

#include "objectIndexAccess.h"

#include "checker/types/ets/etsTupleType.h"
#include "compiler/lowering/util.h"

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
    auto scope = varbinder::LexicalScope<varbinder::Scope>::Enter(checker->VarBinder(),
                                                                  NearestScope(assignmentExpression->Parent()));
    CheckLoweredNode(checker->VarBinder()->AsETSBinder(), checker, loweringResult);
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

static ir::AstNode *ProcessTupleSetAccess(public_lib::Context *ctx, ir::AssignmentExpression *assignmentExpression)
{
    auto *const memberExpression = assignmentExpression->Left()->AsMemberExpression();

    ES2PANDA_ASSERT_POS(memberExpression->Property()->IsNumberLiteral(), memberExpression->Property()->Start());
    auto *const checker = ctx->GetChecker()->AsETSChecker();
    static std::size_t const TUPLE_THRESHOLD = checker->GetGlobalTypesHolder()->VariadicTupleTypeThreshold();

    std::size_t value = memberExpression->Property()->AsNumberLiteral()->Number().GetInt();
    if (value < TUPLE_THRESHOLD) {
        // This case will be processes in 'ProcessTupleGetAccess' method!
        return assignmentExpression;
    }

    value -= TUPLE_THRESHOLD;

    auto const code = "@@E1." + std::string {compiler::Signatures::TUPLE_ARRAY} + '.' +
                      std::string {compiler::Signatures::SET_INDEX_METHOD} + '(' + std::to_string(value) + ", @@E2)";

    auto *loweringResult = ctx->parser->AsETSParser()->CreateFormattedExpression(code, memberExpression->Object(),
                                                                                 assignmentExpression->Right());
    loweringResult->SetParent(assignmentExpression->Parent());
    SetSourceRangesRecursively(loweringResult, assignmentExpression->Range());

    auto *const varbinder = checker->VarBinder()->AsETSBinder();
    auto *scope = NearestScope(assignmentExpression->Parent());
    auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, scope);
    CheckLoweredNode(varbinder, checker, loweringResult);

    return loweringResult;
}

static ir::AstNode *ProcessTupleGetAccess(public_lib::Context *ctx, ir::MemberExpression *memberExpression,
                                          checker::ETSTupleType const *tupleType)
{
    ES2PANDA_ASSERT_POS(memberExpression->Property()->IsNumberLiteral(), memberExpression->Property()->Start());
    auto *const checker = ctx->GetChecker()->AsETSChecker();
    static std::size_t const TUPLE_THRESHOLD = checker->GetGlobalTypesHolder()->VariadicTupleTypeThreshold();

    static std::size_t const CODE_STRING_RESERVE = 64U;
    std::string code {};
    code.reserve(CODE_STRING_RESERVE);

    std::vector<ir::AstNode *> nodes {};
    nodes.emplace_back(memberExpression->Object());

    ir::Expression *loweringResult = nullptr;

    if (std::size_t value = memberExpression->Property()->AsNumberLiteral()->Number().GetInt();
        value < TUPLE_THRESHOLD) {
        code = "@@E1.$" + std::to_string(value);
    } else {
        auto *memberType =
            checker->AllocNode<ir::OpaqueTypeNode>(tupleType->GetTupleTypesList()[value], checker->Allocator());

        value -= TUPLE_THRESHOLD;

        code = "@@E1." + std::string {compiler::Signatures::TUPLE_ARRAY} + '.' +
               std::string {compiler::Signatures::GET_INDEX_METHOD} + '(' + std::to_string(value) + ") as @@T2";

        nodes.emplace_back(memberType);
    }

    loweringResult = ctx->parser->AsETSParser()->CreateFormattedExpression(code, nodes);
    ;
    loweringResult->SetParent(memberExpression->Parent());
    loweringResult->SetRange(memberExpression->Property()->Range());

    auto *const varbinder = checker->VarBinder()->AsETSBinder();
    auto *scope = NearestScope(memberExpression->Parent());
    auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, scope);
    CheckLoweredNode(varbinder, checker, loweringResult);

    return loweringResult;
}

bool ObjectIndexLowering::PerformForProgram(parser::Program *program)
{
    const auto isGetSetExpression = [](const ir::MemberExpression *const memberExpr) {
        return memberExpr->Kind() == ir::MemberExpressionKind::ELEMENT_ACCESS && memberExpr->ObjType() != nullptr;
    };

    const auto isTupleAccess =
        [](const ir::MemberExpression *const memberExpr) -> std::optional<checker::ETSTupleType const *> {
        if (memberExpr->Kind() == ir::MemberExpressionKind::ELEMENT_ACCESS) {
            auto *type = memberExpr->Object()->TsType();
            while (type->IsETSTypeAliasType()) {
                type = type->AsETSTypeAliasType()->GetTargetType();
            }
            if (type->IsETSTupleType()) {
                return std::make_optional(type->AsETSTupleType());
            }
        }
        return std::nullopt;
    };

    program->Ast()->TransformChildrenRecursively(
        [ctx = Context(), &isGetSetExpression, &isTupleAccess](ir::AstNode *const ast) {
            if (ast->IsAssignmentExpression() && ast->AsAssignmentExpression()->Left()->IsMemberExpression()) {
                const auto *const memberExpr = ast->AsAssignmentExpression()->Left()->AsMemberExpression();
                if (isGetSetExpression(memberExpr)) {
                    return ProcessIndexSetAccess(ctx, ast->AsAssignmentExpression());
                }

                if (isTupleAccess(memberExpr)) {
                    return ProcessTupleSetAccess(ctx, ast->AsAssignmentExpression());
                }
            }
            return ast;
        },
        Name());

    program->Ast()->TransformChildrenRecursively(
        [ctx = Context(), &isGetSetExpression, &isTupleAccess](ir::AstNode *const ast) {
            if (ast->IsMemberExpression()) {
                auto *const memberExpr = ast->AsMemberExpression();
                if (isGetSetExpression(memberExpr)) {
                    return ProcessIndexGetAccess(ctx, memberExpr);
                }

                if (auto type = isTupleAccess(memberExpr); type.has_value()) {
                    return ProcessTupleGetAccess(ctx, memberExpr, *type);
                }
            }
            return ast;
        },
        Name());

    return true;
}

bool ObjectIndexLowering::PostconditionForProgram(const parser::Program *program)
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
