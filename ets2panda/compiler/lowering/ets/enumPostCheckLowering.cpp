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

#include "enumPostCheckLowering.h"
#include <cstddef>
#include <string>
#include "checker/ETSchecker.h"
#include "checker/types/type.h"
#include "checker/types/ets/etsEnumType.h"
#include "compiler/lowering/util.h"
#include "ir/astNode.h"
#include "ir/expressions/identifier.h"
#include "ir/statements/ifStatement.h"
#include "ir/ts/tsAsExpression.h"
#include "libarkbase/macros.h"
#include "parser/ETSparser.h"
#include "util.h"
#include "util/ustring.h"
#include "varbinder/ETSBinder.h"
#include "varbinder/variable.h"

namespace ark::es2panda::compiler {

static ir::ClassDeclaration *FindEnclosingClass(ir::AstNode *ast)
{
    for (ir::AstNode *curr = ast->Parent(); curr != nullptr; curr = curr->Parent()) {
        if (curr->IsClassDeclaration()) {
            return curr->AsClassDeclaration();
        }
    }
    ES2PANDA_UNREACHABLE();
}

static std::string TypeToString(checker::Type *type)
{
    std::stringstream ss;
    type->ToString(ss);
    return ss.str();
}

static util::StringView TypeAnnotationToString(ir::ETSTypeReference *typeAnnotation, public_lib::Context *ctx)
{
    std::stringstream ss;
    std::function<void(ir::AstNode *)> typeAnnoToStringImpl = [&ss, &typeAnnoToStringImpl](ir::AstNode *node) -> void {
        if (node->IsIdentifier()) {
            ss << node->AsIdentifier()->ToString() << ".";
        }
        node->Iterate(typeAnnoToStringImpl);
    };
    typeAnnotation->Iterate(typeAnnoToStringImpl);
    std::string res = ss.str();
    res.erase(res.size() - 1);
    return util::UString {res, ctx->Allocator()}.View();
}

static bool IsValidEnumCasting(checker::Type *type, EnumCastType castType)
{
    switch (castType) {
        case EnumCastType::NONE: {
            return false;
        }
        case EnumCastType::CAST_TO_STRING: {
            return type->IsETSStringEnumType();
        }
        case EnumCastType::CAST_TO_INT:
        case EnumCastType::CAST_TO_LONG:
        case EnumCastType::CAST_TO_DOUBLE:
        case EnumCastType::CAST_TO_FLOAT:
        case EnumCastType::CAST_TO_BYTE:
        case EnumCastType::CAST_TO_SHORT: {
            return type->IsETSNumericEnumType();
        }
        case EnumCastType::CAST_TO_NUMERIC_ENUM:
        case EnumCastType::CAST_TO_STRING_ENUM: {
            return true;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

static EnumCastType NeedHandleEnumCasting(ir::TSAsExpression *node)
{
    auto type = node->TsType();
    EnumCastType castType = EnumCastType::NONE;
    if (type == nullptr) {
        return castType;
    }
    if (type->IsETSStringType()) {
        castType = EnumCastType::CAST_TO_STRING;
    } else if (type->HasTypeFlag(checker::TypeFlag::ETS_NUMERIC) || type->IsBuiltinNumeric()) {
        castType = EnumCastType::CAST_TO_INT;
    } else if (type->IsETSEnumType()) {
        castType =
            type->IsETSNumericEnumType() ? EnumCastType::CAST_TO_NUMERIC_ENUM : EnumCastType::CAST_TO_STRING_ENUM;
    } else {
        return castType;
    }
    auto expr = node->Expr();
    if (expr->TsType()->IsETSUnionType()) {
        for (auto *ct : expr->TsType()->AsETSUnionType()->ConstituentTypes()) {
            if (ct->IsETSEnumType() && IsValidEnumCasting(ct, castType)) {
                return castType;
            }
        }
    }
    return IsValidEnumCasting(expr->TsType(), castType) ? castType : EnumCastType::NONE;
}

static ir::CallExpression *CallStaticEnumMethod(public_lib::Context *ctx, parser::ETSParser *parser,
                                                std::string_view className, std::string_view methodName,
                                                ir::Expression *argument)
{
    auto classId = parser->CreateExpression(className);
    auto methodId = ctx->AllocNode<ir::Identifier>(methodName, ctx->Allocator());
    auto callee = ctx->AllocNode<ir::MemberExpression>(classId, methodId, ir::MemberExpressionKind::PROPERTY_ACCESS,
                                                       false, false);
    ArenaVector<ir::Expression *> callArguments({argument}, ctx->Allocator()->Adapter());
    return ctx->AllocNode<ir::CallExpression>(callee, std::move(callArguments), nullptr, false);
}

static ir::CallExpression *CallInstanceEnumMethod(public_lib::Context *ctx, std::string_view methodName,
                                                  ir::Expression *thisArg)
{
    auto methodId = ctx->AllocNode<ir::Identifier>(methodName, ctx->Allocator());
    auto callee = ctx->AllocNode<ir::MemberExpression>(thisArg, methodId, ir::MemberExpressionKind::PROPERTY_ACCESS,
                                                       false, false);

    ArenaVector<ir::Expression *> callArguments({}, ctx->Allocator()->Adapter());
    auto callExpr = ctx->AllocNode<ir::CallExpression>(callee, std::move(callArguments), nullptr, false);
    callExpr->SetRange(thisArg->Range());
    return callExpr;
}

static ir::CallExpression *CreateCallInstanceEnumExpression(public_lib::Context *ctx, ir::AstNode *const node,
                                                            std::string_view methodName)
{
    ES2PANDA_ASSERT(node->IsExpression());
    auto expr = node->AsExpression();
    auto parent = expr->Parent();

    auto *callExpr = CallInstanceEnumMethod(ctx, methodName, expr);
    callExpr->SetParent(parent);

    auto *calleeClass = FindEnclosingClass(expr);

    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *varBinder = checker->VarBinder()->AsETSBinder();

    auto *nearestScope = NearestScope(parent);
    auto lexScope = varbinder::LexicalScope<varbinder::Scope>::Enter(varBinder, nearestScope);
    varBinder->ResolveReferencesForScopeWithContext(callExpr, nearestScope);

    auto checkerCtx = checker::SavedCheckerContext(checker, checker::CheckerStatus::IN_CLASS,
                                                   calleeClass->Definition()->TsType()->AsETSObjectType());
    auto scopeCtx = checker::ScopeContext(checker, nearestScope);

    callExpr->Check(checker);
    return callExpr;
}

[[nodiscard]] static ir::ETSTypeReference *MakeTypeReference(public_lib::Context *ctx, const std::string &name)
{
    auto allocator = ctx->Allocator();
    auto *const ident = ctx->AllocNode<ir::Identifier>(util::UString(name, allocator).View(), allocator);
    auto *const referencePart = ctx->AllocNode<ir::ETSTypeReferencePart>(ident, allocator);
    return ctx->AllocNode<ir::ETSTypeReference>(referencePart, allocator);
}

static ir::IfStatement *CreateEnumIfStatement(public_lib::Context *ctx, ir::Identifier *ident,
                                              const std::string &enumName, ir::Statement *consequent)
{
    auto enumType = MakeTypeReference(ctx, enumName);
    auto clonedIdent = ident->Clone(ctx->Allocator(), nullptr);
    auto ifTestExpr = ctx->AllocNode<ir::BinaryExpression>(clonedIdent, enumType, lexer::TokenType::KEYW_INSTANCEOF);
    return ctx->AllocNode<ir::IfStatement>(ifTestExpr, consequent, nullptr);
}

ir::Statement *EnumPostCheckLoweringPhase::CreateStatement(const std::string &src, ir::Expression *ident,
                                                           ir::Expression *init)
{
    std::vector<ir::AstNode *> nodes;
    if (ident != nullptr) {
        nodes.push_back(ident->Clone(context_->Allocator(), nullptr));
    }

    if (init != nullptr) {
        nodes.push_back(init->Clone(context_->Allocator(), nullptr));
    }

    auto statements = parser_->CreateFormattedStatements(src, nodes);
    if (!statements.empty()) {
        return *statements.begin();
    }

    return nullptr;
}

ir::Expression *EnumPostCheckLoweringPhase::HandleEnumTypeCasting(checker::Type *type, ir::Expression *expr,
                                                                  ir::TSAsExpression *tsAsExpr)
{
    ir::Expression *transformedExpr = nullptr;
    // Generate fromValue call;
    if (type->IsETSEnumType()) {
        auto exprType = expr->TsType();
        if (exprType->IsETSEnumType() || exprType->IsETSAnyType() ||
            (exprType->IsETSObjectType() && exprType->AsETSObjectType()->IsGlobalETSObjectType())) {
            return expr;
        }
        auto name = TypeAnnotationToString(tsAsExpr->TypeAnnotation()->AsETSTypeReference(), context_);
        transformedExpr = GenerateFromValueCall(expr, name);
    } else {
        transformedExpr = CallInstanceEnumMethod(context_, checker::ETSEnumType::VALUE_OF_METHOD_NAME, expr);
    }
    transformedExpr->SetRange(expr->Range());
    transformedExpr->SetParent(expr->Parent());
    return transformedExpr;
}

// CC-OFFNXT(huge_method,G.FUD.05)
void EnumPostCheckLoweringPhase::CreateStatementForUnionConstituentType(EnumCastType castType, ir::Identifier *ident,
                                                                        checker::Type *type,
                                                                        ir::TSAsExpression *tsAsExpr,
                                                                        ArenaVector<ir::Statement *> &statements)
{
    auto createInstanceOfStatement = [this, &statements, &ident, &type](ir::Expression *callExpr) {
        auto consequent = CreateStatement("@@I1 = @@E2;", ident, callExpr);
        auto ifStatement = CreateEnumIfStatement(context_, ident, TypeToString(type), consequent);
        auto prevStatement = statements.back();
        if (prevStatement != nullptr && prevStatement->IsIfStatement()) {
            prevStatement->AsIfStatement()->SetAlternate(ifStatement);
        }
        statements.push_back(ifStatement);
    };
    switch (castType) {
        case EnumCastType::CAST_TO_STRING: {
            if (type->IsETSStringEnumType()) {
                auto callExpr = CallInstanceEnumMethod(context_, checker::ETSEnumType::VALUE_OF_METHOD_NAME,
                                                       ident->Clone(context_->Allocator(), nullptr)->AsExpression());
                callExpr->SetRange(tsAsExpr->Expr()->Range());
                createInstanceOfStatement(callExpr);
            }
            break;
        }
        case EnumCastType::CAST_TO_INT: {
            if (type->IsETSNumericEnumType()) {
                auto callExpr = CallInstanceEnumMethod(context_, checker::ETSEnumType::VALUE_OF_METHOD_NAME,
                                                       ident->Clone(context_->Allocator(), nullptr)->AsExpression());
                callExpr->SetRange(tsAsExpr->Expr()->Range());
                createInstanceOfStatement(callExpr);
            }
            break;
        }
        case EnumCastType::CAST_TO_NUMERIC_ENUM: {
            // int and Boxed Int can be casted to int enum
            if (type->IsIntType() || (type->IsETSObjectType() &&
                                      type->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::BUILTIN_INT))) {
                auto name = TypeAnnotationToString(tsAsExpr->TypeAnnotation()->AsETSTypeReference(), context_);
                auto callExpr =
                    GenerateFromValueCall(ident->Clone(context_->Allocator(), nullptr)->AsExpression(), name);
                callExpr->SetRange(tsAsExpr->Expr()->Range());
                createInstanceOfStatement(callExpr);
            }
            break;
        }
        case EnumCastType::CAST_TO_STRING_ENUM: {
            if (type->IsETSStringType()) {
                auto name = TypeAnnotationToString(tsAsExpr->TypeAnnotation()->AsETSTypeReference(), context_);
                auto callExpr =
                    GenerateFromValueCall(ident->Clone(context_->Allocator(), nullptr)->AsExpression(), name);
                callExpr->SetRange(tsAsExpr->Expr()->Range());
                createInstanceOfStatement(callExpr);
            }
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

ir::Expression *EnumPostCheckLoweringPhase::HandleUnionTypeForCalls(checker::ETSUnionType *unionType,
                                                                    ir::Expression *expr, ir::TSAsExpression *tsAsExpr,
                                                                    EnumCastType castType)
{
    /*
     * For given union type:  number | string | Enum | otherTypes, this method generate instanceof trees to ensuring
     * all union constituent types are handled correctly with enum related casting.
     */
    auto *const allocator = context_->Allocator();
    auto *genSymIdent = Gensym(allocator);
    ArenaVector<ir::Statement *> statements(allocator->Adapter());
    const std::string createSrc = "let @@I1 = @@E2";
    statements.push_back(CreateStatement(createSrc, genSymIdent, expr));
    for (auto type : unionType->ConstituentTypes()) {
        CreateStatementForUnionConstituentType(castType, genSymIdent, type, tsAsExpr, statements);
    }
    statements.push_back(CreateStatement("@@I1", genSymIdent, nullptr));
    auto block = context_->AllocNode<ir::BlockExpression>(std::move(statements));
    return block;
}

ir::AstNode *EnumPostCheckLoweringPhase::GenerateEnumCasting(ir::TSAsExpression *node, EnumCastType castType)
{
    auto expr = node->Expr();
    if (expr->TsType()->IsETSUnionType()) {
        auto newExpr = HandleUnionTypeForCalls(expr->TsType()->AsETSUnionType(), node->Expr(), node, castType);
        node->SetExpr(newExpr);
    } else {
        auto newExpr = HandleEnumTypeCasting(node->TsType(), node->Expr(), node);
        node->SetExpr(newExpr);
    }
    node->SetTsType(nullptr);
    auto *scope = NearestScope(node);
    auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder_, scope);
    CheckLoweredNode(varbinder_, checker_, node);
    return node;
}

static auto *InlineValueOf(ir::MemberExpression *enumMemberRef, ArenaAllocator *allocator)
{
    auto key = enumMemberRef->Property()->AsIdentifier()->Name().Utf8();
    auto enumType = enumMemberRef->TsType()->AsETSEnumType();
    auto ord = enumType->GetOrdinalFromMemberName(key);
    auto origLiteral = enumType->GetValueLiteralFromOrdinal(ord);
    auto literal = origLiteral->Clone(allocator, enumMemberRef->Parent())->AsExpression();
    literal->SetTsType(origLiteral->TsType());
    return literal;
}

ir::AstNode *EnumPostCheckLoweringPhase::GenerateValueOfCall(ir::AstNode *const node)
{
    node->RemoveAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
    if (!node->IsExpression()) {
        return node;
    }
    // NOTE: temporary workaround
    // Need to find out why ETSParameterExpression has GENERATE_VALUE_OF flag
    // Need to be refactored after complete rework on overload resolution
    if (node->IsETSParameterExpression()) {
        return node;
    }
    node->Parent()->AddAstNodeFlags(ir::AstNodeFlags::RECHECK);
    if (node->AsExpression()->TsType()->IsTypeError()) {
        return node;
    }
    auto *enumNode = node->AsExpression()->TsType()->AsETSEnumType();

    if (enumNode->NodeIsEnumLiteral(node->AsExpression()) && enumNode->EnumAnnotedType() == nullptr) {
        return InlineValueOf(node->AsMemberExpression(), context_->Allocator());
    }
    auto *callExpr = CreateCallInstanceEnumExpression(context_, node, checker::ETSEnumType::VALUE_OF_METHOD_NAME);
    return callExpr;
}

ir::Expression *EnumPostCheckLoweringPhase::GenerateFromValueCall(ir::Expression *const node, util::StringView name)
{
    auto *callExpr =
        CallStaticEnumMethod(context_, parser_, name.Utf8(), checker::ETSEnumType::FROM_VALUE_METHOD_NAME, node);
    callExpr->SetRange(node->Range());
    return callExpr;
}

ir::SwitchStatement *EnumPostCheckLoweringPhase::GenerateGetOrdinalCallForSwitch(ir::SwitchStatement *const node)
{
    node->AddAstNodeFlags(ir::AstNodeFlags::RECHECK);
    auto *discrminant =
        CreateCallInstanceEnumExpression(context_, node->Discriminant(), checker::ETSEnumType::GET_ORDINAL_METHOD_NAME);
    node->SetDiscriminant(discrminant);

    for (auto *ele : node->Cases()) {
        // Default case will not handle.
        if (ele->Test() == nullptr) {
            continue;
        }
        auto *newTest =
            CreateCallInstanceEnumExpression(context_, ele->Test(), checker::ETSEnumType::GET_ORDINAL_METHOD_NAME);
        ele->SetTest(newTest);
    }
    return node;
}

static void RecheckNode(ir::AstNode *node, checker::ETSChecker *checker)
{
    // No parent class means that this node is not in the inheritance of the class
    auto *parentClass = util::Helpers::FindAncestorGivenByType(node, ir::AstNodeType::CLASS_DEFINITION);
    if (parentClass == nullptr) {
        return;
    }
    if (node->IsExpression()) {
        node->AsExpression()->SetTsType(nullptr);  // force recheck
    }
    checker::SavedCheckerContext savedContext(checker, checker->Context().Status(),
                                              parentClass->AsClassDefinition()->TsType()->AsETSObjectType());
    node->RemoveAstNodeFlags(ir::AstNodeFlags::RECHECK);
    node->Check(checker);

    if (node->IsExpression() && node->AsExpression()->TsType() != nullptr &&
        !node->AsExpression()->TsType()->IsETSNumericEnumType()) {
        node->RemoveAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
    }
}

ir::AstNode *EnumPostCheckLoweringPhase::BuildEnumCasting(ir::AstNode *const node)
{
    auto castFlag = NeedHandleEnumCasting(node->AsTSAsExpression());
    if (castFlag == EnumCastType::NONE) {
        return node;
    }
    return GenerateEnumCasting(node->AsTSAsExpression(), castFlag);
};

bool EnumPostCheckLoweringPhase::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    if (program->Extension() != ScriptExtension::ETS) {
        return true;
    }

    context_ = ctx;
    parser_ = ctx->parser->AsETSParser();
    checker_ = ctx->GetChecker()->AsETSChecker();
    varbinder_ = ctx->parserProgram->VarBinder()->AsETSBinder();

    program->Ast()->TransformChildrenRecursivelyPostorder(
        // clang-format off
        [this](ir::AstNode *const node) -> ir::AstNode* {
            if (node->HasAstNodeFlags(ir::AstNodeFlags::RECHECK)) {
                RecheckNode(node, checker_);
            }
            if (node->HasAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF)) {
                auto newNode = node;
                if (node->IsTSAsExpression()) {
                    newNode = BuildEnumCasting(newNode);
                }
                return GenerateValueOfCall(newNode);
            }
            if (node->IsTSAsExpression()) {
                return BuildEnumCasting(node);
            }
            if (node->IsSwitchStatement() && (node->AsSwitchStatement()->Discriminant()->TsType() != nullptr) &&
                node->AsSwitchStatement()->Discriminant()->TsType()->IsETSEnumType()) {
                return GenerateGetOrdinalCallForSwitch(node->AsSwitchStatement());
            }
            return node;
        },
        // clang-format on
        Name());
    return true;
}

}  // namespace ark::es2panda::compiler
