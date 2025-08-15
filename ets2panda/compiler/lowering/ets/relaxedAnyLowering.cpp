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

#include "relaxedAnyLowering.h"
#include "ir/expressions/memberExpression.h"

namespace ark::es2panda::compiler {

static bool IsLoweringCandidate(checker::ETSChecker *checker, checker::Type *type)
{
    if (type == nullptr) {  // #29049: nullptr types should not appear here
        return false;
    }
    if (type->IsTypeError()) {  // #29049: type errors should not appear here
        return false;
    }
    if (type->IsETSMethodType() || type->IsETSExtensionFuncHelperType()) {
        return false;  // synthetic types don't represent values
    }
    if (type->IsETSObjectType() && type->AsETSObjectType()->IsGradual()) {
        return true;  // enum-BaseEnum case
    }

    return type->IsETSAnyType() ||
           !checker->Relation()->IsSupertypeOf(checker->GlobalETSUnionUndefinedNullObject(), type);
}

static ir::Expression *InsertTypeGuard(public_lib::Context *ctx, checker::Type *type, ir::Expression *expr)
{
    auto checker = ctx->GetChecker()->AsETSChecker();
    auto allocator = ctx->Allocator();

    if (type == nullptr || type->IsTypeError()) {  // #29049: type errors should not appear here
        return expr;
    }
    if (type->IsETSMethodType()) {  // bug, should not be the case of gradual types
        type = type->AsETSFunctionType()->MethodToArrow(checker);
    }
    if (checker->Relation()->IsIdenticalTo(type, checker->GlobalETSAnyType())) {
        return expr;
    }
    return util::NodeAllocator::ForceSetParent<ir::TSAsExpression>(
        allocator, expr, allocator->New<ir::OpaqueTypeNode>(type, allocator), false);
}

static ir::Expression *CreateIntrin(public_lib::Context *ctx, std::string_view id, checker::Type *type,
                                    ArenaVector<ir::Expression *> &&args)
{
    auto checker = ctx->GetChecker()->AsETSChecker();
    auto allocator = ctx->Allocator();
    auto result = InsertTypeGuard(
        ctx, type, util::NodeAllocator::ForceSetParent<ir::ETSIntrinsicNode>(allocator, id, std::move(args)));

    result->Check(checker);
    return result;
}

template <typename... Args>
static ir::Expression *CreateIntrin(public_lib::Context *ctx, std::string_view id, checker::Type *type, Args &&...args)
{
    return CreateIntrin(ctx, id, type, ArenaVector<ir::Expression *>({args...}, ctx->Allocator()->Adapter()));
}

static ir::StringLiteral *IdentifierToLiteral(public_lib::Context *ctx, ir::Identifier *id)
{
    return ctx->Allocator()->New<ir::StringLiteral>(id->Name());
}

static ir::AstNode *TransformMemberExpression(public_lib::Context *ctx, ir::MemberExpression *node)
{
    auto checker = ctx->GetChecker()->AsETSChecker();
    if (!IsLoweringCandidate(checker, node->Object()->TsType())) {
        return node;
    }

    if (!node->IsComputed()) {
        auto prop = IdentifierToLiteral(ctx, node->Property()->AsIdentifier());
        return CreateIntrin(ctx, "anyldbyname", node->TsType(), node->Object(), prop);
    }
    return CreateIntrin(ctx, node->Property()->TsType()->IsBuiltinNumeric() ? "anyldbyidx" : "anyldbyval",
                        node->TsType(), node->Object(), node->Property());
}

static ir::AstNode *TransformStorePattern(public_lib::Context *ctx, ir::AssignmentExpression *node)
{
    auto checker = ctx->GetChecker()->AsETSChecker();

    auto me = node->Left()->AsMemberExpression();
    if (!IsLoweringCandidate(checker, me->Object()->TsType())) {
        return node;
    }

    if (!me->IsComputed()) {
        auto prop = IdentifierToLiteral(ctx, me->Property()->AsIdentifier());
        return CreateIntrin(ctx, "anystbyname", node->TsType(), me->Object(), prop, node->Right());
    }
    return CreateIntrin(ctx, me->Property()->TsType()->IsBuiltinNumeric() ? "anystbyidx" : "anystbyval", node->TsType(),
                        me->Object(), me->Property(), node->Right());
}

static ir::AstNode *TransformCallExpression(public_lib::Context *ctx, ir::CallExpression *node)
{
    auto checker = ctx->GetChecker()->AsETSChecker();

    auto const callee = node->Callee();
    if (callee->IsMemberExpression()) {
        if (!IsLoweringCandidate(checker, callee->AsMemberExpression()->Object()->TsType())) {
            return node;
        }
        auto prop = callee->AsMemberExpression()->Property();
        prop = callee->AsMemberExpression()->IsComputed() ? prop : IdentifierToLiteral(ctx, prop->AsIdentifier());

        auto args = ArenaVector<ir::Expression *>({}, ctx->Allocator()->Adapter());
        args.reserve(node->Arguments().size() + 2U);
        args.insert(args.end(), {callee->AsMemberExpression()->Object(), prop});
        args.insert(args.end(), node->Arguments().begin(), node->Arguments().end());
        return CreateIntrin(ctx, "anycallthis", node->TsType(), std::move(args));
    }

    if (!IsLoweringCandidate(checker, callee->TsType())) {
        return node;
    }

    auto args = ArenaVector<ir::Expression *>({}, ctx->Allocator()->Adapter());
    args.reserve(node->Arguments().size() + 1U);
    args.insert(args.begin(), callee);
    args.insert(args.end(), node->Arguments().begin(), node->Arguments().end());
    return CreateIntrin(ctx, "anycall", node->TsType(), std::move(args));
}

static ir::Expression *TransformTypeExpressionPattern(public_lib::Context *ctx, ir::Expression *expr)
{
    ir::Expression *typeref = expr->IsETSTypeReference() ? expr->AsETSTypeReference()->Part()->Name() : expr;

    std::vector<ir::Identifier *> names;
    while (typeref->IsTSQualifiedName()) {
        names.push_back(typeref->AsTSQualifiedName()->Right());
        typeref = typeref->AsTSQualifiedName()->Left();
    }
    names.push_back(typeref->AsMemberExpression()->Property()->AsIdentifier());

    ir::Expression *val = typeref->AsMemberExpression()->Object();
    for (auto it = names.rbegin(); it != names.rend(); ++it) {
        val = CreateIntrin(ctx, "anyldbyname", nullptr, val, IdentifierToLiteral(ctx, *it));
    }
    return val;
}

static bool IsTypeExpressionType(checker::Type *type)
{
    return type->IsETSObjectType();
}

static ir::AstNode *TransformNewExpression(public_lib::Context *ctx, ir::ETSNewClassInstanceExpression *node)
{
    auto checker = ctx->GetChecker()->AsETSChecker();

    auto const typeNode = node->GetTypeRef();
    if (!IsLoweringCandidate(checker, typeNode->TsType()) || !IsTypeExpressionType(typeNode->TsType())) {
        return node;
    }

    auto args = ArenaVector<ir::Expression *>({}, ctx->Allocator()->Adapter());
    args.reserve(node->GetArguments().size() + 2U);

    args.insert(args.begin(), TransformTypeExpressionPattern(ctx, typeNode));
    args.insert(args.end(), node->GetArguments().begin(), node->GetArguments().end());
    return CreateIntrin(ctx, "anycallnew", node->TsType(), std::move(args));
}

static ir::AstNode *TransformInstanceofExpression(public_lib::Context *ctx, ir::BinaryExpression *node)
{
    ES2PANDA_ASSERT(node->OperatorType() == lexer::TokenType::KEYW_INSTANCEOF);
    auto checker = ctx->GetChecker()->AsETSChecker();

    auto const typeNode = node->Right();
    if (!IsLoweringCandidate(checker, typeNode->TsType()) || !IsTypeExpressionType(typeNode->TsType())) {
        return node;
    }
    return CreateIntrin(ctx, "anyisinstance", nullptr, node->Left(), TransformTypeExpressionPattern(ctx, typeNode));
}

static ir::AstNode *LowerOperationIfNeeded(public_lib::Context *ctx, ir::AstNode *node)
{
    auto const setParent = [node](ir::AstNode *res) {
        if (res != node) {
            res->SetParent(node->Parent());
        }
        return res;
    };

    if (node->IsETSNewClassInstanceExpression()) {
        return setParent(TransformNewExpression(ctx, node->AsETSNewClassInstanceExpression()));
    }
    if (node->IsBinaryExpression() && node->AsBinaryExpression()->OperatorType() == lexer::TokenType::KEYW_INSTANCEOF) {
        return setParent(TransformInstanceofExpression(ctx, node->AsBinaryExpression()));
    }
    if (node->IsCallExpression()) {
        return setParent(TransformCallExpression(ctx, node->AsCallExpression()));
    }
    if (node->IsAssignmentExpression() && node->AsAssignmentExpression()->Left()->IsMemberExpression()) {
        return setParent(TransformStorePattern(ctx, node->AsAssignmentExpression()));
    }
    if (node->IsMemberExpression()) {
        return setParent(TransformMemberExpression(ctx, node->AsMemberExpression()));
    }
    return node;
}

bool RelaxedAnyLoweringPhase::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    program->Ast()->TransformChildrenRecursivelyPreorder(
        [ctx](ir::AstNode *node) { return LowerOperationIfNeeded(ctx, node); }, Name());

    return true;
}

}  // namespace ark::es2panda::compiler
