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

#include "lateInitialization.h"

#include <sstream>

#include "checker/ETSchecker.h"
#include "compiler/lowering/util.h"
#include "ir/ets/etsUnionType.h"

namespace ark::es2panda::compiler {

using AstNodePtr = ir::AstNode *;

ir::ClassProperty *TransformerClassProperty(public_lib::Context *ctx, ir::ClassProperty *property)
{
    auto checker = ctx->GetChecker()->AsETSChecker();
    auto allocator = ctx->allocator;
    // Note: This code will be excluded after primitive type refactoring
    if (property->TsType()->IsETSPrimitiveType()) {
        return property;
    }

    auto annotationType = checker->CreateETSUnionType({property->TsType(), checker->GlobalETSUndefinedType()});
    auto typeAnnotation = allocator->New<ir::OpaqueTypeNode>(annotationType, allocator);
    ES2PANDA_ASSERT(typeAnnotation);
    typeAnnotation->SetParent(property);
    typeAnnotation->SetTsType(annotationType);
    property->SetTypeAnnotation(typeAnnotation);
    property->SetTsType(annotationType);
    property->Key()->Variable()->SetTsType(annotationType);
    property->ClearModifier(ir::ModifierFlags::DEFINITE);
    return property;
}

static ir::AstNode *TransformerMemberExpression(ir::MemberExpression *memberExpr, public_lib::Context *ctx)
{
    auto checker = ctx->GetChecker()->AsETSChecker();
    auto parser = ctx->parser->AsETSParser();
    auto varbinder = ctx->GetChecker()->VarBinder()->AsETSBinder();
    auto allocator = ctx->Allocator();
    auto originalType = memberExpr->TsType();
    // Note: This code will be excluded after primitive type refactoring
    if (originalType->IsETSPrimitiveType()) {
        return memberExpr;
    }

    auto parent = memberExpr->Parent();
    if (parent->IsAssignmentExpression() && parent->AsAssignmentExpression()->Left() == memberExpr &&
        parent->AsAssignmentExpression()->OperatorType() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        auto maybeBoxType = checker->MaybeBoxType(originalType);
        memberExpr->SetTsType(maybeBoxType);
        parent->AsAssignmentExpression()->SetTsType(maybeBoxType);
        return memberExpr;
    }

    ArenaVector<ir::Statement *> blockStatements(allocator->Adapter());
    std::stringstream ss;
    auto name = GenName(allocator);
    auto *typeNode = allocator->New<ir::OpaqueTypeNode>(originalType, allocator);

    blockStatements.push_back(parser->CreateFormattedStatement("let @@I1 = @@E2", name, memberExpr));
    ss << "if (@@I1 === undefined) { throw new NullPointerError(); }";
    blockStatements.push_back(parser->CreateFormattedStatement(ss.str(), name));
    blockStatements.push_back(parser->CreateFormattedStatement("@@I1 as @@T2", name, typeNode));
    auto *res = util::NodeAllocator::ForceSetParent<ir::BlockExpression>(allocator, std::move(blockStatements));
    ES2PANDA_ASSERT(res);
    res->SetParent(parent);
    Recheck(ctx->phaseManager, varbinder, checker, res);

    return res;
}

bool LateInitializationConvert::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        [ctx](ir::AstNode *node) -> AstNodePtr {
            if (node->IsMemberExpression()) {
                auto property = node->AsMemberExpression()->Property();
                if (!(property->IsIdentifier() && property->AsIdentifier()->Variable() != nullptr)) {
                    return node;
                }
                auto decl = property->AsIdentifier()->Variable()->Declaration();
                if (decl != nullptr && decl->Node() != nullptr && decl->Node()->IsClassProperty() &&
                    decl->Node()->IsDefinite()) {
                    return TransformerMemberExpression(node->AsMemberExpression(), ctx);
                }
            }
            return node;
        },
        Name());

    program->Ast()->TransformChildrenRecursively(
        [ctx](ir::AstNode *node) -> AstNodePtr {
            if (node->IsClassProperty() && node->IsDefinite()) {
                return TransformerClassProperty(ctx, node->AsClassProperty());
            }
            return node;
        },
        Name());
    return true;
}
}  // namespace ark::es2panda::compiler
