/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "identifierHasVariable.h"
#include "ir/base/scriptFunction.h"
#include "ir/expressions/memberExpression.h"
#include "ir/ts/tsEnumDeclaration.h"
#include "checker/types/ets/etsAnyType.h"

namespace ark::es2panda::compiler::ast_verifier {

class IdentifierHasVariable::ExceptionsMatcher {
public:
    ExceptionsMatcher(const IdentifierHasVariable *inv, const ir::Identifier *ast) : inv_(inv), ast_(ast) {}
    bool Match()
    {
        auto res = IsLengthProp() || IsAnyProp() || IsEmptyName() || IsInObjectExpr() || IsInPackageDecl() ||
                   IsBuiltinType() || IsUnionMemberAccess();
        return res;
    }

private:
    bool IsLengthProp()
    {
        return ast_->Parent() != nullptr && ast_->Parent()->IsMemberExpression() && ast_->Name().Is("length");
    }

    bool IsAnyProp()
    {
        return ast_->Parent() != nullptr && ast_->Parent()->IsMemberExpression() &&
               ast_->Parent()->AsMemberExpression()->Object()->TsType()->IsETSAnyType() &&
               ast_->Parent()->AsMemberExpression()->Object()->TsType()->AsETSAnyType()->IsRelaxed();
    }

    bool IsEmptyName()
    {
        // NOTE(kkonkuznetsov): some identifiers have empty names
        return ast_->Name().Empty();
    }

    bool IsInObjectExpr()
    {
        // NOTE(kkonkuznetsov): object expressions
        const auto *parent = ast_->Parent();
        while (parent != nullptr) {
            if (parent->IsObjectExpression()) {
                return true;
            }

            parent = parent->Parent();
        }
        return false;
    }

    bool IsInPackageDecl()
    {
        // NOTE(kkonkuznetsov): skip package declarations
        const auto *parent = ast_->Parent();
        while (parent != nullptr) {
            if (parent->IsETSPackageDeclaration()) {
                return true;
            }
            parent = parent->Parent();
        }
        return false;
    }

    bool IsBuiltinType()
    {
        auto name = ast_->Name();
        // NOTE(mmartin): find a better solution to handle utility type resolution
        return name.Is(Signatures::PARTIAL_TYPE_NAME) || name.Is(Signatures::REQUIRED_TYPE_NAME) ||
               name.Is(Signatures::READONLY_TYPE_NAME) || name.Is(Signatures::FIXED_ARRAY_TYPE_NAME) ||
               name.Is(compiler::Signatures::ANY_TYPE_NAME) || name.Is(Signatures::ANY) ||
               name.Is(compiler::Signatures::AWAITED_TYPE_NAME) || name.Is(compiler::Signatures::RETURN_TYPE_TYPE_NAME);
    }

    bool IsUnionMemberAccess()
    {
        return ast_->Parent() != nullptr && ast_->Parent()->IsMemberExpression() &&
               ast_->Parent()->AsMemberExpression()->Object()->TsType()->IsETSUnionType() &&
               !inv_->UnionLoweringOccurred();
    }

private:
    const IdentifierHasVariable *inv_ {};
    const ir::Identifier *ast_ {};
};

CheckResult IdentifierHasVariable::operator()(const ir::AstNode *ast)
{
    if (ast->IsScriptFunction() && ast->AsScriptFunction()->IsExternal()) {
        // Identifiers in external functions' definitions are not resolved, so skip it
        ES2PANDA_ASSERT(!ast->AsScriptFunction()->IsExternalOverload());
        return {CheckDecision::CORRECT, CheckAction::SKIP_SUBTREE};
    }
    if (!ast->IsIdentifier()) {
        return {CheckDecision::CORRECT, CheckAction::CONTINUE};
    }

    if (ast->AsIdentifier()->Variable() != nullptr) {
        return {CheckDecision::CORRECT, CheckAction::CONTINUE};
    }

    const auto *id = ast->AsIdentifier();
    if (ExceptionsMatcher {this, id}.Match()) {
        return {CheckDecision::CORRECT, CheckAction::CONTINUE};
    }

    AddCheckMessage("NULL_VARIABLE", *id);
    return {CheckDecision::INCORRECT, CheckAction::CONTINUE};
}

}  // namespace ark::es2panda::compiler::ast_verifier
