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

#include "typeFromLowering.h"

#include "checker/ETSchecker.h"
#include "checker/types/ets/etsTupleType.h"
#include "compiler/lowering/util.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "ir/astNode.h"

namespace ark::es2panda::compiler {

static std::string TypeToString(const std::string &name)
{
    std::string src {"Type.resolve"};
    src += "(\"";
    src += name;
    src += "\") as Type";
    return src;
}

std::string PrimitiveTypeToString(const checker::Type *type, checker::ETSChecker *checker)
{
    switch (checker->TypeKind(type)) {
        case checker::TypeFlag::ETS_BOOLEAN:
            return std::string(compiler::Signatures::BOOLEANTYPE_VAL);
        case checker::TypeFlag::BYTE:
            return std::string(compiler::Signatures::BYTETYPE_VAL);
        case checker::TypeFlag::CHAR:
            return std::string(compiler::Signatures::CHARTYPE_VAL);
        case checker::TypeFlag::SHORT:
            return std::string(compiler::Signatures::SHORTTYPE_VAL);
        case checker::TypeFlag::INT:
            return std::string(compiler::Signatures::INTTYPE_VAL);
        case checker::TypeFlag::LONG:
            return std::string(compiler::Signatures::LONGTYPE_VAL);
        case checker::TypeFlag::FLOAT:
            return std::string(compiler::Signatures::FLOATTYPE_VAL);
        case checker::TypeFlag::DOUBLE:
            return std::string(compiler::Signatures::DOUBLETYPE_VAL);
        case checker::TypeFlag::ETS_VOID:
            return std::string(compiler::Signatures::VOIDTYPE_VAL);
        default:
            ES2PANDA_UNREACHABLE();
    }
}

std::string HandleSpecialTypes(checker::Type *type)
{
    if (type->IsETSUndefinedType()) {
        return std::string(compiler::Signatures::UNDEFINEDTYPE_REF);
    }

    if (type->IsETSNullType()) {
        return std::string(compiler::Signatures::NULLTYPE_REF);
    }

    if (type->IsETSVoidType()) {
        return std::string(compiler::Signatures::VOIDTYPE_VAL);
    }
    return "";
}

static bool IsTypeFrom(ir::Expression *callee)
{
    if (!callee->IsMemberExpression()) {
        return false;
    }

    auto *memberExpr = callee->AsMemberExpression();
    return memberExpr->HasMemberKind(ir::MemberExpressionKind::PROPERTY_ACCESS) &&
           memberExpr->Object()->IsIdentifier() &&
           memberExpr->Object()->AsIdentifier()->Name() == compiler::Signatures::TYPE &&
           memberExpr->Property()->IsIdentifier() &&
           memberExpr->Property()->AsIdentifier()->Name() == compiler::Signatures::FROM;
}

std::string HandleTypeParameter(ir::Expression *param, checker::ETSChecker *checker)
{
    auto *source = param->TsType();
    if (source->IsETSObjectType()) {
        return TypeToString(std::string(source->AsETSObjectType()->AssemblerName()));
    }

    if (source->IsETSPrimitiveType()) {
        return PrimitiveTypeToString(source, checker);
    }

    if (source->IsETSArrayType()) {
        return TypeToString(std::string(compiler::Signatures::STD_CORE_ARRAY));
    }

    return HandleSpecialTypes(source);
}

static ir::Expression *ReplaceTypeFrom(public_lib::Context *ctx, ir::CallExpression *ast)
{
    auto parser = ctx->parser->AsETSParser();
    auto checker = ctx->GetChecker()->AsETSChecker();
    auto *typeParams = ast->AsCallExpression()->TypeParams();

    ES2PANDA_ASSERT(typeParams != nullptr && typeParams->Params().size() == 1);

    auto *param = typeParams->Params().front();
    if (param->IsETSUnionType()) {
        checker->LogError(diagnostic::TYPE_FROM_UNION_TYPE_UNSUPPORTED, {}, ast->Start());
        return ast;
    }

    if (param->IsETSTuple()) {
        checker->LogError(diagnostic::TYPE_FROM_TUPLE_TYPE_UNSUPPORTED, {}, ast->Start());
        return ast;
    }

    if (param->IsETSStringLiteralType()) {
        checker->LogError(diagnostic::TYPE_FROM_STRING_LITERAL_TYPE_UNSUPPORTED, {}, ast->Start());
        return ast;
    }

    std::string src = HandleTypeParameter(param, checker);
    if (src.empty()) {
        checker->LogError(diagnostic::TYPE_FROM_UNRESOLVABLE_TYPE, {}, ast->Start());
        return ast;
    }

    auto loweringResult = parser->CreateExpression(src);
    loweringResult->SetParent(ast->Parent());

    InitScopesPhaseETS::RunExternalNode(loweringResult, checker->VarBinder());
    checker->VarBinder()->AsETSBinder()->ResolveReferencesForScope(loweringResult, NearestScope(loweringResult));
    loweringResult->Check(checker);
    return loweringResult;
}

bool TypeFromLowering::PerformForProgram(parser::Program *program)
{
    using AstNodePtr = ir::AstNode *;
    program->Ast()->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [ctx = Context()](ir::AstNode *ast) -> AstNodePtr {
            if (ast->IsCallExpression() && IsTypeFrom(ast->AsCallExpression()->Callee())) {
                return ReplaceTypeFrom(ctx, ast->AsCallExpression());
            }
            return ast;
        },
        Name());

    return true;
}
}  // namespace ark::es2panda::compiler
