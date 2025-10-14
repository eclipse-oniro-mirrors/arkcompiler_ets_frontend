/**
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

/**
 * @file extract_symbol.h/cpp
 * @brief Extract symbol refactor implementation for the ES2Panda LSP.
 *
 * @details
 * This file implements the ExtractSymbolRefactor used by the language server
 * to offer extraction refactorings (extract constant, extract variable,
 * extract function) from a selected source range. It provides:
 *  - ExtractSymbolRefactor class which registers available extract actions
 *    and creates the edits for the selected action.
 *  - Helper routines to locate and validate the AST nodes to extract,
 *    compute insertion positions, build extracted function text, and
 *    generate the corresponding text edits via ChangeTracker.
 *
 * Main components:
 *  - class ExtractSymbolRefactor
 *      Registers extract refactor kinds and exposes:
 *        - GetAvailableActions(const RefactorContext&)
 *        - GetEditsForAction(const RefactorContext&, const std::string&)
 *
 *  - Utility functions:
 *      - std::vector<ir::AstNode *> IsRightNodeInRange(ir::AstNode*, TextRange)
 *          Collects right-side binary-expression nodes that lie fully inside
 *          the provided span (used when combining binary-expr parts).
 *
 *      - bool IsNodeInScope(ir::AstNode*)
 *          Predicate to determine whether a node is unsuitable for extraction
 *          (e.g. literals, member/call expressions, variable declarations).
 *
 *      - TextRange GetCallPositionOfExtraction(const RefactorContext &)
 *          Compute the textual call-site range that should be replaced with
 *          an invocation to the newly-extracted symbol/function.
 *
 *      - TextRange GetVarAndFunctionPosToWriteNode(const RefactorContext &)
 *          Determine a suitable insertion position for generated variable or
 *          function declarations (walks parent nodes to find the block scope).
 *
 *      - ir::AstNode *FindExtractedVals(const RefactorContext &)
 *        ir::AstNode *FindExtractedFunction(const RefactorContext &)
 *          Find the expression/statement/function node corresponding to the
 *          user-selected span to be extracted.
 *
 *      - std::vector<FunctionExtraction> GetPossibleFunctionExtractions(const RefactorContext &)
 *          Enumerate candidate statements/blocks that can be turned into a function.
 *
 *      - void CollectFunctionParameters(FunctionExtraction &)
 *          Collect function parameters from function nodes to build call-site arguments.
 *
 *      - std::string BuildFunctionText(const FunctionExtraction &, const RefactorContext &)
 *          Build the source text for the extracted function (name, params, body).
 *
 *      - std::string ReplaceWithFunctionCall(const FunctionExtraction &, const std::string &)
 *          Build the call-site text that replaces the original code after extraction.
 *
 *      - std::string GenerateInlineEdits(const RefactorContext &, ir::AstNode *)
 *          Generate the declaration (const/let) text for extracted values.
 *
 *      - RefactorEditInfo GetRefactorEditsToExtractVals(const RefactorContext &, ir::AstNode *)
 *        RefactorEditInfo GetRefactorEditsToExtractFunction(const RefactorContext &, ir::AstNode *)
 *          Create and return actual FileTextChanges via ChangeTracker for each refactor.
 *
 * Usage / Notes:
 *  - This module relies on the ES2Panda public context (public_lib::Context),
 *    the parser AST, and the ChangeTracker service to build edits.
 *  - The refactor logic assumes a valid RefactorContext describing the
 *    selection span and editor context; callers should validate that the
 *    context and parser AST are available before calling the entry points.
 *
 *
 * @see refactors/extract_symbol.h
 * @see refactors/refactor_types.h
 * @see services/text_change/change_tracker.h
 */

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>
#include "refactors/extract_symbol.h"
#include "ir/astNode.h"
#include "ir/expressions/identifier.h"
#include "lexer/token/sourceLocation.h"
#include "public/public.h"
#include "refactor_provider.h"
#include "internal_api.h"
#include "services/text_change/change_tracker.h"
#include "refactors/refactor_types.h"
#include "rename.h"
#include "types.h"

