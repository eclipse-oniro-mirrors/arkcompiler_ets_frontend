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
#ifndef ES2PANDA_CHECKER_ISOLATED_DECLGEN_CHECKER_H
#define ES2PANDA_CHECKER_ISOLATED_DECLGEN_CHECKER_H

#include <string>
#include "macros.h"
#include "checker/checker.h"

namespace ark::es2panda::declgen {
class IsolatedDeclgenChecker {
public:
    explicit IsolatedDeclgenChecker(util::DiagnosticEngine &diagnosticEngine, const parser::Program &program)
        : diagnosticEngine_(diagnosticEngine), program_(program)
    {
    }
    ~IsolatedDeclgenChecker() = default;

    NO_COPY_SEMANTIC(IsolatedDeclgenChecker);
    NO_MOVE_SEMANTIC(IsolatedDeclgenChecker);

    void Check();

    std::string Check(ir::ScriptFunction *ast);

private:
    void Check(ir::ClassProperty *ast);
    void Check(ir::ArrayExpression *ast);
    void Check(ir::ETSParameterExpression *ast);
    void Check(ir::ExportDefaultDeclaration *ast);

    std::string InferReturnTypeForArgument(ir::ReturnStatement *returnStatement);

    std::string ProcessLiteralArgument(ir::Literal *literal, ir::ReturnStatement *returnStatement);
    std::string ProcessIdentifierArgument(ir::Identifier *identifier, ir::ReturnStatement *returnStatement);
    std::string ProcessConstArrayArgument(ir::ArrayExpression *array, ir::ReturnStatement *returnStatement);
    std::string ProcessNewClassInstanceExpressionArgument(ir::ETSNewClassInstanceExpression *newClassInstanceExpression,
                                                          ir::ReturnStatement *returnStatement);

    void LogError(const diagnostic::DiagnosticKind &diagnostic, const util::DiagnosticMessageParams &diagnosticParams,
                  const lexer::SourcePosition &pos);

private:
    util::DiagnosticEngine &diagnosticEngine_;
    const parser::Program &program_;
};
}  // namespace ark::es2panda::declgen

#endif  // ES2PANDA_CHECKER_ISOLATED_DECLGEN_CHECKER_H