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

#include "referenceTypeAnnotationIsNull.h"

#include "ir/expressions/identifier.h"
#include "ir/expressions/assignmentExpression.h"

namespace ark::es2panda::compiler::ast_verifier {

CheckResult ReferenceTypeAnnotationIsNull::operator()(const ir::AstNode *ast)
{
    if (!ast->IsIdentifier()) {
        return {CheckDecision::CORRECT, CheckAction::CONTINUE};
    }

    // We are running AST verifier only for ETS files so it is correct to pass ETS extension here
    auto const *const id = ast->AsIdentifier();
    if (id->IsReference(ScriptExtension::ATS) && id->TypeAnnotation() != nullptr) {
        AddCheckMessage("TYPE_ANNOTATION_NOT_NULLPTR", *ast);
        return {CheckDecision::INCORRECT, CheckAction::CONTINUE};
    }

    return {CheckDecision::CORRECT, CheckAction::CONTINUE};
}

}  // namespace ark::es2panda::compiler::ast_verifier