namespace ark::es2panda::lsp {

ExtractSymbolRefactor::ExtractSymbolRefactor()
{
    AddKind(std::string(EXTRACT_CONSTANT_ACTION.kind));
    AddKind(std::string(EXTRACT_FUNCTION_ACTION.kind));
    AddKind(std::string(EXTRACT_VARIABLE_ACTION.kind));
}

std::vector<ir::AstNode *> IsRightNodeInRange(ir::AstNode *node, TextRange span)
{
    std::vector<ir::AstNode *> nodeList;
    if (!node->IsBinaryExpression()) {
        return nodeList;
    }
    ir::AstNode *right = node;
    while (right->AsBinaryExpression()->Right() != nullptr && right->Start().index > span.pos &&
           right->End().index < span.end) {
        nodeList.push_back(right);
        right = right->AsBinaryExpression()->Right();
    }
    return nodeList;
}

bool IsNodeInScope(ir::AstNode *node)
{
    return (node != nullptr && !node->IsVariableDeclaration() && !node->IsCallExpression() &&
            !node->IsMemberExpression() && !node->IsExpressionStatement() && !node->IsBinaryExpression() &&
            !node->IsStringLiteral() && !node->IsNumberLiteral() && !node->IsBooleanLiteral() &&
            !node->IsNullLiteral() && !node->IsCharLiteral());
}

TextRange GetCallPositionOfExtraction(const RefactorContext &context)
{
    auto start = context.span.pos;
    auto end = context.span.end;
    const auto startedNode = GetTouchingToken(context.context, start, false);
    const auto endNode = GetTouchingToken(context.context, end, false);
    if (startedNode->Start().index < start && endNode->End().index <= end) {
        start = startedNode->Start().index;
    }
    if (endNode->Start().index >= start && endNode->End().index > end) {
        end = endNode->End().index;
    }
    return {start, end};
}

TextRange GetVarAndFunctionPosToWriteNode(const RefactorContext &context)
{
    auto start = context.span.pos;
    auto startedNode = GetTouchingToken(context.context, start, false);
    while (!startedNode->Parent()->IsBlockStatement() && startedNode->Parent()->IsFunctionExpression() &&
           startedNode->Parent()->IsClassDeclaration() && startedNode->Parent()->IsMethodDefinition()) {
        startedNode = startedNode->Parent();
    }
    return {startedNode->Parent()->Start().index, startedNode->Parent()->End().index};
}

bool IsNodeInRange(ir::AstNode *node, TextRange span)
{
    return node->Parent()->Start().index >= span.pos && node->Parent()->End().index <= span.end;
}

ir::AstNode *FindExtractedVals(const RefactorContext &context)
{
    const auto rangeToExtract = context.span;
    if (rangeToExtract.pos >= rangeToExtract.end) {
        return nullptr;
    }

    const auto ast = reinterpret_cast<public_lib::Context *>(context.context)->parserProgram->Ast();
    if (ast == nullptr) {
        return nullptr;
    }

    auto node = GetTouchingToken(context.context, rangeToExtract.pos, false);
    if (node == nullptr) {
        return nullptr;
    }
    while (node != nullptr &&
           (!node->IsExpression() && !node->IsVariableDeclaration() && !node->IsBinaryExpression())) {
        node = node->Parent();
    }

    if (node != nullptr) {
        return node;
    }
    return nullptr;
}
ir::AstNode *FindExtractedFunction(const RefactorContext &context)
{
    const auto rangeToExtract = context.span;
    if (rangeToExtract.pos >= rangeToExtract.end) {
        return nullptr;
    }
    const auto ast = reinterpret_cast<public_lib::Context *>(context.context)->parserProgram->Ast();
    if (ast == nullptr) {
        return nullptr;
    }
    auto node = GetTouchingToken(context.context, rangeToExtract.pos, false);
    if (node == nullptr) {
        return nullptr;
    }
    while (node != nullptr && (!node->IsExpression() && !node->IsFunctionExpression() &&
                               !node->IsArrowFunctionExpression() && !node->IsStatement())) {
        node = node->Parent();
    }
    if (node != nullptr) {
        return node;
    }
    return nullptr;
}

std::vector<FunctionExtraction> GetPossibleFunctionExtractions(const RefactorContext &context)
{
    std::vector<FunctionExtraction> res;

    if (context.span.pos >= context.span.end) {
        return res;
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr) {
        return res;
    }

    const auto ast = ctx->parserProgram->Ast();
    if (ast == nullptr) {
        return res;
    }
    ast->FindChild([&res](ir::AstNode *node) {
        if (node->IsStatement() || node->IsExpressionStatement() || node->IsBlockStatement()) {
            FunctionExtraction fe;
            fe.node = node;
            fe.targetRange = TextRange {node->Start().index, node->End().index};
            fe.description = "Extract statement(s) to function";
            res.push_back(std::move(fe));
        }
        return false;
    });

    return res;
}

void CollectFunctionParameters(FunctionExtraction &funExt)
{
    const auto node = funExt.node;
    if (node == nullptr) {
        return;
    }
    if (node->IsFunctionDeclaration()) {
        auto *func = node->AsFunctionDeclaration();
        for (auto *param : func->Function()->Params()) {
            if (param->IsETSParameterExpression()) {
                funExt.parameters.push_back(param->AsETSParameterExpression());
            }
        }
    } else if (node->IsFunctionExpression() || node->IsArrowFunctionExpression()) {
        auto *func = node->AsFunctionExpression();
        for (auto *param : func->Function()->Params()) {
            if (param->IsETSParameterExpression()) {
                funExt.parameters.push_back(param->AsETSParameterExpression());
            }
        }
    }
}

bool IsNodeInParamList(ir::Identifier *ident, const std::vector<ir::Identifier *> &list)
{
    if (list.empty()) {
        return false;
    }
    for (auto param : list) {
        if (ident->ToString() == param->ToString()) {
            return true;
        }
    }
    return false;
}
std::string GetParamsText(const FunctionExtraction &candidate, std::vector<ir::Identifier *> functionParams)
{
    std::string params;
    for (size_t i = 0; i < candidate.parameters.size(); i++) {
        bool funcParamInParams = IsNodeInParamList(candidate.parameters[i]->Ident(), functionParams);
        if (!candidate.parameters.empty() && candidate.parameters.size() > i && funcParamInParams) {
            if (i != 0) {
                params += ", ";
            }
            params += candidate.parameters[i]->Ident()->ToString() + " : " +
                      candidate.parameters[i]->TypeAnnotation()->AsETSTypeReference()->BaseName()->Name().Mutf8();
        }
    }
    return params;
}
std::string BuildFunctionText(const FunctionExtraction &candidate, const RefactorContext &context)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr) {
        return "";
    }
    const auto src = ctx->sourceFile->source;
    if (src.empty()) {
        return "";
    }
    const auto ast = ctx->parserProgram->Ast();
    auto extractionPos = GetCallPositionOfExtraction(context);

    size_t start = extractionPos.pos;
    size_t end = extractionPos.end;
    if (start >= src.size() || end > src.size() || start >= end) {
        return "";
    }
    std::vector<ir::Identifier *> functionParams;
    bool needParams = false;
    ast->FindChild([&needParams, &functionParams, &start, &end](ir::AstNode *child) {
        if ((child->Start().index >= start && child->End().index <= end) &&
            (!child->IsStringLiteral() && !child->IsStringLiteral() && !child->IsNumberLiteral() &&
             !child->IsBooleanLiteral() && !child->IsNullLiteral() && !child->IsCharLiteral())) {
            if (child->IsIdentifier()) {
                needParams = true;
                functionParams.push_back(child->AsIdentifier());
            }
        }
        return false;
    });
    std::string body(src.begin() + start, src.begin() + end);
    std::string params;
    if (needParams) {
        params = GetParamsText(candidate, functionParams);
    }
    std::string functionName = "extractedFunction";
    static int anonCounter = 0;
    functionName += std::to_string(++anonCounter);
    std::ostringstream oss;
    oss << "function " << functionName << "(" << params << ") {\n";
    std::istringstream lines(body);
    std::string line;
    while (std::getline(lines, line)) {
        oss << "    return " << line << ";\n";
    }
    oss << "}\n\n";
    return oss.str();
}

