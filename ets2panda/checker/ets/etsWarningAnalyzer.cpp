
/**
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

#include "etsWarningAnalyzer.h"

#include "generated/diagnostic.h"
#include "parser/program/program.h"
#include "util/options.h"
#include "ir/expressions/binaryExpression.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/statements/classDeclaration.h"
#include "ir/statements/expressionStatement.h"
#include "ir/statements/blockStatement.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/memberExpression.h"
#include "ir/ets/etsTypeReferencePart.h"
#include "ir/ets/etsTypeReference.h"
#include "ir/base/classDefinition.h"
#include "ir/statements/forOfStatement.h"
#include "ir/statements/variableDeclarator.h"

namespace ark::es2panda::checker {

void ETSWarningAnalyzer::AnalyzeClassDefForFinalModifier(const ir::ClassDefinition *classDef)
{
    ES2PANDA_ASSERT(classDef != nullptr);

    if (program_ == nullptr || classDef->IsFinal() || classDef->IsAbstract() || classDef->IsStatic() ||
        classDef->IsGlobal() || classDef->IsExported() || classDef->HasExportAlias()) {
        return;
    }

    const auto statements = program_->Ast()->Statements();
    for (const auto *it : statements) {
        if (!it->IsClassDeclaration() ||
            classDef->Ident()->Name() == it->AsClassDeclaration()->Definition()->Ident()->Name()) {
            continue;
        }

        const auto *itAsClassDef = it->AsClassDeclaration()->Definition();

        if (!itAsClassDef->IsGlobal()) {
            const auto *superClass = itAsClassDef->Super();

            if (superClass == nullptr) {
                continue;
            }

            if (superClass->IsETSTypeReference() &&
                superClass->AsETSTypeReference()->Part()->GetIdent()->Name() == classDef->Ident()->Name()) {
                return;
            }
        }
    }

    LogWarning(diagnostic::SUGGEST_FINAL_MODIFIER_FOR_CLASS, classDef->Ident()->Start());
}

void ETSWarningAnalyzer::AnalyzeClassMethodForFinalModifier(const ir::MethodDefinition *methodDef,
                                                            const ir::ClassDefinition *classDef)
{
    ES2PANDA_ASSERT(methodDef != nullptr && classDef != nullptr);

    if (methodDef->IsAbstract() || methodDef->IsStatic() || classDef->IsFinal() || program_ == nullptr ||
        methodDef->IsFinal() || methodDef->IsConstructor() || classDef->IsGlobal()) {
        return;
    }

    bool suggestFinal = true;

    const auto statements = program_->Ast()->Statements();
    for (const auto *it : statements) {
        if (!it->IsClassDeclaration() || it->AsClassDeclaration()->Definition()->IsGlobal() ||
            classDef->Ident()->Name() == it->AsClassDeclaration()->Definition()->Ident()->Name()) {
            continue;
        }

        const auto *statementDef = it->AsClassDeclaration()->Definition();
        for (const auto *bodyPart : statementDef->Body()) {
            if (!bodyPart->IsMethodDefinition()) {
                continue;
            }
            static auto classAsETSObject = classDef->TsType()->AsETSObjectType();
            static auto potentialDescendant = statementDef->TsType()->AsETSObjectType();
            if (!potentialDescendant->IsDescendantOf(classAsETSObject)) {
                continue;
            }
            auto signature = ETSChecker::GetSignatureFromMethodDefinition(bodyPart->AsMethodDefinition());
            ES2PANDA_ASSERT(signature != nullptr);
            const util::StringView bodyMethodName = signature->Function()->Id()->Name();
            const auto *func = methodDef->Function();
            ES2PANDA_ASSERT(func != nullptr);
            if (bodyPart->IsOverride() && bodyMethodName != compiler::Signatures::CTOR &&
                bodyMethodName == func->Id()->Name()) {
                suggestFinal = false;
                break;
            }
        }
    }

    if (suggestFinal) {
        LogWarning(diagnostic::SUGGEST_FINAL_MODIFIER_FOR_METHOD, classDef->Ident()->Start());
    }
}

void ETSWarningAnalyzer::ETSWarningSuggestFinal(const ir::AstNode *node)
{
    if (node->IsClassDeclaration() && !program_->NodeContainsETSNolint(node, ETSWarnings::ETS_SUGGEST_FINAL)) {
        if (node->AsClassDeclaration()->Definition()->IsClassDefinition()) {
            AnalyzeClassDefForFinalModifier(node->AsClassDeclaration()->Definition());
        }

        const auto classBody = node->AsClassDeclaration()->Definition()->Body();
        for (const auto *it : classBody) {
            if (it->IsMethodDefinition()) {
                AnalyzeClassMethodForFinalModifier(it->AsMethodDefinition(), node->AsClassDeclaration()->Definition());
            }
        }
    }
    node->Iterate([&](auto *childNode) { ETSWarningSuggestFinal(childNode); });
}

void ETSWarningAnalyzer::CheckTopLevelExpressions(const ir::Expression *expression)
{
    if (expression->IsCallExpression()) {
        const auto exprCallee = expression->AsCallExpression()->Callee();
        lexer::SourcePosition pos = exprCallee->Start();
        if (exprCallee->IsMemberExpression()) {
            pos = exprCallee->AsMemberExpression()->Object()->Start();
            LogWarning(diagnostic::PROHIBIT_TOP_LEVEL_STATEMENTS, pos);
        }
    } else if (expression->IsAssignmentExpression()) {
        const auto assignmentExpr = expression->AsAssignmentExpression();
        LogWarning(diagnostic::PROHIBIT_TOP_LEVEL_STATEMENTS, assignmentExpr->Left()->Start());
    }
}

void ETSWarningAnalyzer::CheckProhibitedTopLevelStatements(const ir::Statement *statement)
{
    switch (statement->Type()) {
        case ir::AstNodeType::ARROW_FUNCTION_EXPRESSION:
        case ir::AstNodeType::FUNCTION_DECLARATION:
        case ir::AstNodeType::SCRIPT_FUNCTION:
        case ir::AstNodeType::ETS_FUNCTION_TYPE:
        case ir::AstNodeType::IMPORT_NAMESPACE_SPECIFIER:
        case ir::AstNodeType::CLASS_DECLARATION:
        case ir::AstNodeType::CLASS_EXPRESSION:
        case ir::AstNodeType::VARIABLE_DECLARATION:
        case ir::AstNodeType::CLASS_DEFINITION:
        case ir::AstNodeType::CLASS_PROPERTY:
            break;
        default:
            LogWarning(diagnostic::PROHIBIT_TOP_LEVEL_STATEMENTS, statement->Start());
            break;
    }
}

void ETSWarningAnalyzer::ETSWarningAnnotationUnusedGenericAliasWarn(const ir::AstNode *node)
{
    if (!node->IsTSTypeAliasDeclaration()) {
        node->Iterate([&](auto *childNode) { ETSWarningAnnotationUnusedGenericAliasWarn(childNode); });
        return;
    }

    auto st = node->AsTSTypeAliasDeclaration();
    for (auto *const param : st->TypeParams()->Params()) {
        const auto *const res = st->TypeAnnotation()->FindChild([&param](const ir::AstNode *const astNode) {
            if (!astNode->IsIdentifier()) {
                return false;
            }

            return param->Name()->AsIdentifier()->Variable() == astNode->AsIdentifier()->Variable();
        });

        if (res == nullptr) {
            util::DiagnosticMessageParams diagnosticParams = {param->Name()->Name()};
            LogWarning(diagnostic::ANNOTATION_UNUSED_GENERIC_ALIAS_WARN, diagnosticParams, param->Start());
            return;
        }
    }
}

void ETSWarningAnalyzer::ETSWarningsProhibitTopLevelStatements(const ir::AstNode *node)
{
    if (!node->IsClassDeclaration() ||
        program_->NodeContainsETSNolint(node, ETSWarnings::ETS_PROHIBIT_TOP_LEVEL_STATEMENTS)) {
        node->Iterate([&](auto *childNode) { ETSWarningsProhibitTopLevelStatements(childNode); });
        return;
    }

    const auto *classDef = node->AsClassDeclaration()->Definition();
    if (!classDef->IsGlobal()) {
        node->Iterate([&](auto *childNode) { ETSWarningsProhibitTopLevelStatements(childNode); });
        return;
    }

    for (const auto *itBody : classDef->Body()) {
        if (!itBody->IsMethodDefinition() || itBody->AsMethodDefinition()->Id() == nullptr ||
            itBody->AsMethodDefinition()->Id()->Name() != compiler::Signatures::INIT_METHOD) {
            continue;
        }
        const auto *func = itBody->AsMethodDefinition()->Function();
        ES2PANDA_ASSERT(func != nullptr);
        for (const auto *statement : func->Body()->AsBlockStatement()->Statements()) {
            if (program_->NodeContainsETSNolint(statement, ETSWarnings::ETS_PROHIBIT_TOP_LEVEL_STATEMENTS)) {
                continue;
            }

            if (!statement->IsExpressionStatement()) {
                CheckProhibitedTopLevelStatements(statement);
                continue;
            }

            // Rewrite this part after fixing AST issue about tiop-level
            CheckTopLevelExpressions(statement->AsExpressionStatement()->GetExpression());
        }
    }
}

void ETSWarningAnalyzer::ETSWarningBoostEqualityStatement(const ir::AstNode *node)
{
    ES2PANDA_ASSERT(node != nullptr);

    if (node->IsBinaryExpression() &&
        !program_->NodeContainsETSNolint(node, ETSWarnings::ETS_BOOST_EQUALITY_STATEMENT)) {
        const auto binExpr = node->AsBinaryExpression();
        if (binExpr->OperatorType() == lexer::TokenType::PUNCTUATOR_EQUAL ||
            binExpr->OperatorType() == lexer::TokenType::PUNCTUATOR_NOT_EQUAL) {
            if (binExpr->Right()->IsNullLiteral() && !binExpr->Left()->IsNullLiteral()) {
                LogWarning(diagnostic::BOOST_EQUALITY_STATEMENT, node->Start());
            }
        }
    }
    node->Iterate([&](auto *childNode) { ETSWarningBoostEqualityStatement(childNode); });
}

void ETSWarningAnalyzer::ETSWarningRemoveAsync(const ir::AstNode *node)
{
    if (node->IsMethodDefinition() && !program_->NodeContainsETSNolint(node, ETSWarnings::ETS_REMOVE_ASYNC)) {
        const auto methodDefinition = node->AsMethodDefinition();
        if (methodDefinition->IsAsync()) {
            LogWarning(diagnostic::REPLACE_ASYNC_FUNCTION_WITH_COROUTINE, methodDefinition->Start());
        }
    }
    node->Iterate([&](auto *childNode) { ETSWarningRemoveAsync(childNode); });
}

void ETSWarningAnalyzer::ETSWarningRemoveLambda(const ir::AstNode *node)
{
    ES2PANDA_ASSERT(node != nullptr);

    if (node->IsArrowFunctionExpression() && !program_->NodeContainsETSNolint(node, ETSWarnings::ETS_REMOVE_LAMBDA)) {
        LogWarning(diagnostic::REPLACE_LAMBDA_FUNCTION_WITH_REGULAR_FUNCTION, node->Start());
    }
    node->Iterate([&](auto *childNode) { ETSWarningRemoveLambda(childNode); });
}

void ETSWarningAnalyzer::LogWarning(const diagnostic::DiagnosticKind &diagnostic,
                                    const lexer::SourcePosition &position) const
{
    this->LogWarning(diagnostic, {}, position);
}

void ETSWarningAnalyzer::LogWarning(const diagnostic::DiagnosticKind &diagnostic,
                                    const util::DiagnosticMessageParams &diagnosticParams,
                                    const lexer::SourcePosition &position) const
{
    diagnosticEngine_.LogDiagnostic(diagnostic, diagnosticParams, position);
}

}  // namespace ark::es2panda::checker
