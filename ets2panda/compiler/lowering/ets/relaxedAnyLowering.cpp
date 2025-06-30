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
    if (type->IsGradualType()) {
        return false;  // should be removed after Gradual type refactoring
    }

    return type->IsETSAnyType() ||
           !checker->Relation()->IsSupertypeOf(checker->GlobalETSUnionUndefinedNullObject(), type);
}

static ir::Expression *CreateIntrin(public_lib::Context *ctx, std::string_view id, checker::Type *type,
                                    ArenaVector<ir::Expression *> &&args)
{
    auto checker = ctx->GetChecker()->AsETSChecker();
    auto allocator = ctx->Allocator();
    ir::Expression *result = util::NodeAllocator::ForceSetParent<ir::ETSIntrinsicNode>(allocator, id, std::move(args));
    if (type != nullptr && !type->IsTypeError() &&  // #29049: type errors should not appear here
        !checker->Relation()->IsIdenticalTo(type, checker->GlobalETSAnyType())) {
        result = util::NodeAllocator::ForceSetParent<ir::TSAsExpression>(
            allocator, result, allocator->New<ir::OpaqueTypeNode>(type, allocator), false);
    }
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
    if (!IsLoweringCandidate(checker, callee->TsType())) {
        return node;
    }

    auto args = ArenaVector<ir::Expression *>({}, ctx->Allocator()->Adapter());
    args.reserve(node->Arguments().size() + 2U);

    if (callee->IsMemberExpression()) {
        auto prop = callee->AsMemberExpression()->Property();
        prop = callee->AsMemberExpression()->IsComputed() ? prop : IdentifierToLiteral(ctx, prop->AsIdentifier());
        args.insert(args.end(), {callee->AsMemberExpression()->Object(), prop});
        args.insert(args.end(), node->Arguments().begin(), node->Arguments().end());
        return CreateIntrin(ctx, "anycallthis", node->TsType(), std::move(args));
    }

    args.insert(args.begin(), callee);
    args.insert(args.end(), node->Arguments().begin(), node->Arguments().end());
    return CreateIntrin(ctx, "anycall", node->TsType(), std::move(args));
}

static ir::AstNode *LowerOperationIfNeeded(public_lib::Context *ctx, ir::AstNode *node)
{
    auto const setParent = [node](ir::AstNode *res) {
        if (res != node) {
            res->SetParent(node->Parent());
        }
        return res;
    };

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