std::string ReplaceWithFunctionCall(const FunctionExtraction &candidate, const std::string &functionText)
{
    std::string functionName = "extractedFunction";
    {
        auto pos = functionText.find("function ");
        if (pos != std::string::npos) {
            pos += strlen("function ");
            auto paren = functionText.find('(', pos);
            if (paren != std::string::npos) {
                functionName = functionText.substr(pos, paren - pos);
            }
        }
    }
    std::string callArgs;
    for (size_t i = 0; i < candidate.parameters.size(); ++i) {
        if (i == 0) {
            callArgs += ", ";
        }
        callArgs += candidate.parameters[i]->Ident()->Name().Mutf8();
    }
    std::string callText = functionName + "(" + callArgs + ")";
    if (candidate.node->Parent()->IsBlockStatement() || candidate.node->Parent()->IsStatement()) {
        callText += ",";
    }
    return callText;
}

ir::AstNode *FindRefactor(const RefactorContext &context)
{
    if (context.kind == EXTRACT_CONSTANT_ACTION.name || context.kind == EXTRACT_VARIABLE_ACTION.name) {
        return FindExtractedVals(context);
    }
    if (context.kind == EXTRACT_FUNCTION_ACTION.name) {
        return FindExtractedFunction(context);
    }

    return nullptr;
}

