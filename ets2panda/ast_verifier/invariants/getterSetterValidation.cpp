/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include "getterSetterValidation.h"
#include "ir/expression.h"
#include "ir/expressions/functionExpression.h"
#include "ir/typeNode.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"

namespace ark::es2panda::compiler::ast_verifier {

[[nodiscard]] CheckResult GetterSetterValidation::operator()(const ir::AstNode *ast)
{
    if (!ast->IsMethodDefinition()) {
        return {CheckDecision::CORRECT, CheckAction::CONTINUE};
    }

    bool errorFound = false;
    auto const validateMethod = [&errorFound, this](ir::MethodDefinition const *const method) {
        auto const kind = method->Kind();
        if (kind == ir::MethodDefinitionKind::GET) {
            errorFound |= !ValidateGetter(method);
        } else if (kind == ir::MethodDefinitionKind::SET) {
            errorFound |= !ValidateSetter(method);
        };
    };

    auto const *const method = ast->AsMethodDefinition();
    validateMethod(method);
    for (auto const *const overload : method->Overloads()) {
        validateMethod(overload);
    }

    if (errorFound) {
        return {CheckDecision::INCORRECT, CheckAction::CONTINUE};
    }

    return {CheckDecision::CORRECT, CheckAction::CONTINUE};
}

bool GetterSetterValidation::ValidateGetter(ir::MethodDefinition const *const method)
{
    if (!method->Value()->IsFunctionExpression()) {
        return true;
    }

    bool result = true;
    auto const report = [this, &result, method](const std::string &msg) {
        AddCheckMessage(msg, *method);
        result = false;
    };

    auto const *const function = method->Value()->AsFunctionExpression()->Function();

    // Check getter flag
    if (!function->IsGetter()) {
        report("GETTER METHOD DOES NOT HAVE GETTER FLAG");
    }

    // Check number of arguments
    auto const &params = function->Params();
    if (!params.empty()) {
        report("GETTER METHOD HAS INCORRECT NUMBER OF ARGUMENTS");
    }

    return result;
}

bool GetterSetterValidation::ValidateSetter(ir::MethodDefinition const *const method)
{
    if (!method->Value()->IsFunctionExpression()) {
        return true;
    }

    bool result = true;
    auto const report = [this, &result, method](const std::string &msg) {
        AddCheckMessage(msg, *method);
        result = false;
    };

    auto const *const function = method->Value()->AsFunctionExpression()->Function();

    // Check setter flag
    if (!function->IsSetter()) {
        report("SETTER METHOD DOES NOT HAVE SETTER FLAG");
    }

    // Check return type annotation
    if (function->ReturnTypeAnnotation() != nullptr) {
        report("SETTER METHOD MUST NOT HAVE RETURN TYPE");
    }

    // Check number of arguments
    auto const &params = function->Params();
    if (params.size() != 1) {
        report("SETTER METHOD HAS INCORRECT NUMBER OF ARGUMENTS");
    }

    return result;
}

}  // namespace ark::es2panda::compiler::ast_verifier