std::string GetBinaryElementsText(std::string_view &src, ir::AstNode *extractedText, const RefactorContext &context)
{
    std::string strNow;
    const auto rightNodes = IsRightNodeInRange(extractedText, context.span);
    if (!rightNodes.empty()) {
        for (ir::AstNode *node : rightNodes) {
            strNow += "" + GetSourceTextOfNodeFromSourceFile(src, node);
        }
    }
    return strNow;
}

std::string GetConstantString(std::string_view &src, ir::AstNode *extractedText, const RefactorContext &context)
{
    if (extractedText == nullptr) {
        return "";
    }
    if (extractedText->IsVariableDeclaration()) {
        auto declarators = extractedText->AsVariableDeclaration()->Declarators();
        if (declarators.empty()) {
            return "";
        }
        auto init = declarators.front()->Init();
        if (init == nullptr || !init->IsExpression()) {
            return "";
        }
        return GetSourceTextOfNodeFromSourceFile(src, init);
    }
    if (extractedText->IsExpressionStatement()) {
        auto expression = extractedText->AsExpressionStatement()->GetExpression();
        if (expression == nullptr || !expression->IsExpression()) {
            return "";
        }
        return GetSourceTextOfNodeFromSourceFile(src, expression);
    }
    if (extractedText->IsMemberExpression()) {
        const size_t sizeOfPuncht = 2;
        size_t endPos = extractedText->AsMemberExpression()->End().index;
        if (extractedText->AsMemberExpression()->Object()->IsETSNewClassInstanceExpression()) {
            endPos = extractedText->AsMemberExpression()->End().index + sizeOfPuncht;
        }
        return std::string(src).substr(extractedText->AsMemberExpression()->Start().index,
                                       endPos - extractedText->AsMemberExpression()->Start().index);
    }
    if (extractedText != nullptr) {
        std::string strNow = GetSourceTextOfNodeFromSourceFile(src, extractedText);
        if (extractedText->Parent()->IsBinaryExpression()) {
            strNow = GetBinaryElementsText(src, extractedText, context);
        }
        return strNow;
    }
    return "";
}

std::string GenerateInlineEdits(const RefactorContext &context, ir::AstNode *extractedText)
{
    if (extractedText == nullptr) {
        return "";
    }
    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    if (impl == nullptr) {
        return "";
    }
    const auto ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr) {
        return "";
    }
    while (IsNodeInScope(extractedText) || IsNodeInRange(extractedText, context.span)) {
        extractedText = extractedText->Parent();
    }
    if (extractedText == nullptr) {
        return "";
    }
    auto src = reinterpret_cast<public_lib::Context *>(context.context)->sourceFile->source;
    if (src.empty()) {
        return "";
    }
    std::string extractedStr;
    if (context.kind == EXTRACT_CONSTANT_ACTION.name) {
        extractedStr = "const EXTRACTED_VAL = ";
    } else if (context.kind == EXTRACT_VARIABLE_ACTION.name) {
        extractedStr = "let EXTRACTED_VAL = ";
    }
    auto placeholder = GetConstantString(src, extractedText, context);
    if (placeholder.empty()) {
        return "";
    }
    extractedStr += placeholder;
    if (extractedStr.find(';') == std::string::npos) {
        extractedStr += ";";
    }
    return extractedStr;
}

ApplicableRefactorInfo ExtractSymbolRefactor::GetAvailableActions(const RefactorContext &refContext) const
{
    const auto requestedRefactor = refContext.kind;
    const auto rangeToExtract = refContext.span;
    if (requestedRefactor.empty() || rangeToExtract.pos >= rangeToExtract.end) {
        return {};
    }
    const auto extractedText = FindRefactor(refContext);
    if (extractedText == nullptr) {
        return {};
    }

    ApplicableRefactorInfo res;
    res.name = REFACTOR_NAME;
    res.description = "Extract symbol refactor";
    if (requestedRefactor == EXTRACT_CONSTANT_ACTION.kind) {
        res.action =
            RefactorAction {std::string(EXTRACT_CONSTANT_ACTION.name), std::string(EXTRACT_CONSTANT_ACTION.description),
                            std::string(EXTRACT_CONSTANT_ACTION.kind)};
    } else if (requestedRefactor == EXTRACT_VARIABLE_ACTION.kind) {
        res.action =
            RefactorAction {std::string(EXTRACT_VARIABLE_ACTION.name), std::string(EXTRACT_VARIABLE_ACTION.description),
                            std::string(EXTRACT_VARIABLE_ACTION.kind)};
    } else if (requestedRefactor == EXTRACT_FUNCTION_ACTION.kind) {
        res.action =
            RefactorAction {std::string(EXTRACT_FUNCTION_ACTION.name), std::string(EXTRACT_FUNCTION_ACTION.description),
                            std::string(EXTRACT_FUNCTION_ACTION.kind)};
    }
    return res;
}

RefactorEditInfo GetRefactorEditsToExtractVals(const RefactorContext &context, ir::AstNode *extractedText)
{
    std::string generatedText = GenerateInlineEdits(context, extractedText);
    if (generatedText.empty()) {
        return RefactorEditInfo {};
    }
    std::vector<FileTextChanges> edits;
    TextChangesContext textChangesContext = *context.textChangesContext;
    const auto src = reinterpret_cast<public_lib::Context *>(context.context)->sourceFile;
    edits = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        tracker.InsertText(src, GetVarAndFunctionPosToWriteNode(context).pos, generatedText);
        tracker.ReplaceNodeWithText(context.context, extractedText, "EXTRACTED_VAL");
    });

    RefactorEditInfo refactorEdits(std::move(edits));
    return refactorEdits;
}

RefactorEditInfo GetRefactorEditsToExtractFunction(const RefactorContext &context, ir::AstNode *extractedText)
{
    std::vector<FileTextChanges> edits;

    auto *extractedNode = FindExtractedFunction(context);
    if (extractedNode == nullptr) {
        return RefactorEditInfo();
    }
    auto candidates = GetPossibleFunctionExtractions(context);
    if (candidates.empty()) {
        return RefactorEditInfo();
    }

    int index = 0;
    FunctionExtraction candidate = candidates[index];

    CollectFunctionParameters(candidate);
    std::string functionText = BuildFunctionText(candidate, context);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    auto tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto funcCallText = ReplaceWithFunctionCall(candidate, functionText);

    TextChangesContext textChangesContext = *context.textChangesContext;
    const auto src = reinterpret_cast<public_lib::Context *>(context.context)->sourceFile;
    edits = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        tracker.InsertText(src, GetVarAndFunctionPosToWriteNode(context).pos, functionText);
        tracker.ReplaceNodeWithText(context.context, extractedText, funcCallText);
    });
    RefactorEditInfo refactorEdits(std::move(edits));
    return refactorEdits;
}

std::unique_ptr<RefactorEditInfo> ExtractSymbolRefactor::GetEditsForAction(const RefactorContext &context,
                                                                           const std::string &actionName) const
{
    const auto ctx = context.context;
    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    if (ctx == nullptr || impl == nullptr) {
        return nullptr;
    }
    const auto rangeToExtract = context.span;
    if (rangeToExtract.pos >= rangeToExtract.end) {
        return nullptr;
    }
    const auto extractedText = FindRefactor(context);
    if (extractedText == nullptr) {
        return nullptr;
    }
    RefactorEditInfo refactor;
    if (actionName == EXTRACT_CONSTANT_ACTION.name) {
        refactor = GetRefactorEditsToExtractVals(context, extractedText);
    } else if (actionName == EXTRACT_VARIABLE_ACTION.name) {
        refactor = GetRefactorEditsToExtractVals(context, extractedText);
    } else if (actionName == EXTRACT_FUNCTION_ACTION.name) {
        refactor = GetRefactorEditsToExtractFunction(context, extractedText);
    }

    return std::make_unique<RefactorEditInfo>(refactor);
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<ExtractSymbolRefactor> g_extractSymbolRefactorRegister("ExtractSymbolRefactor");

}  // namespace ark::es2panda::lsp
