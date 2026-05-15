/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "refactors/extract_symbol_internal.h"
#include <cctype>

#include "checker/ETSchecker.h"
#include "lsp/include/internal_api.h"

namespace ark::es2panda::lsp {

// Keep this compilation unit as the analysis-layer extension point.
// Existing behavior remains in extract_symbol.cpp and extract_symbol_impl_edits.cpp.

static bool ShouldReturnExtractedExpressionResult(const RefactorContext &context, ir::AstNode *extractedNode);
static bool HasAwaitInRange(ir::AstNode *ast, TextRange range);

std::string InferTypeFromChecker(checker::ETSChecker *checker, ir::AstNode *node)
{
    if (checker == nullptr || node == nullptr || IsInsideFinallyBlock(node)) {
        return "";
    }
    auto type = GetTypeOfSymbolAtLocation(checker, node);
    if (type == nullptr) {
        return "";
    }
    std::string typeText = type->ToString();
    return typeText.empty() ? "" : ": " + typeText;
}

static std::string TypeTextFromAnno(public_lib::Context *ctx, ir::TypeNode *typeAnno)
{
    if (ctx == nullptr || typeAnno == nullptr) {
        return "";
    }
    std::string typeText = GetNodeText(ctx, typeAnno);
    return typeText.empty() ? typeAnno->ToString() : typeText;
}

ir::TypeNode *TypeAnnoFromDeclaratorId(ir::Expression *idExpr)
{
    if (idExpr == nullptr) {
        return nullptr;
    }
    if (idExpr->IsIdentifier()) {
        return idExpr->AsIdentifier()->TypeAnnotation();
    }
    if (idExpr->IsAnnotatedExpression()) {
        return idExpr->AsAnnotatedExpression()->TypeAnnotation();
    }
    return nullptr;
}

static std::string InferDeclaredTypeFromSourceAt(public_lib::Context *ctx, size_t pos)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return "";
    }
    if (auto declaredType = ExtractVariableDeclaredTypeFromInitializer(ctx->sourceFile->source, pos);
        declaredType.has_value() && !declaredType->empty()) {
        return ": " + declaredType.value();
    }
    if (auto declaredType = ExtractClassPropertyDeclaredTypeFromInitializer(ctx->sourceFile->source, pos);
        declaredType.has_value() && !declaredType->empty()) {
        return ": " + declaredType.value();
    }
    return "";
}

static bool ContainsProbeRange(const ir::AstNode *node, size_t probeStart, size_t probeEnd)
{
    return node != nullptr && node->Start().index <= probeStart && node->End().index >= probeEnd;
}

static std::string InferTypeFromVariableDeclaratorOwner(public_lib::Context *ctx, ir::VariableDeclarator *decl,
                                                        size_t probeStart, size_t probeEnd)
{
    auto *init = decl == nullptr ? nullptr : decl->Init();
    if (!ContainsProbeRange(init, probeStart, probeEnd) || decl->Id() == nullptr) {
        return "";
    }
    if (std::string typeText = TypeTextFromAnno(ctx, TypeAnnoFromDeclaratorId(decl->Id())); !typeText.empty()) {
        return ": " + typeText;
    }
    return InferDeclaredTypeFromSourceAt(ctx, init->Start().index);
}

static std::string InferTypeFromClassPropertyOwner(public_lib::Context *ctx, ir::ClassProperty *prop, size_t probeStart,
                                                   size_t probeEnd)
{
    auto *value = prop == nullptr ? nullptr : prop->Value();
    if (!ContainsProbeRange(value, probeStart, probeEnd)) {
        return "";
    }
    ir::TypeNode *typeAnno = prop->TypeAnnotation();
    if (typeAnno == nullptr && prop->Key() != nullptr && prop->Key()->IsIdentifier()) {
        typeAnno = prop->Key()->AsIdentifier()->TypeAnnotation();
    }
    if (std::string typeText = TypeTextFromAnno(ctx, typeAnno); !typeText.empty()) {
        return ": " + typeText;
    }
    return InferDeclaredTypeFromSourceAt(ctx, value->Start().index);
}

static std::string InferConsumerTypeFromNearestOwner(public_lib::Context *ctx, ir::AstNode *node, size_t probeStart,
                                                     size_t probeEnd)
{
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (current->IsVariableDeclarator()) {
            if (std::string inferred =
                    InferTypeFromVariableDeclaratorOwner(ctx, current->AsVariableDeclarator(), probeStart, probeEnd);
                !inferred.empty()) {
                return inferred;
            }
            continue;
        }
        if (!current->IsClassProperty()) {
            continue;
        }
        if (std::string inferred =
                InferTypeFromClassPropertyOwner(ctx, current->AsClassProperty(), probeStart, probeEnd);
            !inferred.empty()) {
            return inferred;
        }
    }
    return "";
}

static void UpdateBestConsumerTypeCandidate(ir::TypeNode *candidate, size_t span, size_t &bestSpan,
                                            ir::TypeNode *&typeAnno)
{
    if (candidate != nullptr && span < bestSpan) {
        bestSpan = span;
        typeAnno = candidate;
    }
}

static void ConsiderVariableDeclaratorTypeCandidate(ir::AstNode *current, size_t probeStart, size_t probeEnd,
                                                    size_t &bestSpan, ir::TypeNode *&typeAnno)
{
    auto *decl = current->AsVariableDeclarator();
    auto *init = decl == nullptr ? nullptr : decl->Init();
    if (!ContainsProbeRange(init, probeStart, probeEnd) || decl->Id() == nullptr) {
        return;
    }
    auto *candidate = TypeAnnoFromDeclaratorId(decl->Id());
    UpdateBestConsumerTypeCandidate(candidate, init->End().index - init->Start().index, bestSpan, typeAnno);
}

static void ConsiderClassPropertyTypeCandidate(ir::AstNode *current, size_t probeStart, size_t probeEnd,
                                               size_t &bestSpan, ir::TypeNode *&typeAnno)
{
    auto *prop = current->AsClassProperty();
    auto *value = prop == nullptr ? nullptr : prop->Value();
    if (!ContainsProbeRange(value, probeStart, probeEnd)) {
        return;
    }
    ir::TypeNode *candidate = prop->TypeAnnotation();
    if (candidate == nullptr && prop->Key() != nullptr && prop->Key()->IsIdentifier()) {
        candidate = prop->Key()->AsIdentifier()->TypeAnnotation();
    }
    UpdateBestConsumerTypeCandidate(candidate, value->End().index - value->Start().index, bestSpan, typeAnno);
}

static ir::TypeNode *FindNarrowestConsumerTypeAnnotation(public_lib::Context *ctx, size_t probeStart, size_t probeEnd)
{
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return nullptr;
    }
    ir::TypeNode *typeAnno = nullptr;
    size_t bestSpan = std::numeric_limits<size_t>::max();
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *current) {
        if (current == nullptr) {
            return false;
        }
        if (current->IsVariableDeclarator()) {
            ConsiderVariableDeclaratorTypeCandidate(current, probeStart, probeEnd, bestSpan, typeAnno);
            return false;
        }
        if (current->IsClassProperty()) {
            ConsiderClassPropertyTypeCandidate(current, probeStart, probeEnd, bestSpan, typeAnno);
        }
        return false;
    });
    return typeAnno;
}

std::string InferFromConsumerTypeAnnotation(const RefactorContext &context, public_lib::Context *ctx, ir::AstNode *node)
{
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr || node == nullptr) {
        return "";
    }
    const TextRange selectionRange = GetTrimmedSelectionSpan(context);
    const size_t probeStart = selectionRange.pos;
    const size_t probeEnd = selectionRange.end;
    if (std::string nearest = InferConsumerTypeFromNearestOwner(ctx, node, probeStart, probeEnd); !nearest.empty()) {
        return nearest;
    }
    std::string typeText = TypeTextFromAnno(ctx, FindNarrowestConsumerTypeAnnotation(ctx, probeStart, probeEnd));
    return (typeText.empty() || typeText == "[]") ? InferDeclaredTypeFromSourceAt(ctx, node->Start().index)
                                                  : ": " + typeText;
}

static std::string ReplaceArrayLiteralTypeFallback(const RefactorContext &context, public_lib::Context *ctx,
                                                   const std::string &typeText)
{
    if (typeText != ": []" || ctx == nullptr || ctx->sourceFile == nullptr) {
        return typeText;
    }
    const TextRange selectionRange = GetTrimmedSelectionSpan(context);
    if (std::string declared = InferDeclaredTypeFromSourceAt(ctx, selectionRange.pos); !declared.empty()) {
        return declared;
    }
    return typeText;
}

std::string InferExtractedReturnTypeAnnotationImpl(const RefactorContext &context, ir::AstNode *extractedNode)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    auto *checker = ctx == nullptr || ctx->GetChecker() == nullptr ? nullptr : ctx->GetChecker()->AsETSChecker();

    if (extractedNode == nullptr) {
        return "";
    }
    if (extractedNode->IsReturnStatement()) {
        auto *retStmt = extractedNode->AsReturnStatement();
        if (retStmt == nullptr || retStmt->Argument() == nullptr) {
            return ": void";
        }
        auto *argument = retStmt->Argument();
        if (std::string fromTypeAnnotation = InferFromConsumerTypeAnnotation(context, ctx, argument);
            !fromTypeAnnotation.empty()) {
            return fromTypeAnnotation;
        }
        if (std::string fromChecker = InferTypeFromChecker(checker, argument); !fromChecker.empty()) {
            return fromChecker;
        }
    } else if (extractedNode->IsExpression()) {
        if (std::string fromTypeAnnotation = InferFromConsumerTypeAnnotation(context, ctx, extractedNode);
            !fromTypeAnnotation.empty()) {
            return ReplaceArrayLiteralTypeFallback(context, ctx, fromTypeAnnotation);
        }
        if (std::string fromChecker = InferTypeFromChecker(checker, extractedNode); !fromChecker.empty()) {
            return ReplaceArrayLiteralTypeFallback(context, ctx, fromChecker);
        }
    } else {
        return "";
    }
    return "";
}

std::string NormalizeReturnTypeAnnotation(std::string annotation)
{
    return annotation;
}

static std::string InferReturnTypeFromDeclaredBinding(const RefactorContext &context, public_lib::Context *ctx,
                                                      TextRange extractionPos)
{
    VariableBindingInfo binding;
    auto *touch = GetTouchingToken(context.context, extractionPos.pos, false);
    for (auto *current = touch; current != nullptr; current = current->Parent()) {
        if (!ResolveVariableBinding(current, binding) || binding.identifier == nullptr ||
            binding.initializer == nullptr || binding.identifier->TypeAnnotation() == nullptr) {
            continue;
        }
        if (extractionPos.pos < binding.initializer->Start().index ||
            extractionPos.end > binding.initializer->End().index) {
            continue;
        }
        std::string declaredType = GetNodeText(ctx, binding.identifier->TypeAnnotation());
        if (declaredType.empty()) {
            declaredType = binding.identifier->TypeAnnotation()->ToString();
        }
        if (!declaredType.empty()) {
            return NormalizeReturnTypeAnnotation(": " + declaredType);
        }
    }
    return "";
}

static ir::AstNode *GetSelectedExpressionNode(const RefactorContext &context, TextRange extractionPos)
{
    auto *selected = GetTouchingTokenByRange(context.context, extractionPos, false);
    return selected == nullptr ? nullptr : GetOptimumNodeByRange(selected, extractionPos);
}

static std::string InferReturnTypeFromChecker(public_lib::Context *ctx, ir::AstNode *selected)
{
    auto *checker = ctx->GetChecker() == nullptr ? nullptr : ctx->GetChecker()->AsETSChecker();
    if (checker == nullptr || selected == nullptr || !selected->IsExpression()) {
        return "";
    }
    auto type = GetTypeOfSymbolAtLocation(checker, selected);
    if (type == nullptr) {
        return "";
    }
    std::string typeText = type->ToString();
    return typeText.empty() ? "" : NormalizeReturnTypeAnnotation(": " + typeText);
}

static std::string InferLiteralReturnType(ir::AstNode *selected)
{
    if (selected == nullptr) {
        return "";
    }
    if (selected->IsNumberLiteral()) {
        return ": Int";
    }
    if (selected->IsStringLiteral()) {
        return ": string";
    }
    return selected->IsBooleanLiteral() ? ": boolean" : "";
}

static std::string InferReturnTypeFromSelectionText(public_lib::Context *ctx, TextRange trimmed)
{
    if (trimmed.end <= trimmed.pos || trimmed.end > ctx->sourceFile->source.size()) {
        return "";
    }
    std::string selectedText =
        TrimAsciiWhitespace(ctx->sourceFile->source.substr(trimmed.pos, trimmed.end - trimmed.pos));
    if (selectedText.empty()) {
        return "";
    }
    if ((selectedText.front() == '"' && selectedText.back() == '"') ||
        (selectedText.front() == '\'' && selectedText.back() == '\'')) {
        return ": string";
    }
    if (selectedText.find('"') != std::string::npos || selectedText.find('\'') != std::string::npos) {
        return ": string";
    }
    bool onlyNumeric = true;
    for (size_t i = 0; i < selectedText.size(); ++i) {
        const char ch = selectedText[i];
        if (i == 0 && (ch == '+' || ch == '-')) {
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(ch)) == 0 && ch != '.' &&
            !std::isspace(static_cast<unsigned char>(ch))) {
            onlyNumeric = false;
            break;
        }
    }
    return onlyNumeric ? ": Int" : "";
}

std::string InferReturnTypeAnnotationFromSelectionFallback(const RefactorContext &context, public_lib::Context *ctx,
                                                           TextRange extractionPos)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return "";
    }
    if (std::string declared = InferReturnTypeFromDeclaredBinding(context, ctx, extractionPos); !declared.empty()) {
        return declared;
    }
    auto *selected = GetSelectedExpressionNode(context, extractionPos);
    if (std::string checkerType = InferReturnTypeFromChecker(ctx, selected); !checkerType.empty()) {
        return checkerType;
    }
    if (std::string literalType = InferLiteralReturnType(selected); !literalType.empty()) {
        return literalType;
    }
    return InferReturnTypeFromSelectionText(ctx, GetTrimmedSelectionSpan(context));
}

void EnsureTrailingSemicolon(std::string &text)
{
    if (text.empty()) {
        return;
    }
    size_t end = text.size();
    while (end > 0 && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    if (end == 0) {
        return;
    }
    if (text[end - 1] == ';' || text[end - 1] == '}') {
        return;
    }
    text.insert(end, ";");
}

static std::string GetTypeAnnotationText(public_lib::Context *ctx, ir::TypeNode *typeAnnotation)
{
    if (ctx == nullptr || typeAnnotation == nullptr) {
        return "";
    }
    std::string typeText = GetNodeText(ctx, typeAnnotation);
    return typeText.empty() ? typeAnnotation->ToString() : typeText;
}

static std::string ResolveVarTypeTextFromDeclarator(public_lib::Context *ctx, ir::VariableDeclarator *declarator)
{
    if (ctx == nullptr || declarator == nullptr || declarator->Id() == nullptr || !declarator->Id()->IsIdentifier()) {
        return "";
    }
    auto *ident = declarator->Id()->AsIdentifier();
    if (auto *typeAnnotation = ident->TypeAnnotation(); typeAnnotation != nullptr) {
        std::string typeText = GetTypeAnnotationText(ctx, typeAnnotation);
        if (!typeText.empty()) {
            return typeText;
        }
    }
    auto *checker = ctx->GetChecker() == nullptr ? nullptr : ctx->GetChecker()->AsETSChecker();
    auto type = GetTypeOfSymbolAtLocation(checker, ident);
    return type == nullptr ? "" : type->ToString();
}

static std::string FindClassPropertyTypeAtExtraction(ir::AstNode *node, TextRange extractionPos,
                                                     public_lib::Context *ctx)
{
    for (ir::AstNode *current = node; current != nullptr; current = current->Parent()) {
        if (!current->IsClassProperty()) {
            continue;
        }
        auto *classProperty = current->AsClassProperty();
        auto *value = classProperty->Value();
        if (value != nullptr && value->Start().index == extractionPos.pos && value->End().index == extractionPos.end) {
            return GetTypeAnnotationText(ctx, classProperty->TypeAnnotation());
        }
    }
    return "";
}

static std::string FindVariableDeclaratorTypeAtExtraction(ir::AstNode *node, TextRange extractionPos,
                                                          public_lib::Context *ctx)
{
    for (ir::AstNode *current = node; current != nullptr; current = current->Parent()) {
        if (!current->IsVariableDeclarator()) {
            continue;
        }
        auto *declarator = current->AsVariableDeclarator();
        auto *init = declarator->Init();
        if (init == nullptr || init->Start().index != extractionPos.pos || init->End().index != extractionPos.end) {
            continue;
        }
        return ResolveVarTypeTextFromDeclarator(ctx, declarator);
    }
    return "";
}

static std::string FindFallbackDeclaredTypeAtExtraction(std::string_view source, size_t extractionPos)
{
    if (auto typeText = ExtractVariableDeclaredTypeFromInitializer(source, extractionPos); typeText.has_value()) {
        return ": " + typeText.value();
    }
    if (auto typeText = ExtractClassPropertyDeclaredTypeFromInitializer(source, extractionPos); typeText.has_value()) {
        return ": " + typeText.value();
    }
    return "";
}

static std::string ResolveTypeAnnotationFromEnclosingBinding(ir::AstNode *node, TextRange extractionPos,
                                                             public_lib::Context *ctx)
{
    if (ctx == nullptr) {
        return "";
    }
    VariableBindingInfo binding;
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (!ResolveVariableBinding(current, binding) || binding.identifier == nullptr ||
            binding.initializer == nullptr) {
            continue;
        }
        if (binding.initializer->Start().index > extractionPos.pos ||
            binding.initializer->End().index < extractionPos.end) {
            continue;
        }
        std::string typeText = ResolveVarTypeTextFromDeclarator(ctx, binding.declarator);
        if (!typeText.empty()) {
            return ": " + typeText;
        }
    }
    return "";
}

std::string ResolveTypeAnnotationFromContainingDeclarator(const RefactorContext &context, TextRange selection,
                                                          public_lib::Context *ctx)
{
    auto *decl = FindContainingDeclaratorByRange(context, selection);
    if (decl == nullptr) {
        return "";
    }
    std::string typeText = ResolveVarTypeTextFromDeclarator(ctx, decl);
    return typeText.empty() ? "" : ": " + typeText;
}

std::string ResolveVariableTypeAnnotation(public_lib::Context *ctx, const RefactorContext &context,
                                          ir::AstNode *extractedText)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || extractedText == nullptr) {
        return "";
    }
    const auto extractionPos = GetTrimmedSelectionSpan(context);
    if (std::string typeText = FindClassPropertyTypeAtExtraction(extractedText, extractionPos, ctx);
        !typeText.empty()) {
        return ": " + typeText;
    }
    if (std::string typeText = FindVariableDeclaratorTypeAtExtraction(extractedText, extractionPos, ctx);
        !typeText.empty()) {
        return ": " + typeText;
    }
    ir::AstNode *bindingProbe = extractedText;
    if (auto *initializerExpr = ResolveInitializerExpressionContainingSelection(context, extractionPos);
        initializerExpr != nullptr) {
        bindingProbe = initializerExpr;
    }
    if (std::string typeText = ResolveTypeAnnotationFromEnclosingBinding(bindingProbe, extractionPos, ctx);
        !typeText.empty()) {
        return typeText;
    }
    if (std::string typeText = ResolveTypeAnnotationFromContainingDeclarator(context, extractionPos, ctx);
        !typeText.empty()) {
        return typeText;
    }
    std::string fallback = FindFallbackDeclaredTypeAtExtraction(ctx->sourceFile->source, extractionPos.pos);
    if (!fallback.empty()) {
        return fallback;
    }
    if (auto *initializerExpr = ResolveInitializerExpressionContainingSelection(context, extractionPos);
        initializerExpr != nullptr) {
        return FindFallbackDeclaredTypeAtExtraction(ctx->sourceFile->source, initializerExpr->Start().index);
    }
    return "";
}

ir::AstNode *UnwrapExpressionStatement(ir::AstNode *node)
{
    if (node != nullptr && node->IsExpressionStatement()) {
        auto *innerExpr = node->AsExpressionStatement()->GetExpression();
        if (innerExpr != nullptr) {
            return innerExpr;
        }
    }
    return node;
}

static bool IsBooleanBinaryExpression(const ir::AstNode *node);
static std::string ResolveSemanticCheckerTypeAnnotation(const ExtractedVariableTypeAnnotationState &state,
                                                        ir::AstNode *typeSemanticNode, std::string typeAnnotation);
static std::string ResolveSemanticStringTypeAnnotation(const ExtractedVariableTypeAnnotationState &state,
                                                       ir::AstNode *typeSemanticNode, std::string typeAnnotation);
static std::string ResolveSemanticBinaryTypeAnnotation(const ExtractedVariableTypeAnnotationState &state,
                                                       std::string typeAnnotation);
static std::string ResolveSemanticDeclaratorFallbackTypeAnnotation(const ExtractedVariableTypeAnnotationState &state,
                                                                   std::string typeAnnotation);
static std::string TryResolveGlobalConstantDeclaredTypeFromLine(const ExtractedVariableTypeAnnotationState &state);

std::string TryResolveCallSelectionTypeAnnotation(const ExtractedVariableTypeAnnotationState &state,
                                                  bool &isCallSelection)
{
    const bool canTreatAsFullCallSelection = state.ctx != nullptr && state.ctx->sourceFile != nullptr &&
                                             !HasSelectionNewline(state.context, state.ctx->sourceFile->source) &&
                                             state.coveringExpr != nullptr && state.coveringExpr->IsCallExpression() &&
                                             state.coveringExpr->Start().index == state.trimmed.pos &&
                                             state.coveringExpr->End().index == state.trimmed.end;
    isCallSelection =
        (state.selectionExpr != nullptr && state.selectionExpr->IsCallExpression()) || canTreatAsFullCallSelection;
    if (!isCallSelection) {
        return "";
    }
    if (std::string declaredType = TryResolveGlobalConstantDeclaredTypeFromLine(state); !declaredType.empty()) {
        return declaredType;
    }
    if (state.selectionExpr != nullptr) {
        std::string consumerAnno = InferFromConsumerTypeAnnotation(state.context, state.ctx, state.selectionExpr);
        if (!consumerAnno.empty() && consumerAnno != ": undefined") {
            return consumerAnno;
        }
    }
    return "";
}

static std::string TryResolveGlobalConstantDeclaredTypeFromLine(const ExtractedVariableTypeAnnotationState &state)
{
    if (state.ctx == nullptr || state.ctx->sourceFile == nullptr) {
        return "";
    }
    if (state.trimmed.end <= state.trimmed.pos || state.trimmed.end > state.ctx->sourceFile->source.size()) {
        return "";
    }

    const std::string_view source = state.ctx->sourceFile->source;
    size_t lineStart = source.rfind('\n', state.trimmed.pos);
    lineStart = (lineStart == std::string_view::npos) ? 0 : (lineStart + 1);
    const size_t eqPos = source.rfind('=', state.trimmed.pos);
    if (eqPos == std::string_view::npos || eqPos < lineStart) {
        return "";
    }

    const std::string_view head = source.substr(lineStart, eqPos - lineStart);
    const bool isDeclarationHead = head.find("let ") != std::string_view::npos ||
                                   head.find("const ") != std::string_view::npos ||
                                   head.find("var ") != std::string_view::npos;
    if (!isDeclarationHead) {
        return "";
    }

    const size_t colonPos = source.rfind(':', eqPos);
    if (colonPos == std::string_view::npos || colonPos < lineStart) {
        return "";
    }
    std::string declared = TrimAsciiWhitespace(source.substr(colonPos + 1, eqPos - colonPos - 1));
    if (declared.empty()) {
        return "";
    }
    return ": " + declared;
}

static ir::AstNode *ResolveTypeSemanticNode(const ExtractedVariableTypeAnnotationState &state)
{
    ir::AstNode *typeSemanticNode =
        UnwrapExpressionStatement(state.exactSelectionExpr != nullptr ? state.exactSelectionExpr : state.extractedText);
    if (typeSemanticNode == nullptr) {
        auto *touching = GetTouchingTokenByRange(state.context.context, state.trimmed, false);
        for (auto *current = touching; current != nullptr; current = current->Parent()) {
            if (current->IsExpressionStatement()) {
                auto *innerExpr = current->AsExpressionStatement()->GetExpression();
                typeSemanticNode = innerExpr == nullptr ? current : innerExpr;
                break;
            }
            if (current->IsExpression()) {
                typeSemanticNode = current;
                break;
            }
        }
    }
    return typeSemanticNode;
}

std::string ResolveExtractedVariableTypeAnnotationFromSemanticNode(const ExtractedVariableTypeAnnotationState &state,
                                                                   std::string extractedVarTypeAnnotation)
{
    ir::AstNode *typeSemanticNode = ResolveTypeSemanticNode(state);
    if (extractedVarTypeAnnotation == ": undefined") {
        extractedVarTypeAnnotation.clear();
    }
    extractedVarTypeAnnotation =
        ResolveSemanticCheckerTypeAnnotation(state, typeSemanticNode, std::move(extractedVarTypeAnnotation));
    extractedVarTypeAnnotation =
        ResolveSemanticStringTypeAnnotation(state, typeSemanticNode, std::move(extractedVarTypeAnnotation));
    extractedVarTypeAnnotation = ResolveSemanticBinaryTypeAnnotation(state, std::move(extractedVarTypeAnnotation));
    return ResolveSemanticDeclaratorFallbackTypeAnnotation(state, std::move(extractedVarTypeAnnotation));
}

static bool IsBooleanBinaryExpression(const ir::AstNode *node)
{
    if (node == nullptr || !node->IsBinaryExpression()) {
        return false;
    }
    auto op = node->AsBinaryExpression()->OperatorType();
    return node->AsBinaryExpression()->IsLogical() || node->AsBinaryExpression()->IsEquality() ||
           op == lexer::TokenType::PUNCTUATOR_LESS_THAN || op == lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL ||
           op == lexer::TokenType::PUNCTUATOR_GREATER_THAN || op == lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL;
}

static std::string ResolveSemanticCheckerTypeAnnotation(const ExtractedVariableTypeAnnotationState &state,
                                                        ir::AstNode *typeSemanticNode, std::string typeAnnotation)
{
    if (!typeAnnotation.empty() || (!state.isConstantEnclose && !state.isGlobalConstant) || state.ctx == nullptr ||
        state.ctx->GetChecker() == nullptr ||
        (state.isGlobalConstant && state.extractedText != nullptr && state.extractedText->IsCallExpression())) {
        return typeAnnotation;
    }
    ir::AstNode *typeProbe = typeSemanticNode;
    if (typeProbe == nullptr) {
        typeProbe = FindExactSelectionExpression(state.context, state.trimmed);
    }
    if (typeProbe == nullptr) {
        typeProbe = ResolveExpressionCoveringRange(state.context, state.trimmed);
    }
    typeProbe = UnwrapExpressionStatement(typeProbe);
    if (typeProbe != nullptr && !typeProbe->IsFunctionExpression() && !typeProbe->IsArrowFunctionExpression()) {
        typeAnnotation = InferTypeFromChecker(state.ctx->GetChecker()->AsETSChecker(), typeProbe);
    }
    return typeAnnotation;
}

static std::string ResolveSemanticStringTypeAnnotation(const ExtractedVariableTypeAnnotationState &state,
                                                       ir::AstNode *typeSemanticNode, std::string typeAnnotation)
{
    if (typeSemanticNode == nullptr) {
        return typeAnnotation;
    }
    if (typeSemanticNode->IsStringLiteral() || typeSemanticNode->IsTemplateLiteral() ||
        typeSemanticNode->IsCharLiteral()) {
        if (state.isConstantEnclose || state.isGlobalConstant) {
            return ": String";
        }
        if (state.isVariableExtraction) {
            return ": string";
        }
    }
    return typeAnnotation;
}

static std::string ResolveSemanticBinaryTypeAnnotation(const ExtractedVariableTypeAnnotationState &state,
                                                       std::string typeAnnotation)
{
    if (typeAnnotation.empty() && state.isGlobalConstant && IsBooleanBinaryExpression(state.selectionExpr)) {
        return ": Boolean";
    }
    return typeAnnotation;
}

static std::string ResolveSemanticDeclaratorFallbackTypeAnnotation(const ExtractedVariableTypeAnnotationState &state,
                                                                   std::string typeAnnotation)
{
    if ((state.isConstantEnclose || state.isGlobalConstant) && typeAnnotation.empty()) {
        typeAnnotation = ResolveTypeAnnotationFromContainingDeclarator(
            state.context, GetTrimmedSelectionSpan(state.context), state.ctx);
        if (typeAnnotation.empty()) {
            typeAnnotation = TryResolveGlobalConstantDeclaredTypeFromLine(state);
        }
    }
    if (state.isGlobalConstant && typeAnnotation.empty() && state.ctx != nullptr && state.ctx->sourceFile != nullptr) {
        typeAnnotation = FindFallbackDeclaredTypeAtExtraction(state.ctx->sourceFile->source, state.trimmed.pos);
    }
    return typeAnnotation;
}

std::pair<std::string, bool> BuildClassConstantPrefix(const std::string &varName, ir::AstNode *startedNode,
                                                      const std::string &typeAnnotation)
{
    std::string prefix;
    bool append = false;
    if (IsNamespaceContext(startedNode)) {
        prefix.append("const ").append(varName).append(" = ");
    } else {
        prefix.append("private readonly ").append(varName.substr(std::string("this.").size()));
        if (!typeAnnotation.empty()) {
            prefix.append(typeAnnotation);
        }
        prefix.append(" = ");
        append = true;
    }
    return {std::move(prefix), append};
}

bool IsObjectLiteralConstantExtraction(const std::string &actionName, const ir::AstNode *extractedText)
{
    if (!IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL) || extractedText == nullptr) {
        return false;
    }
    if (extractedText->IsObjectExpression()) {
        return true;
    }
    return IsObjectLiteralInitializerExtraction(extractedText);
}

std::string BuildMultiDeclPrefix(const std::string &varName)
{
    return varName + " = ";
}

std::string BuildStandardDeclPrefix(const std::string &varName, bool isConstantExtraction,
                                    const std::string &typeAnnotation)
{
    std::string prefix;
    prefix.append(isConstantExtraction ? "const " : "let ").append(varName);
    if (!typeAnnotation.empty()) {
        prefix.append(typeAnnotation);
    }
    prefix.append(" = ");
    return prefix;
}

static std::string GetNamespaceQualifierForDeclNode(const ir::AstNode *declNode)
{
    auto scopes = CollectEnclosingNamespaceScopes(const_cast<ir::AstNode *>(declNode));
    if (scopes.empty()) {
        return "";
    }
    std::string qualifier;
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        const std::string nsName = IdentifierNameMutf8(*it == nullptr ? nullptr : (*it)->Ident());
        if (nsName.empty()) {
            continue;
        }
        if (!qualifier.empty()) {
            qualifier.push_back('.');
        }
        qualifier.append(nsName);
    }
    return qualifier;
}

struct NamespaceReplaceOp {
    size_t start;
    size_t len;
    std::string text;
};

static void CollectNamespaceQualifierReplacement(std::vector<NamespaceReplaceOp> &replacements,
                                                 ir::AstNode *extractedText, std::string_view placeholder,
                                                 ir::AstNode *node)
{
    if (node == nullptr || !node->IsIdentifier()) {
        return;
    }
    auto *ident = node->AsIdentifier();
    if (IsMemberPropertyIdentifier(ident) || IsDeclarationIdentifier(ident)) {
        return;
    }
    auto *variable = ResolveIdentifier(ident);
    auto *decl = variable == nullptr ? nullptr : variable->Declaration();
    auto *declNode = decl == nullptr ? nullptr : decl->Node();
    if (declNode == nullptr) {
        return;
    }
    const std::string qualifier = GetNamespaceQualifierForDeclNode(declNode);
    if (qualifier.empty()) {
        return;
    }
    const std::string name = IdentifierNameMutf8(ident);
    if (name.empty()) {
        return;
    }
    if (ident->Start().index < extractedText->Start().index || ident->End().index > extractedText->End().index) {
        return;
    }
    const size_t relStart = ident->Start().index - extractedText->Start().index;
    const size_t relLen = ident->End().index - ident->Start().index;
    if (relStart + relLen > placeholder.size()) {
        return;
    }
    replacements.push_back({relStart, relLen, qualifier + "." + name});
}

static std::string ApplyNamespaceQualifierReplacements(std::string placeholder,
                                                       std::vector<NamespaceReplaceOp> &replacements)
{
    if (replacements.empty()) {
        return placeholder;
    }
    std::sort(replacements.begin(), replacements.end(),
              [](const NamespaceReplaceOp &lhs, const NamespaceReplaceOp &rhs) { return lhs.start > rhs.start; });
    size_t lastStart = std::numeric_limits<size_t>::max();
    for (const auto &op : replacements) {
        if (op.start == lastStart) {
            continue;
        }
        placeholder.replace(op.start, op.len, op.text);
        lastStart = op.start;
    }
    return placeholder;
}

static std::string QualifyNamespaceRefsForGlobalConstant(ir::AstNode *extractedText, std::string placeholder)
{
    if (placeholder.empty() || extractedText == nullptr) {
        return placeholder;
    }
    std::vector<NamespaceReplaceOp> replacements;
    CollectNamespaceQualifierReplacement(replacements, extractedText, placeholder, extractedText);
    extractedText->FindChild([&](ir::AstNode *child) {
        CollectNamespaceQualifierReplacement(replacements, extractedText, placeholder, child);
        return false;
    });
    return ApplyNamespaceQualifierReplacements(std::move(placeholder), replacements);
}

static std::string ResolveGlobalConstantPlaceholderText(public_lib::Context *ctx, const std::string &placeholder,
                                                        const std::optional<TextRange> &rhsRange)
{
    if (rhsRange.has_value()) {
        const TextRange range = rhsRange.value();
        return std::string(ctx->sourceFile->source.substr(range.pos, range.end - range.pos));
    }
    const size_t eqPos = placeholder.find('=');
    return TrimAsciiWhitespace(placeholder.substr(eqPos + 1));
}

static bool IsValidTrimmedSpanInSource(public_lib::Context *ctx, TextRange trimmedSpan)
{
    return ctx != nullptr && ctx->sourceFile != nullptr && trimmedSpan.end > trimmedSpan.pos &&
           trimmedSpan.end <= ctx->sourceFile->source.size();
}

static std::string GetTrimmedSpanText(public_lib::Context *ctx, TextRange trimmedSpan)
{
    return TrimAsciiWhitespace(ctx->sourceFile->source.substr(trimmedSpan.pos, trimmedSpan.end - trimmedSpan.pos));
}

static bool IsInsideStringLiteralFragment(const RefactorContext &context, TextRange trimmedSpan)
{
    ir::AstNode *coverExpr = ResolveExpressionCoveringRange(context, trimmedSpan);
    return coverExpr != nullptr &&
           (coverExpr->IsStringLiteral() || coverExpr->IsTemplateLiteral() || coverExpr->IsCharLiteral()) &&
           (coverExpr->Start().index != trimmedSpan.pos || coverExpr->End().index != trimmedSpan.end);
}

static std::string ResolveInitialPlaceholder(const RefactorContext &context, public_lib::Context *ctx,
                                             ir::AstNode *extractedText, const std::string &actionName,
                                             TextRange trimmedSpan)
{
    std::string placeholder = GetConstantString(std::string_view(ctx->sourceFile->source), extractedText);
    if (!IsValidTrimmedSpanInSource(ctx, trimmedSpan)) {
        return placeholder;
    }
    if (IsVariableExtractionAction(actionName)) {
        return GetTrimmedSpanText(ctx, trimmedSpan);
    }
    const bool canUseConstantSelectionText = IsConstantExtractionAction(actionName) && extractedText != nullptr &&
                                             extractedText->IsExpression() &&
                                             !IsInsideStringLiteralFragment(context, trimmedSpan);
    if (canUseConstantSelectionText) {
        return GetTrimmedSpanText(ctx, trimmedSpan);
    }
    return placeholder;
}

struct GlobalConstantPlaceholderContext {
    bool isGlobalConstant {false};
    bool isArrowSelectionText {false};
    TextRange trimmedSpan {};
    std::optional<TextRange> rhsRange {};
};

static void UpdateGlobalConstantPlaceholderFlags(PlaceholderBuildInfo &info,
                                                 const GlobalConstantPlaceholderContext &placeholderCtx,
                                                 ir::AstNode *extractedText, const std::string &placeholder)
{
    info.globalConstantInitializerSelection = placeholderCtx.isGlobalConstant && placeholderCtx.rhsRange.has_value() &&
                                              placeholderCtx.rhsRange->pos == placeholderCtx.trimmedSpan.pos &&
                                              placeholderCtx.rhsRange->end == placeholderCtx.trimmedSpan.end;
    info.globalConstantDeclShaped = placeholderCtx.isGlobalConstant && extractedText != nullptr &&
                                    !extractedText->IsExpression() && placeholder.find('=') != std::string::npos;
}

static std::string ResolveGlobalConstantPlaceholder(const PlaceholderBuildInfo &info, public_lib::Context *ctx,
                                                    std::string placeholder,
                                                    const GlobalConstantPlaceholderContext &placeholderCtx)
{
    if (!placeholderCtx.isGlobalConstant) {
        return placeholder;
    }
    if (info.globalConstantDeclShaped || info.globalConstantInitializerSelection) {
        return ResolveGlobalConstantPlaceholderText(ctx, placeholder, placeholderCtx.rhsRange);
    }
    if (IsValidTrimmedSpanInSource(ctx, placeholderCtx.trimmedSpan)) {
        placeholder = GetTrimmedSpanText(ctx, placeholderCtx.trimmedSpan);
    }
    if (placeholderCtx.isArrowSelectionText && IsValidTrimmedSpanInSource(ctx, placeholderCtx.trimmedSpan)) {
        placeholder = GetTrimmedSpanText(ctx, placeholderCtx.trimmedSpan);
    }
    return placeholder;
}

PlaceholderBuildInfo BuildExtractionPlaceholder(const RefactorContext &context, public_lib::Context *ctx,
                                                ir::AstNode *extractedText, const std::string &actionName)
{
    PlaceholderBuildInfo info;
    const TextRange trimmedSpan = GetTrimmedSelectionSpan(context);
    std::string placeholder = ResolveInitialPlaceholder(context, ctx, extractedText, actionName, trimmedSpan);
    const GlobalConstantPlaceholderContext placeholderCtx {
        IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL), IsArrowFunctionSelection(context), trimmedSpan,
        ResolveInitializerRhsRange(context, trimmedSpan)};
    UpdateGlobalConstantPlaceholderFlags(info, placeholderCtx, extractedText, placeholder);
    placeholder = ResolveGlobalConstantPlaceholder(info, ctx, std::move(placeholder), placeholderCtx);
    info.placeholder = placeholderCtx.isGlobalConstant
                           ? QualifyNamespaceRefsForGlobalConstant(extractedText, std::move(placeholder))
                           : placeholder;
    return info;
}

static std::string ResolveExtractedVariableTypeAnnotationFromSelection(
    const ExtractedVariableTypeAnnotationState &state)
{
    std::string extractedVarTypeAnnotation =
        (state.isVariableExtraction || state.isConstantEnclose ||
         (state.isGlobalConstant && state.isArrowSelectionText) ||
         IsConstantExtractionInClassAction(state.actionName) ||
         IsObjectLiteralConstantExtraction(state.actionName, state.extractedText))
            ? ResolveVariableTypeAnnotation(state.ctx, state.context, state.extractedText)
            : "";
    if ((state.placeholderInfo.globalConstantDeclShaped || state.placeholderInfo.globalConstantInitializerSelection) &&
        extractedVarTypeAnnotation.empty()) {
        std::string declaredType = ResolveTypeAnnotationFromContainingDeclarator(
            state.context, GetTrimmedSelectionSpan(state.context), state.ctx);
        if (declaredType.empty()) {
            declaredType = ResolveVariableTypeAnnotation(state.ctx, state.context, state.extractedText);
        }
        if (declaredType.rfind(": Array<", 0) == 0 || declaredType == ": double") {
            extractedVarTypeAnnotation = declaredType;
        }
    }
    if ((state.isVariableExtraction || (state.isGlobalConstant && state.isArrowSelectionText)) &&
        extractedVarTypeAnnotation.empty() && state.ctx != nullptr && state.ctx->GetChecker() != nullptr) {
        if (!(state.selectionExpr != nullptr && state.selectionExpr->IsTypeofExpression())) {
            auto *checker = state.ctx->GetChecker()->AsETSChecker();
            extractedVarTypeAnnotation = InferTypeFromChecker(checker, state.extractedText);
        }
    }
    if (state.isConstantEnclose && extractedVarTypeAnnotation.empty()) {
        ir::AstNode *consumerProbe =
            UnwrapExpressionStatement(state.selectionExpr != nullptr ? state.selectionExpr : state.extractedText);
        if (consumerProbe != nullptr) {
            extractedVarTypeAnnotation = InferFromConsumerTypeAnnotation(state.context, state.ctx, consumerProbe);
        }
    }
    return extractedVarTypeAnnotation;
}

static ExtractedVariableTypeAnnotationState BuildExtractedVariableTypeAnnotationState(
    const RefactorContext &context, public_lib::Context *ctx, ir::AstNode *extractedText, const std::string &actionName,
    const PlaceholderBuildInfo &placeholderInfo)
{
    const bool isVariableExtraction = IsVariableExtractionAction(actionName);
    const TextRange trimmed = GetTrimmedSelectionSpan(context);
    ir::AstNode *coveringExpr = ResolveExpressionCoveringRange(context, trimmed);
    ir::AstNode *exactSelectionExpr = FindExactSelectionExpression(context, trimmed);
    ir::AstNode *selectionExpr =
        UnwrapExpressionStatement(exactSelectionExpr != nullptr ? exactSelectionExpr : extractedText);
    ExtractedVariableTypeAnnotationState state {context,
                                                ctx,
                                                extractedText,
                                                actionName,
                                                placeholderInfo,
                                                trimmed,
                                                coveringExpr,
                                                exactSelectionExpr,
                                                selectionExpr,
                                                isVariableExtraction,
                                                false,
                                                false,
                                                false};
    state.isConstantEnclose = IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE);
    if (!state.isConstantEnclose) {
        state.isConstantEnclose = IsNamespaceAction(actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE.name,
                                                    EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX);
    }
    state.isGlobalConstant = IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL);
    state.isArrowSelectionText = IsArrowFunctionSelection(context);
    return state;
}

static bool ShouldSkipExtractedVariableTypeAnnotation(const ExtractedVariableTypeAnnotationState &state,
                                                      bool isCallSelection)
{
    if (state.isVariableExtraction && state.extractedText != nullptr &&
        state.extractedText->IsConditionalExpression()) {
        return true;
    }
    const bool isInstanceofSelection =
        state.selectionExpr != nullptr && state.selectionExpr->IsBinaryExpression() &&
        state.selectionExpr->AsBinaryExpression()->OperatorType() == lexer::TokenType::KEYW_INSTANCEOF;
    const bool isTypeofSelection = state.selectionExpr != nullptr && state.selectionExpr->IsTypeofExpression();
    if (isTypeofSelection && state.isVariableExtraction) {
        return true;
    }
    if ((isTypeofSelection || isInstanceofSelection) && (state.isConstantEnclose || state.isGlobalConstant)) {
        return true;
    }
    if (isCallSelection && (state.isConstantEnclose || state.isGlobalConstant)) {
        return true;
    }
    if (state.selectionExpr != nullptr &&
        (state.selectionExpr->IsFunctionExpression() || state.selectionExpr->IsArrowFunctionExpression()) &&
        (state.isVariableExtraction || state.isConstantEnclose || state.isGlobalConstant ||
         IsConstantExtractionInClassAction(state.actionName))) {
        return true;
    }
    return false;
}

static std::string ResolveCallTypeAnnotationForExtractedVariable(const ExtractedVariableTypeAnnotationState &state,
                                                                 bool &isCallSelection)
{
    return TryResolveCallSelectionTypeAnnotation(state, isCallSelection);
}

std::string ResolveExtractedVariableTypeAnnotation(const RefactorContext &context, public_lib::Context *ctx,
                                                   ir::AstNode *extractedText, const std::string &actionName,
                                                   const PlaceholderBuildInfo &placeholderInfo)
{
    ExtractedVariableTypeAnnotationState state =
        BuildExtractedVariableTypeAnnotationState(context, ctx, extractedText, actionName, placeholderInfo);
    bool isCallSelection = false;
    if (std::string callType = ResolveCallTypeAnnotationForExtractedVariable(state, isCallSelection);
        !callType.empty()) {
        return callType;
    }
    if (ShouldSkipExtractedVariableTypeAnnotation(state, isCallSelection)) {
        return "";
    }
    std::string extractedVarTypeAnnotation = ResolveExtractedVariableTypeAnnotationFromSelection(state);
    return ResolveExtractedVariableTypeAnnotationFromSemanticNode(state, std::move(extractedVarTypeAnnotation));
}

std::string BuildExtractionDeclaration(const RefactorContext &context, ir::AstNode *extractedText,
                                       const std::string &actionName, const std::string &varName)
{
    const bool isVariableExtraction = IsVariableExtractionAction(actionName);
    const bool isConstantExtraction = IsConstantExtractionAction(actionName);
    if (!isConstantExtraction && !isVariableExtraction) {
        return "";
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return "";
    }

    const PlaceholderBuildInfo placeholderInfo = BuildExtractionPlaceholder(context, ctx, extractedText, actionName);
    if (placeholderInfo.placeholder.empty()) {
        return "";
    }
    std::string extractedVarTypeAnnotation =
        ResolveExtractedVariableTypeAnnotation(context, ctx, extractedText, actionName, placeholderInfo);

    auto startedNode = GetTouchingTokenByRange(context.context, context.span, false);
    const bool isMultiDecl = IsMultiDecl(startedNode, ctx);
    const bool useInlineMultiDeclPrefix =
        isMultiDecl && !IsActionNameOrKind(actionName, EXTRACT_VARIABLE_ACTION_GLOBAL);
    bool isAppend = false;
    std::string declaration;
    if (IsConstantExtractionInClassAction(actionName)) {
        auto prefixResult = BuildClassConstantPrefix(varName, startedNode, extractedVarTypeAnnotation);
        declaration = std::move(prefixResult.first);
        isAppend = prefixResult.second;
    } else if (useInlineMultiDeclPrefix) {
        declaration = BuildMultiDeclPrefix(varName);
    } else {
        declaration = BuildStandardDeclPrefix(varName, isConstantExtraction, extractedVarTypeAnnotation);
    }
    declaration.append(placeholderInfo.placeholder);
    if (useInlineMultiDeclPrefix && declaration.find(',') == std::string::npos) {
        declaration.append(", ");
    } else if (declaration.find(';') == std::string::npos) {
        declaration.append(";");
    }
    if (isAppend) {
        declaration.append(context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter());
    }
    return declaration;
}

void ApplyVariableFormatting(const RefactorContext &context, public_lib::Context *ctx, const std::string &actionName,
                             std::string &declaration)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return;
    }
    const std::string newLine = context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();
    size_t insertPos = GetVarAndFunctionPosToWriteNode(context, actionName).pos;
    std::string insertionIndent = GetIndentAtPosition(ctx, insertPos);
    TextRange callRange = GetCallPositionOfExtraction(context);
    std::string statementIndent = GetIndentAtPosition(ctx, callRange.pos);
    const std::string &indentToUse = statementIndent.empty() ? insertionIndent : statementIndent;

    declaration = newLine + indentToUse + declaration;
    declaration.append(newLine).append(newLine);
}

static void NormalizeInlineExtractedNode(ir::AstNode *&extractedText, TextRange normalizedSpan,
                                         const std::string &actionName)
{
    if (extractedText == nullptr) {
        return;
    }
    if (auto *optimum = GetOptimumNodeByRange(extractedText, normalizedSpan); optimum != nullptr) {
        const bool isGlobalConstant = IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL);
        const bool currentCoversSelection =
            extractedText->Start().index <= normalizedSpan.pos && extractedText->End().index >= normalizedSpan.end;
        const bool optimumCoversSelection =
            optimum->Start().index <= normalizedSpan.pos && optimum->End().index >= normalizedSpan.end;
        const bool keepCurrentForGlobalConstant =
            isGlobalConstant && extractedText->IsExpression() &&
            (!optimum->IsExpression() || (currentCoversSelection && optimumCoversSelection));
        if (!keepCurrentForGlobalConstant && (!isGlobalConstant || !currentCoversSelection || optimumCoversSelection)) {
            extractedText = optimum;
        }
    }
    if (IsActionNameOrKind(actionName, EXTRACT_CONSTANT_ACTION_GLOBAL)) {
        while (extractedText != nullptr &&
               (extractedText->Start().index > normalizedSpan.pos || extractedText->End().index < normalizedSpan.end)) {
            extractedText = extractedText->Parent();
        }
    }
}

static bool IsLiteralNodeForInlineExtraction(ir::AstNode *node)
{
    return node != nullptr && (node->IsStringLiteral() || node->IsNumberLiteral() || node->IsBooleanLiteral() ||
                               node->IsNullLiteral() || node->IsCharLiteral());
}

static void ApplyInlineExtractedFallbackNode(const RefactorContext &context, ir::AstNode *&extractedText,
                                             TextRange normalizedSpan, const std::string &actionName)
{
    const bool allowLiteralForConstant = IsConstantExtractionAction(actionName);
    if (extractedText == nullptr || !IsNodeInScope(extractedText)) {
        return;
    }
    if (auto *fallback = GetTouchingTokenByRange(context.context, normalizedSpan, false); fallback != nullptr) {
        if (auto *optimum = GetOptimumNodeByRange(fallback, normalizedSpan); optimum != nullptr) {
            fallback = optimum;
        }
        const bool fallbackCoversSelection =
            fallback->Start().index <= normalizedSpan.pos && fallback->End().index >= normalizedSpan.end;
        const bool canUseConstantExpressionFallback =
            IsConstantExtractionAction(actionName) && fallback->IsExpression() && fallbackCoversSelection;
        if (!IsNodeInScope(fallback) || (allowLiteralForConstant && IsLiteralNodeForInlineExtraction(fallback)) ||
            canUseConstantExpressionFallback) {
            extractedText = fallback;
        }
    }
}

std::string GenerateInlineEdits(const RefactorContext &context, ir::AstNode *&extractedText,
                                const std::string &actionName, const std::string &varName)
{
    if (extractedText == nullptr) {
        return "";
    }
    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    if (impl == nullptr) {
        return "";
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    const auto normalizedSpan = GetTrimmedSelectionSpan(context);
    const bool allowLiteralForConstant = IsConstantExtractionAction(actionName);
    const bool allowExpressionForConstant =
        IsConstantExtractionAction(actionName) && extractedText != nullptr && extractedText->IsExpression();
    NormalizeInlineExtractedNode(extractedText, normalizedSpan, actionName);
    ApplyInlineExtractedFallbackNode(context, extractedText, normalizedSpan, actionName);
    const bool isConstantExtraction = IsConstantExtractionAction(actionName);
    auto isInvalidExtractedNode = [allowExpressionForConstant, allowLiteralForConstant, ctx, extractedText,
                                   isConstantExtraction]() -> bool {
        return extractedText == nullptr ||
               (isConstantExtraction && IsNodeInScope(extractedText) &&
                !(allowLiteralForConstant && IsLiteralNodeForInlineExtraction(extractedText)) &&
                !allowExpressionForConstant) ||
               ctx->sourceFile == nullptr || ctx->sourceFile->source.empty();
    };
    if (isInvalidExtractedNode()) {
        return "";
    }

    return BuildExtractionDeclaration(context, extractedText, actionName, varName);
}

static std::vector<const TextChange *> CollectOrderedTextChanges(const std::vector<FileTextChanges> &edits,
                                                                 size_t insertPos, const TextChange *&insertChange,
                                                                 size_t &insertShift)
{
    std::vector<const TextChange *> ordered;
    insertChange = nullptr;
    insertShift = 0;
    if (edits.empty() || edits[0].textChanges.empty()) {
        return ordered;
    }
    ordered.reserve(edits[0].textChanges.size());
    for (const auto &change : edits[0].textChanges) {
        ordered.push_back(&change);
        if (insertChange == nullptr && change.span.length == 0 && change.span.start == insertPos) {
            insertChange = &change;
        }
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const TextChange *lhs, const TextChange *rhs) { return lhs->span.start < rhs->span.start; });
    for (const auto *change : ordered) {
        if (change->span.start + change->span.length <= insertPos) {
            if (change->span.start == insertPos && change->span.length == 0) {
                continue;
            }
            insertShift += change->newText.length() - change->span.length;
        }
    }
    return ordered;
}

static void AdjustRenameLocFromChanges(const std::vector<const TextChange *> &ordered, bool renameLocIsFinal,
                                       size_t &renameLoc)
{
    if (renameLocIsFinal || ordered.empty()) {
        return;
    }
    size_t shift = 0;
    bool resolved = false;
    for (const auto *change : ordered) {
        if (change->span.start > renameLoc) {
            break;
        }
        if (change->span.start + change->span.length <= renameLoc) {
            shift += change->newText.length() - change->span.length;
            continue;
        }
        renameLoc = change->span.start + shift;
        resolved = true;
        break;
    }
    if (!resolved) {
        renameLoc += shift;
    }
}

static std::string BuildFinalText(std::string_view source, const std::vector<const TextChange *> &ordered)
{
    std::string finalText;
    finalText.reserve(source.size());
    size_t cursor = 0;
    for (const auto *change : ordered) {
        size_t start = std::min(change->span.start, source.size());
        if (start < cursor) {
            start = cursor;
        }
        size_t end = std::min(start + change->span.length, static_cast<size_t>(source.size()));
        if (cursor < start) {
            finalText.append(source.substr(cursor, start - cursor));
        }
        finalText.append(change->newText);
        cursor = end;
    }
    if (cursor < source.size()) {
        finalText.append(source.substr(cursor, source.size() - cursor));
    }
    return finalText;
}

static std::optional<size_t> FindBestRenameLoc(const std::string &finalText, const std::string &uniqueVarName,
                                               size_t renameLoc, const std::optional<TextRange> &insertedRange)
{
    size_t bestPos = std::string::npos;
    size_t bestDist = std::numeric_limits<size_t>::max();
    size_t bestPosAny = std::string::npos;
    size_t bestDistAny = std::numeric_limits<size_t>::max();
    size_t pos = finalText.find(uniqueVarName);
    while (pos != std::string::npos) {
        size_t end = pos + uniqueVarName.size();
        size_t dist = 0;
        if (renameLoc < pos) {
            dist = pos - renameLoc;
        } else if (renameLoc > end) {
            dist = renameLoc - end;
        }
        if (dist < bestDistAny) {
            bestDistAny = dist;
            bestPosAny = pos;
        }
        const bool insideInserted = insertedRange.has_value() && pos >= insertedRange->pos && pos < insertedRange->end;
        if (!insideInserted && dist < bestDist) {
            bestDist = dist;
            bestPos = pos;
        }
        pos = finalText.find(uniqueVarName, pos + 1);
    }
    if (bestPos == std::string::npos) {
        bestPos = bestPosAny;
    }
    if (bestPos == std::string::npos) {
        return std::nullopt;
    }
    size_t renameOffset = (uniqueVarName.size() > 1) ? 1 : 0;
    if (uniqueVarName.rfind("this.", 0) == 0) {
        renameOffset = std::string("this.").size() + 1;
    }
    return bestPos + renameOffset;
}

static void ApplyImplicitPrefix(std::string &finalText, size_t adjustedInsertPos, const std::string &implicitPrefix)
{
    if (implicitPrefix.empty() || adjustedInsertPos > finalText.size()) {
        return;
    }
    if (implicitPrefix[0] == '\n' && adjustedInsertPos < finalText.size() &&
        IsLineBreakChar(finalText[adjustedInsertPos])) {
        finalText.erase(adjustedInsertPos, 1);
    }
    finalText.insert(adjustedInsertPos, implicitPrefix);
}

static std::optional<TextRange> ComputeInsertedRange(size_t adjustedInsertPos, const TextChange *insertChange,
                                                     const std::string &implicitPrefix)
{
    if (insertChange == nullptr) {
        return std::nullopt;
    }
    size_t insertedStart = adjustedInsertPos + (!implicitPrefix.empty() ? implicitPrefix.size() : 0);
    size_t insertedEnd = insertedStart + insertChange->newText.size();
    return TextRange {insertedStart, insertedEnd};
}

static std::optional<size_t> RecomputeRenameLoc(const std::string &finalText, const std::string &uniqueVarName,
                                                size_t renameLoc, const std::optional<TextRange> &insertedRange)
{
    if (finalText.empty()) {
        return std::nullopt;
    }
    return FindBestRenameLoc(finalText, uniqueVarName, renameLoc, insertedRange);
}

static std::optional<size_t> ComputeVariableUsageRenameLocFromEdits(const std::vector<FileTextChanges> &edits,
                                                                    const std::string &uniqueVarName)
{
    if (edits.empty() || edits[0].textChanges.empty() || uniqueVarName.empty()) {
        return std::nullopt;
    }
    const TextChange *replaceChange = nullptr;
    for (const auto &change : edits[0].textChanges) {
        if (change.span.length > 0 && change.newText.find(uniqueVarName) != std::string::npos) {
            replaceChange = &change;
            break;
        }
    }
    if (replaceChange == nullptr) {
        return std::nullopt;
    }

    std::vector<const TextChange *> ordered;
    ordered.reserve(edits[0].textChanges.size());
    for (const auto &change : edits[0].textChanges) {
        ordered.push_back(&change);
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const TextChange *lhs, const TextChange *rhs) { return lhs->span.start < rhs->span.start; });

    size_t shiftBeforeReplace = 0;
    for (const auto *change : ordered) {
        if (change == replaceChange) {
            break;
        }
        if (change->span.start + change->span.length <= replaceChange->span.start) {
            shiftBeforeReplace += change->newText.length() - change->span.length;
        }
    }

    size_t nameOffset = replaceChange->newText.find(uniqueVarName);
    if (nameOffset == std::string::npos && uniqueVarName.rfind("this.", 0) == 0) {
        nameOffset = replaceChange->newText.find(uniqueVarName.substr(std::string("this.").size()));
    }
    if (nameOffset == std::string::npos) {
        return std::nullopt;
    }

    size_t renameOffset = (uniqueVarName.size() > 1) ? 1 : 0;
    if (uniqueVarName.rfind("this.", 0) == 0) {
        renameOffset = std::string("this.").size() + 1;
    }
    return replaceChange->span.start + shiftBeforeReplace + nameOffset + renameOffset;
}

static std::optional<size_t> ComputeRenameLocForExprStmt(ir::AstNode *exprStmt, const std::string &generatedText,
                                                         const std::string &uniqueVarName, size_t insertPos)
{
    if (exprStmt == nullptr) {
        return std::nullopt;
    }
    std::string renameToken = uniqueVarName;
    size_t nameOffset = generatedText.find(renameToken);
    if (nameOffset == std::string::npos && renameToken.rfind("this.", 0) == 0) {
        renameToken = renameToken.substr(std::string("this.").size());
        nameOffset = generatedText.find(renameToken);
    }
    if (nameOffset == std::string::npos) {
        return std::nullopt;
    }
    size_t renameOffset = (renameToken.size() > 1) ? 1 : 0;
    return insertPos + nameOffset + renameOffset;
}

static bool IsAsciiIdentifierChar(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '$';
}

static std::optional<size_t> FindCallCalleeOffset(std::string_view callText)
{
    const size_t parenPos = callText.find('(');
    if (parenPos == std::string::npos || parenPos == 0) {
        return std::nullopt;
    }
    size_t tokenEnd = parenPos;
    while (tokenEnd > 0 && isspace(callText[tokenEnd - 1])) {
        --tokenEnd;
    }
    if (tokenEnd == 0) {
        return std::nullopt;
    }
    size_t tokenStart = tokenEnd;
    while (tokenStart > 0 && IsAsciiIdentifierChar(callText[tokenStart - 1])) {
        --tokenStart;
    }
    if (tokenStart == tokenEnd) {
        return std::nullopt;
    }
    return tokenStart;
}

static size_t CountIdentifierLength(std::string_view text, size_t start)
{
    size_t end = start;
    while (end < text.size() && IsAsciiIdentifierChar(text[end])) {
        ++end;
    }
    return end - start;
}

size_t ComputeRenameLocFromEdits(const std::vector<FileTextChanges> &edits, size_t renameLoc)
{
    if (edits.empty() || edits[0].textChanges.empty()) {
        return renameLoc;
    }
    std::vector<const TextChange *> ordered;
    ordered.reserve(edits[0].textChanges.size());
    for (const auto &change : edits[0].textChanges) {
        ordered.push_back(&change);
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const TextChange *lhs, const TextChange *rhs) { return lhs->span.start < rhs->span.start; });
    bool renameLocIsFinal = false;
    AdjustRenameLocFromChanges(ordered, renameLocIsFinal, renameLoc);
    return renameLoc;
}

std::optional<size_t> ComputeFunctionCallRenameLocFromEdits(const std::vector<FileTextChanges> &edits,
                                                            TextRange extractionPos)
{
    if (edits.empty() || edits[0].textChanges.empty()) {
        return std::nullopt;
    }
    const size_t extractionLen = extractionPos.end - extractionPos.pos;
    const TextChange *replaceChange = nullptr;
    for (const auto &change : edits[0].textChanges) {
        if (change.span.start == extractionPos.pos && change.span.length == extractionLen) {
            replaceChange = &change;
            break;
        }
    }
    if (replaceChange == nullptr) {
        for (const auto &change : edits[0].textChanges) {
            if (change.span.length == 0) {
                continue;
            }
            if (FindCallCalleeOffset(change.newText).has_value()) {
                replaceChange = &change;
                break;
            }
        }
    }
    if (replaceChange == nullptr) {
        return std::nullopt;
    }
    const auto calleeOffset = FindCallCalleeOffset(replaceChange->newText);
    if (!calleeOffset.has_value()) {
        return std::nullopt;
    }
    const size_t calleeLen = CountIdentifierLength(replaceChange->newText, calleeOffset.value());
    const size_t renameOffset = calleeLen > 1 ? 1 : 0;

    std::vector<const TextChange *> ordered;
    ordered.reserve(edits[0].textChanges.size());
    for (const auto &change : edits[0].textChanges) {
        ordered.push_back(&change);
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const TextChange *lhs, const TextChange *rhs) { return lhs->span.start < rhs->span.start; });

    size_t shiftBeforeReplace = 0;
    for (const auto *change : ordered) {
        if (change == replaceChange) {
            break;
        }
        if (change->span.start + change->span.length <= replaceChange->span.start) {
            shiftBeforeReplace += change->newText.length() - change->span.length;
        }
    }

    return replaceChange->span.start + shiftBeforeReplace + calleeOffset.value() + renameOffset;
}

bool HasSourceNewlineInRange(public_lib::Context *ctx, TextRange range)
{
    return ctx != nullptr && ctx->sourceFile != nullptr && HasNewlineInRange(ctx->sourceFile->source, range);
}

void MaybeIncludeTrailingSemicolonForReturnSelection(public_lib::Context *ctx, ir::AstNode *extractedNode,
                                                     TextRange &range)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || extractedNode == nullptr ||
        !extractedNode->IsReturnStatement()) {
        return;
    }
    auto &source = ctx->sourceFile->source;
    if (range.end >= source.size()) {
        return;
    }
    size_t probe = range.end;
    while (probe < source.size() && IsIndentChar(source[probe])) {
        ++probe;
    }
    if (probe < source.size() && source[probe] == ';') {
        range.end = probe + 1;
    }
}

size_t NormalizeFunctionInsertPos(const RefactorContext &context, public_lib::Context *ctx,
                                  const std::string &actionName)
{
    size_t insertPos = GetVarAndFunctionPosToWriteNode(context, actionName).pos;
    if (ctx != nullptr && ctx->sourceFile != nullptr) {
        const auto &source = ctx->sourceFile->source;
        insertPos = NormalizeInsertPos(source, insertPos);
        size_t probe = insertPos;
        while (probe < source.size() && IsIndentChar(source[probe])) {
            ++probe;
        }
        if (probe < source.size() && IsLineBreakChar(source[probe])) {
            size_t afterBlankLine = NormalizeInsertPos(source, probe);
            size_t closingProbe = afterBlankLine;
            while (closingProbe < source.size() && IsIndentChar(source[closingProbe])) {
                ++closingProbe;
            }
            if (closingProbe < source.size() && source[closingProbe] == '}') {
                insertPos = afterBlankLine;
            }
        }
    }
    return insertPos;
}

std::vector<std::string> BuildFunctionCallArgs(const FunctionExtraction &candidate, bool treatAsStatements,
                                               const FunctionIOInfo &ioInfo,
                                               const std::vector<std::string> *capturedArgs)
{
    if (treatAsStatements) {
        return ioInfo.callArgs;
    }
    if (capturedArgs != nullptr) {
        return *capturedArgs;
    }
    std::vector<std::string> callArgs;
    callArgs.reserve(candidate.parameters.size());
    for (auto *param : candidate.parameters) {
        if (param != nullptr && param->Ident() != nullptr) {
            callArgs.emplace_back(IdentifierNameMutf8(param->Ident()));
        }
    }
    return callArgs;
}

static int CountBraceDelta(std::string_view text)
{
    int delta = 0;
    for (char ch : text) {
        if (ch == '{') {
            ++delta;
        } else if (ch == '}') {
            --delta;
        }
    }
    return delta;
}

static std::string_view TrimLineView(std::string_view line)
{
    size_t begin = 0;
    while (begin < line.size() && std::isspace(static_cast<unsigned char>(line[begin])) != 0) {
        ++begin;
    }
    size_t end = line.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(line[end - 1])) != 0) {
        --end;
    }
    return line.substr(begin, end - begin);
}

static size_t FindLastTopLevelDeclLineEnd(std::string_view source, size_t limit)
{
    size_t cursor = 0;
    size_t lastDeclEnd = 0;
    int braceDepth = 0;

    while (cursor < limit) {
        size_t lineEnd = source.find('\n', cursor);
        if (lineEnd == std::string_view::npos || lineEnd > limit) {
            lineEnd = limit;
        }
        const bool isCompleteLine = lineEnd < source.size() && (source[lineEnd] == '\n' || source[lineEnd] == '\r');
        const std::string_view line = source.substr(cursor, lineEnd - cursor);
        const std::string_view trimmedLine = TrimLineView(line);
        const bool isTerminatedDeclLine = !trimmedLine.empty() && trimmedLine.back() == ';';
        const bool isVarDeclLine = trimmedLine.rfind("let ", 0) == 0 || trimmedLine.rfind("const ", 0) == 0;
        if (braceDepth == 0 && isCompleteLine && isTerminatedDeclLine && isVarDeclLine) {
            lastDeclEnd = lineEnd;
        }
        braceDepth += CountBraceDelta(line);
        if (lineEnd >= limit) {
            break;
        }
        cursor = lineEnd + 1;
    }
    return lastDeclEnd;
}

size_t ResolveGlobalConstantInsertionPosFromSource(std::string_view source, size_t limit, size_t fallbackPos,
                                                   size_t globalFallbackPos)
{
    if (limit > source.size()) {
        limit = source.size();
    }
    const size_t lastDeclEnd = FindLastTopLevelDeclLineEnd(source, limit);
    if (lastDeclEnd > 0) {
        return lastDeclEnd;
    }
    if (fallbackPos <= source.size()) {
        return fallbackPos;
    }
    return globalFallbackPos <= source.size() ? globalFallbackPos : fallbackPos;
}

std::string RemoveMarkerComments(std::string_view text)
{
    std::string out(text);
    auto eraseAll = [&out](std::string_view marker) {
        size_t pos = out.find(marker);
        while (pos != std::string::npos) {
            out.erase(pos, marker.size());
            pos = out.find(marker, pos);
        }
    };
    eraseAll("/*start*/");
    eraseAll("/*end*/");
    return out;
}

std::string TrimSemicolonSeparatedText(std::string_view text)
{
    std::string trimmed = TrimAsciiWhitespace(text);
    std::string_view view(trimmed);
    while (!view.empty() && view.back() == ';') {
        view.remove_suffix(1);
        trimmed = TrimAsciiWhitespace(view);
        view = trimmed;
    }
    return trimmed;
}

bool ShouldDeleteWholeExprStmtBySpan(const SourceFile *src, TextRange selectionSpan, ir::AstNode *stmt)
{
    if (stmt == nullptr || !stmt->IsExpressionStatement() || src == nullptr) {
        return false;
    }
    const auto &source = src->source;
    if (stmt->Start().index > source.size() || stmt->End().index > source.size() || selectionSpan.pos > source.size() ||
        selectionSpan.end > source.size()) {
        return false;
    }
    if (!(stmt->Start().index <= selectionSpan.pos && stmt->End().index >= selectionSpan.end)) {
        return false;
    }
    std::string stmtNorm =
        RemoveMarkerComments(source.substr(stmt->Start().index, stmt->End().index - stmt->Start().index));
    std::string selectedNorm =
        RemoveMarkerComments(source.substr(selectionSpan.pos, selectionSpan.end - selectionSpan.pos));
    return TrimSemicolonSeparatedText(stmtNorm) == TrimSemicolonSeparatedText(selectedNorm);
}

std::string BuildDeclarationCoreFromInsertedText(std::string_view insertedText)
{
    auto trimText = [](std::string_view text) -> std::string_view {
        size_t begin = 0;
        while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
            ++begin;
        }
        size_t end = text.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
            --end;
        }
        return text.substr(begin, end - begin);
    };
    auto firstNonEmptyLine = [trimText](std::string_view text) -> std::string {
        std::string_view trimmed = trimText(text);
        if (trimmed.empty()) {
            return "";
        }
        size_t pos = 0;
        while (pos < trimmed.size()) {
            size_t next = trimmed.find('\n', pos);
            if (next == std::string_view::npos) {
                next = trimmed.size();
            }
            std::string_view line = trimText(trimmed.substr(pos, next - pos));
            if (!line.empty()) {
                return std::string(line);
            }
            pos = next + 1;
        }
        return "";
    };
    auto stripOneTrailingSemicolon = [](std::string text) -> std::string {
        size_t end = text.size();
        while (end > 0 && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
            --end;
        }
        if (end > 0 && text[end - 1] == ';') {
            text.erase(end - 1, text.size() - (end - 1));
        }
        return text;
    };
    return stripOneTrailingSemicolon(firstNonEmptyLine(insertedText));
}

bool TryBuildCommentAdjacentReplacement(std::string_view source, TextRange extractedRange,
                                        const std::string &declarationCore, TextRange &replaceRange,
                                        std::string &replacementText)
{
    if (declarationCore.empty() || extractedRange.end > source.size()) {
        return false;
    }
    size_t left = extractedRange.pos;
    while (left > 0 && std::isspace(static_cast<unsigned char>(source[left - 1])) != 0) {
        --left;
    }
    size_t right = extractedRange.end;
    while (right < source.size() && std::isspace(static_cast<unsigned char>(source[right])) != 0) {
        ++right;
    }
    const bool hasLeadingAdjacentComment = left >= 2 && source[left - 2] == '*' && source[left - 1] == '/';
    const bool hasTrailingAdjacentComment =
        right + 1 < source.size() && source[right] == '/' && source[right + 1] == '*';
    if (!hasLeadingAdjacentComment && !hasTrailingAdjacentComment) {
        return false;
    }
    replaceRange = extractedRange;
    replacementText = declarationCore;
    return true;
}

ir::AstNode *FindEnclosingExprStmtBySpan(ir::AstNode *node, TextRange span)
{
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (!current->IsExpressionStatement()) {
            continue;
        }
        if (current->Start().index <= span.pos && current->End().index >= span.end) {
            return current;
        }
    }
    return nullptr;
}

ir::AstNode *TryResolveExprStmtByNode(const RefactorContext &context, const SourceFile *src, ir::AstNode *node)
{
    const TextRange trimmed = GetTrimmedSelectionSpan(context);
    auto *stmt = FindEnclosingExprStmtBySpan(node, trimmed);
    return ShouldDeleteWholeExprStmtBySpan(src, trimmed, stmt) ? stmt : nullptr;
}

ir::AstNode *ResolveExprStmtForValueExtraction(const RefactorContext &context, ir::AstNode *extractedText,
                                               const std::string &actionName, ir::AstNode *exprStmt,
                                               const SourceFile *src)
{
    if (exprStmt == nullptr && extractedText != nullptr && extractedText->IsExpressionStatement() &&
        IsVariableExtractionAction(actionName) && extractedText->Start().index <= context.span.pos &&
        extractedText->End().index >= context.span.end) {
        exprStmt = extractedText;
    }

    if (exprStmt == nullptr && (IsVariableExtractionAction(actionName) || IsConstantExtractionAction(actionName))) {
        if (auto *stmt = TryResolveExprStmtByNode(context, src, extractedText); stmt != nullptr) {
            exprStmt = stmt;
        }
    }
    if (exprStmt == nullptr && (IsVariableExtractionAction(actionName) || IsConstantExtractionAction(actionName))) {
        if (auto *stmt =
                TryResolveExprStmtByNode(context, src, GetTouchingTokenByRange(context.context, context.span, false));
            stmt != nullptr) {
            exprStmt = stmt;
        }
    }
    if (IsConstantExtractionInClassAction(actionName)) {
        return nullptr;
    }
    return exprStmt;
}

bool TryApplyExprStmtExtractionEdit(ChangeTracker &tracker, const TryApplyExprStmtExtractionEditInputs &inputs)
{
    if (inputs.exprStmt == nullptr || inputs.src == nullptr) {
        return false;
    }
    TextRange replaceRange {};
    std::string replacementText;
    const std::string declarationCore = BuildDeclarationCoreFromInsertedText(inputs.insertionData.second);
    const bool isGlobalConstant = IsActionNameOrKind(inputs.actionName, EXTRACT_CONSTANT_ACTION_GLOBAL);
    const bool hasCommentAdjacentReplacement = TryBuildCommentAdjacentReplacement(
        inputs.src->source, inputs.extractedRange, declarationCore, replaceRange, replacementText);
    if (hasCommentAdjacentReplacement) {
        if (isGlobalConstant) {
            tracker.InsertText(inputs.src, inputs.insertionData.first, inputs.insertionData.second);
            tracker.ReplaceRangeWithText(inputs.src, inputs.extractedRange, "");
        } else {
            tracker.ReplaceRangeWithText(inputs.src, replaceRange, replacementText);
        }
        return true;
    }
    tracker.InsertText(inputs.src, inputs.insertionData.first, inputs.insertionData.second);
    tracker.DeleteRange(inputs.src, TextRange {inputs.exprStmt->Start().index, inputs.exprStmt->End().index});
    return true;
}

std::vector<FileTextChanges> BuildFunctionExtractionTextChanges(const FunctionExtractionTextChangeInputs &inputs)
{
    TextChangesContext textChangesContext = *inputs.context.textChangesContext;
    const auto src = reinterpret_cast<public_lib::Context *>(inputs.context.context)->sourceFile;
    std::string adjustedFunctionText = inputs.functionText;
    if (src != nullptr) {
        const auto &source = src->source;
        size_t probe = std::min(inputs.insertPos, source.size());
        while (probe < source.size() && IsIndentChar(source[probe])) {
            ++probe;
        }
        if (probe < source.size() && IsLineBreakChar(source[probe])) {
            probe = NormalizeInsertPos(source, probe);
            while (probe < source.size() && IsIndentChar(source[probe])) {
                ++probe;
            }
        }
        const std::string newLine =
            inputs.context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();
        const std::string doubleNewLine = newLine + newLine;
        const auto namespaceDepth = GetNamespaceActionDepth(inputs.actionName, EXTRACT_FUNCTION_ACTION_ENCLOSE.name,
                                                            EXTRACT_FUNCTION_NAMESPACE_ACTION_PREFIX);
        const bool trimTrailingBlankLineBeforeClosingBrace = !namespaceDepth.has_value() || namespaceDepth.value() == 1;
        if (trimTrailingBlankLineBeforeClosingBrace && probe < source.size() && source[probe] == '}' &&
            adjustedFunctionText.size() >= doubleNewLine.size() &&
            adjustedFunctionText.compare(adjustedFunctionText.size() - doubleNewLine.size(), doubleNewLine.size(),
                                         doubleNewLine) == 0) {
            adjustedFunctionText.erase(adjustedFunctionText.size() - newLine.size());
        }
    }
    return ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        tracker.InsertText(src, inputs.insertPos, adjustedFunctionText);
        tracker.ReplaceRangeWithText(src, inputs.extractionPos, inputs.funcCallText);
    });
}

bool RangeEndsWithStatementSemicolon(std::string_view source, TextRange range)
{
    if (range.end <= range.pos || range.end > source.size()) {
        return false;
    }
    size_t end = range.end;
    while (end > range.pos && std::isspace(static_cast<unsigned char>(source[end - 1])) != 0) {
        --end;
    }
    return end > range.pos && source[end - 1] == ';';
}

size_t ComputeFunctionRenameLoc(const std::vector<FileTextChanges> &edits, TextRange extractionPos)
{
    size_t renameLoc = ComputeRenameLocFromEdits(edits, extractionPos.pos);
    if (auto renameLocOnCall = ComputeFunctionCallRenameLocFromEdits(edits, extractionPos);
        renameLocOnCall.has_value()) {
        renameLoc = renameLocOnCall.value();
    }
    return renameLoc;
}

struct TopLevelDeclLeadingInfo {
    bool isConst {false};
    size_t lineStart {0};
    std::string leadingText;
    std::string declaratorText;
    std::string varName;
    std::string_view selected;
};

static bool IsTopLevelDeclLeadingSelectionShape(const SourceFile *sourceFile, TextRange extractionPos,
                                                size_t &lineStart, std::string_view &selected, size_t &eqPos)
{
    if (sourceFile == nullptr || extractionPos.end <= extractionPos.pos ||
        extractionPos.end > sourceFile->source.size()) {
        return false;
    }
    const auto &source = sourceFile->source;
    auto [computedLineStart, indentEnd] = ComputeLineIndent(util::StringView(source), extractionPos.pos);
    if (computedLineStart != extractionPos.pos || indentEnd != computedLineStart) {
        size_t statementStart = indentEnd;
        while (statementStart < extractionPos.pos) {
            if (std::isspace(static_cast<unsigned char>(source[statementStart])) != 0) {
                ++statementStart;
                continue;
            }
            if (source.compare(statementStart, std::strlen("/*"), "/*") != 0) {
                return false;
            }
            const size_t commentEnd = source.find("*/", statementStart + std::strlen("/*"));
            if (commentEnd == std::string::npos || commentEnd + std::strlen("*/") > extractionPos.pos) {
                return false;
            }
            statementStart = commentEnd + std::strlen("*/");
        }
        if (statementStart != extractionPos.pos) {
            return false;
        }
    }
    if (indentEnd != computedLineStart) {
        return false;
    }
    selected = std::string_view(source.data() + extractionPos.pos, extractionPos.end - extractionPos.pos);
    const bool startsWithDecl = selected.rfind("const ", 0) == 0 || selected.rfind("let ", 0) == 0;
    const bool hasNewline = selected.find('\n') != std::string_view::npos;
    eqPos = selected.find('=');
    const size_t semiPos = selected.find(';');
    if (!startsWithDecl || !hasNewline || eqPos == std::string_view::npos || semiPos == std::string_view::npos ||
        eqPos >= semiPos) {
        return false;
    }
    lineStart = computedLineStart;
    return true;
}

static std::optional<std::pair<std::string, std::string>> ParseTopLevelDeclaratorNameAndText(std::string_view selected,
                                                                                             size_t eqPos)
{
    const bool isConst = selected.rfind("const ", 0) == 0;
    const size_t keywordLen = isConst ? std::string_view("const ").size() : std::string_view("let ").size();
    size_t declStart = keywordLen;
    while (declStart < eqPos && std::isspace(static_cast<unsigned char>(selected[declStart])) != 0) {
        ++declStart;
    }
    size_t declEnd = eqPos;
    while (declEnd > declStart && std::isspace(static_cast<unsigned char>(selected[declEnd - 1])) != 0) {
        --declEnd;
    }
    if (declEnd <= declStart) {
        return std::nullopt;
    }

    const std::string declaratorText(selected.substr(declStart, declEnd - declStart));
    size_t nameStart = 0;
    while (nameStart < declaratorText.size() &&
           std::isspace(static_cast<unsigned char>(declaratorText[nameStart])) != 0) {
        ++nameStart;
    }
    size_t nameEnd = nameStart;
    while (nameEnd < declaratorText.size()) {
        const char ch = declaratorText[nameEnd];
        if (std::isalnum(static_cast<unsigned char>(ch)) == 0 && ch != '_' && ch != '$') {
            break;
        }
        ++nameEnd;
    }
    if (nameEnd <= nameStart) {
        return std::nullopt;
    }
    return std::make_pair(declaratorText, declaratorText.substr(nameStart, nameEnd - nameStart));
}

static bool TryParseTopLevelDeclLeadingSelection(const SourceFile *sourceFile, TextRange extractionPos,
                                                 TopLevelDeclLeadingInfo &out)
{
    size_t lineStart = 0;
    std::string_view selected;
    size_t eqPos = 0;
    if (!IsTopLevelDeclLeadingSelectionShape(sourceFile, extractionPos, lineStart, selected, eqPos)) {
        return false;
    }
    const auto parsed = ParseTopLevelDeclaratorNameAndText(selected, eqPos);
    if (!parsed.has_value()) {
        return false;
    }
    const bool isConst = selected.rfind("const ", 0) == 0;
    out.isConst = isConst;
    out.lineStart = lineStart;
    if (sourceFile != nullptr && extractionPos.pos >= lineStart) {
        out.leadingText.assign(sourceFile->source.data() + lineStart, extractionPos.pos - lineStart);
    }
    out.declaratorText = parsed->first;
    out.varName = parsed->second;
    out.selected = selected;
    return true;
}

static std::string GetDeclaredReturnTypeFromTopLevelDeclarator(std::string_view declaratorText)
{
    const size_t colonPos = declaratorText.find(':');
    if (colonPos == std::string_view::npos || colonPos + 1U >= declaratorText.size()) {
        return "";
    }
    std::string declaredType(declaratorText.substr(colonPos + 1U));
    const std::string trimmedType = TrimAsciiWhitespace(declaredType);
    return trimmedType.empty() ? "" : ": " + trimmedType;
}

static std::string InferReturnTypeFromTopLevelBinding(const RefactorContext &context, public_lib::Context *ctx,
                                                      TextRange extractionPos)
{
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return "";
    }
    std::string returnTypeAnnotation;
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (!returnTypeAnnotation.empty()) {
            return true;
        }
        if (node == nullptr || !node->IsVariableDeclaration() || node->Start().index != extractionPos.pos) {
            return false;
        }
        auto *decl = node->AsVariableDeclaration();
        if (decl == nullptr || decl->Declarators().empty()) {
            return false;
        }
        auto *declarator = decl->Declarators().front();
        if (declarator == nullptr || declarator->Id() == nullptr || !declarator->Id()->IsIdentifier() ||
            declarator->Init() == nullptr || !declarator->Init()->IsExpression()) {
            return false;
        }
        VariableBindingInfo binding {decl, declarator, declarator->Id()->AsIdentifier(), declarator->Init()};
        returnTypeAnnotation = ResolveReturnTypeAnnotationForBinding(context, binding, false, true);
        return !returnTypeAnnotation.empty();
    });
    return returnTypeAnnotation;
}

static std::string InferReturnTypeFromTopLevelLiteral(std::string_view selected)
{
    const size_t eqPos = selected.find('=');
    const size_t semiPos = selected.find(';');
    if (eqPos == std::string_view::npos || semiPos == std::string_view::npos || eqPos >= semiPos) {
        return "";
    }
    size_t rhsStart = eqPos + 1;
    while (rhsStart < semiPos && std::isspace(static_cast<unsigned char>(selected[rhsStart])) != 0) {
        ++rhsStart;
    }
    size_t rhsEnd = semiPos;
    while (rhsEnd > rhsStart && std::isspace(static_cast<unsigned char>(selected[rhsEnd - 1])) != 0) {
        --rhsEnd;
    }
    const std::string firstInit(selected.substr(rhsStart, rhsEnd - rhsStart));
    const bool isIntLiteral = !firstInit.empty() && std::all_of(firstInit.begin(), firstInit.end(), [](char c) {
        return std::isdigit(static_cast<unsigned char>(c)) != 0;
    });
    if (isIntLiteral) {
        return ": int";
    }
    if (firstInit.size() >= 2U && firstInit.front() == '"' && firstInit.back() == '"') {
        return ": String";
    }
    if (firstInit == "true" || firstInit == "false") {
        return ": boolean";
    }
    return "";
}

static std::string InferTopLevelDeclLeadingReturnType(const RefactorContext &context, public_lib::Context *ctx,
                                                      TextRange extractionPos, std::string_view declaratorText,
                                                      std::string_view selected)
{
    std::string returnTypeAnnotation = GetDeclaredReturnTypeFromTopLevelDeclarator(declaratorText);
    if (returnTypeAnnotation.empty()) {
        returnTypeAnnotation = InferReturnTypeFromTopLevelBinding(context, ctx, extractionPos);
    }
    if (!returnTypeAnnotation.empty()) {
        return returnTypeAnnotation;
    }
    return InferReturnTypeFromTopLevelLiteral(selected);
}

static std::string BuildTopLevelDeclLeadingHelperText(const RefactorContext &context, std::string_view body,
                                                      std::string_view helperName,
                                                      std::string_view returnTypeAnnotation, std::string_view varName)
{
    const std::string newLine = context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();
    const std::string indentStep(ResolveIndentSize(context), SPACE_CHAR);
    std::string helper;
    helper.reserve(body.size() + varName.size() + helperName.size() + K_HELPER_RESERVE_PADDING);
    helper.append(newLine)
        .append("function ")
        .append(helperName)
        .append("()")
        .append(returnTypeAnnotation)
        .append(" {")
        .append(newLine);
    std::istringstream lines {std::string(body)};
    std::string line;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        helper.append(indentStep).append(line).append(newLine);
    }
    helper.append(indentStep).append("return ").append(varName).append(";").append(newLine);
    helper.append("}").append(newLine);
    return helper;
}

bool TryBuildTopLevelDeclarationLeadingFunctionExtraction(const RefactorContext &context, public_lib::Context *ctx,
                                                          TextRange extractionPos, RefactorEditInfo &out)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return false;
    }
    TopLevelDeclLeadingInfo parsed;
    if (!TryParseTopLevelDeclLeadingSelection(ctx->sourceFile, extractionPos, parsed)) {
        return false;
    }
    const auto &source = ctx->sourceFile->source;
    std::string helperName =
        GenerateUniqueFuncName(context, "newFunction", std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name));
    const std::string returnTypeAnnotation =
        InferTopLevelDeclLeadingReturnType(context, ctx, extractionPos, parsed.declaratorText, parsed.selected);
    std::string body(parsed.selected);
    TrimTrailingNewlines(body);
    const std::string helper =
        BuildTopLevelDeclLeadingHelperText(context, body, helperName, returnTypeAnnotation, parsed.varName);
    const std::string newLine = context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();
    std::string replacement = parsed.leadingText + (parsed.isConst ? "const " : "let ") + parsed.declaratorText +
                              " = " + helperName + "();" + newLine;
    TextRange replaceRange {parsed.lineStart, ExtendToLineEnd(util::StringView(source), extractionPos.end)};
    HelperPieces pieces;
    pieces.insertHelper = true;
    pieces.insertPos = DetermineGlobalInsertPos(ctx);
    pieces.helperText = std::move(helper);
    pieces.replaceRange = replaceRange;
    pieces.replacementText = std::move(replacement);

    TextChangesContext textChangesContext = *context.textChangesContext;
    auto edits = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        tracker.InsertText(ctx->sourceFile, pieces.insertPos, pieces.helperText);
        tracker.ReplaceRangeWithText(ctx->sourceFile, pieces.replaceRange, pieces.replacementText);
    });
    out = RefactorEditInfo(std::move(edits), std::optional<std::string>(ctx->sourceFile->filePath),
                           std::optional<size_t>(FindRenameIndex(pieces)));
    return true;
}

static bool CanUseDeclarationLeadingGlobalFunctionExtraction(const VariableBindingInfo &binding,
                                                             TextRange extractionPos)
{
    if (binding.declaration == nullptr || binding.identifier == nullptr || binding.initializer == nullptr) {
        return false;
    }
    if (binding.declaration->Start().index != extractionPos.pos ||
        extractionPos.end <= binding.declaration->End().index) {
        return false;
    }
    if (FindScriptFunction(binding.declaration) != nullptr || IsNamespaceContext(binding.declaration)) {
        return false;
    }
    auto *classDef = FindEnclosingClassDefinition(binding.declaration);
    return classDef == nullptr || classDef->IsGlobal();
}

static std::optional<RefactorEditInfo> BuildDeclarationLeadingGlobalFunctionEdits(const RefactorContext &context,
                                                                                  public_lib::Context *ctx,
                                                                                  TextRange extractionPos,
                                                                                  const VariableBindingInfo &binding)
{
    HelperPieces pieces;
    if (!BuildGlobalPiecesFromDeclarationSelection(context, binding, extractionPos, pieces)) {
        return std::nullopt;
    }
    TextChangesContext textChangesContext = *context.textChangesContext;
    auto edits = ChangeTracker::With(textChangesContext, [ctx, &pieces](ChangeTracker &tracker) {
        if (pieces.insertHelper && !pieces.helperText.empty()) {
            tracker.InsertText(ctx->sourceFile, pieces.insertPos, pieces.helperText);
        }
        tracker.ReplaceRangeWithText(ctx->sourceFile, pieces.replaceRange, pieces.replacementText);
    });
    return RefactorEditInfo(std::move(edits), std::optional<std::string>(ctx->sourceFile->filePath),
                            std::optional<size_t>(FindRenameIndex(pieces)));
}

static std::optional<RefactorEditInfo> TryBuildDeclarationLeadingGlobalFunctionExtraction(
    const RefactorContext &context, public_lib::Context *ctx, TextRange extractionPos)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || ctx->parserProgram == nullptr ||
        ctx->parserProgram->Ast() == nullptr || extractionPos.end <= extractionPos.pos) {
        return std::nullopt;
    }
    bool handledDeclarationLeadingSelection = false;
    RefactorEditInfo declarationLeadingEdits;
    ctx->parserProgram->Ast()->FindChild([&context, ctx, extractionPos, &declarationLeadingEdits,
                                          &handledDeclarationLeadingSelection](ir::AstNode *node) {
        if (handledDeclarationLeadingSelection || node == nullptr || !node->IsVariableDeclaration()) {
            return false;
        }
        VariableBindingInfo binding;
        if (!ResolveVariableBinding(node, binding) ||
            !CanUseDeclarationLeadingGlobalFunctionExtraction(binding, extractionPos)) {
            return false;
        }
        if (auto edits = BuildDeclarationLeadingGlobalFunctionEdits(context, ctx, extractionPos, binding);
            edits.has_value()) {
            declarationLeadingEdits = std::move(edits.value());
            handledDeclarationLeadingSelection = true;
            return true;
        }
        return false;
    });
    if (!handledDeclarationLeadingSelection) {
        return std::nullopt;
    }
    return declarationLeadingEdits;
}

bool MatchesSelectionWithOptionalSemicolon(public_lib::Context *ctx, TextRange extractionPos, const ir::AstNode *node)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || node == nullptr) {
        return false;
    }
    const size_t start = node->Start().index;
    const size_t end = node->End().index;
    if (start != extractionPos.pos || end < extractionPos.end) {
        return false;
    }
    if (end == extractionPos.end) {
        return true;
    }
    const auto &source = ctx->sourceFile->source;
    if (extractionPos.end >= source.size() || end > source.size()) {
        return false;
    }
    for (size_t i = extractionPos.end; i < end; ++i) {
        char ch = source[i];
        if (ch == ';' || std::isspace(static_cast<unsigned char>(ch)) != 0) {
            continue;
        }
        return false;
    }
    return true;
}

static bool TryBuildWholeGlobalDeclarationBinding(public_lib::Context *ctx, TextRange extractionPos, ir::AstNode *node,
                                                  VariableBindingInfo &outBinding)
{
    if (node == nullptr || !node->IsVariableDeclaration()) {
        return false;
    }
    auto *vd = node->AsVariableDeclaration();
    if (vd == nullptr || vd->Declarators().empty()) {
        return false;
    }
    auto *firstDecl = vd->Declarators().front();
    if (firstDecl == nullptr || firstDecl->Id() == nullptr || !firstDecl->Id()->IsIdentifier() ||
        firstDecl->Init() == nullptr || !firstDecl->Init()->IsExpression()) {
        return false;
    }
    if (vd->Start().index != extractionPos.pos || extractionPos.end <= vd->End().index || IsNamespaceContext(vd)) {
        return false;
    }
    auto *enclosingClass = FindEnclosingClassDefinition(vd);
    if (enclosingClass != nullptr && !enclosingClass->IsGlobal()) {
        return false;
    }
    auto [lineStart, indentEnd] = ComputeLineIndent(util::StringView(ctx->sourceFile->source), vd->Start().index);
    if (indentEnd != lineStart) {
        return false;
    }
    outBinding = {vd, firstDecl, firstDecl->Id()->AsIdentifier(), firstDecl->Init()};
    return true;
}

static bool FindWholeGlobalDeclarationBinding(public_lib::Context *ctx, TextRange extractionPos,
                                              VariableBindingInfo &binding)
{
    bool found = false;
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (found) {
            return true;
        }
        found = TryBuildWholeGlobalDeclarationBinding(ctx, extractionPos, node, binding);
        return found;
    });
    return found;
}

static std::optional<RefactorEditInfo> TryBuildWholeGlobalDeclarationFunctionExtraction(const RefactorContext &context,
                                                                                        public_lib::Context *ctx,
                                                                                        TextRange extractionPos)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || ctx->parserProgram == nullptr ||
        ctx->parserProgram->Ast() == nullptr || extractionPos.end <= extractionPos.pos) {
        return std::nullopt;
    }
    VariableBindingInfo binding;
    if (!FindWholeGlobalDeclarationBinding(ctx, extractionPos, binding)) {
        return std::nullopt;
    }
    return BuildDeclarationLeadingGlobalFunctionEdits(context, ctx, extractionPos, binding);
}

static bool TryRewriteExtractionToBindingInitializer(public_lib::Context *ctx, TextRange &extractionPos,
                                                     ir::AstNode *&extractedNode, ir::AstNode *candidate)
{
    VariableBindingInfo binding;
    if (!ResolveVariableBinding(candidate, binding) || binding.declaration == nullptr ||
        binding.initializer == nullptr ||
        !MatchesSelectionWithOptionalSemicolon(ctx, extractionPos, binding.declaration)) {
        return false;
    }
    extractionPos = {binding.initializer->Start().index, binding.initializer->End().index};
    extractedNode = binding.initializer;
    return true;
}

static bool RewriteWholeDeclarationSelectionFromAst(public_lib::Context *ctx, TextRange &extractionPos,
                                                    ir::AstNode *&extractedNode)
{
    bool rewritten = false;
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (rewritten || node == nullptr || !node->IsVariableDeclaration() ||
            !MatchesSelectionWithOptionalSemicolon(ctx, extractionPos, node)) {
            return false;
        }
        auto *decl = node->AsVariableDeclaration();
        if (decl == nullptr || decl->Declarators().empty()) {
            return false;
        }
        auto *init = decl->Declarators().front()->Init();
        if (init == nullptr || !init->IsExpression()) {
            return false;
        }
        extractionPos = {init->Start().index, init->End().index};
        extractedNode = init;
        rewritten = true;
        return true;
    });
    return rewritten;
}

static bool TryRewriteExtractionAlongParents(public_lib::Context *ctx, TextRange &extractionPos,
                                             ir::AstNode *&extractedNode, ir::AstNode *startNode)
{
    for (auto *current = startNode; current != nullptr; current = current->Parent()) {
        if (TryRewriteExtractionToBindingInitializer(ctx, extractionPos, extractedNode, current)) {
            return true;
        }
    }
    return false;
}

static bool TryRewriteExtractionFromTouchPos(const RefactorContext &context, public_lib::Context *ctx,
                                             TextRange &extractionPos, ir::AstNode *&extractedNode, size_t pos)
{
    auto *touchNode = GetTouchingToken(context.context, pos, false);
    return TryRewriteExtractionAlongParents(ctx, extractionPos, extractedNode, touchNode);
}

static bool TryRewriteExtractionFromKnownNodes(const RefactorContext &context, public_lib::Context *ctx,
                                               TextRange &extractionPos, ir::AstNode *&extractedNode)
{
    auto *spanNode = GetTouchingTokenByRange(context.context, extractionPos, false);
    if (TryRewriteExtractionAlongParents(ctx, extractionPos, extractedNode, spanNode)) {
        return true;
    }
    if (TryRewriteExtractionToBindingInitializer(ctx, extractionPos, extractedNode, extractedNode)) {
        return true;
    }
    return TryRewriteExtractionAlongParents(ctx, extractionPos, extractedNode, extractedNode);
}

static bool TryRewriteExtractionFromTouchPositions(const RefactorContext &context, public_lib::Context *ctx,
                                                   TextRange &extractionPos, ir::AstNode *&extractedNode)
{
    if (TryRewriteExtractionFromTouchPos(context, ctx, extractionPos, extractedNode, extractionPos.pos)) {
        return true;
    }
    return extractionPos.pos > 0 &&
           TryRewriteExtractionFromTouchPos(context, ctx, extractionPos, extractedNode, extractionPos.pos - 1);
}

bool TryRewriteExtractionToWholeDeclarationInitializer(const RefactorContext &context, public_lib::Context *ctx,
                                                       TextRange &extractionPos, ir::AstNode *&extractedNode)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || ctx->parserProgram == nullptr ||
        ctx->parserProgram->Ast() == nullptr) {
        return false;
    }
    if (RewriteWholeDeclarationSelectionFromAst(ctx, extractionPos, extractedNode)) {
        return true;
    }
    if (TryRewriteExtractionFromKnownNodes(context, ctx, extractionPos, extractedNode)) {
        return true;
    }
    if (TryRewriteExtractionFromTouchPositions(context, ctx, extractionPos, extractedNode)) {
        return true;
    }
    return RewriteWholeDeclarationSelectionFromAst(ctx, extractionPos, extractedNode);
}

bool TryRewriteExtractionToTextInitializer(const RefactorContext &context, public_lib::Context *ctx,
                                           TextRange &extractionPos, ir::AstNode *&extractedNode)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || extractionPos.end <= extractionPos.pos ||
        extractionPos.end > ctx->sourceFile->source.size()) {
        return false;
    }
    const std::string_view selected(ctx->sourceFile->source.data() + extractionPos.pos,
                                    extractionPos.end - extractionPos.pos);
    if (!(selected.rfind("let ", 0) == 0 || selected.rfind("const ", 0) == 0)) {
        return false;
    }
    const size_t eqPos = selected.find('=');
    if (eqPos == std::string_view::npos) {
        return false;
    }
    size_t rhsStart = eqPos + 1;
    while (rhsStart < selected.size() && std::isspace(static_cast<unsigned char>(selected[rhsStart])) != 0) {
        ++rhsStart;
    }
    size_t rhsEnd = selected.size();
    while (rhsEnd > rhsStart && std::isspace(static_cast<unsigned char>(selected[rhsEnd - 1])) != 0) {
        --rhsEnd;
    }
    if (rhsEnd <= rhsStart) {
        return false;
    }
    extractionPos = {extractionPos.pos + rhsStart, extractionPos.pos + rhsEnd};
    extractedNode = GetTouchingToken(context.context, extractionPos.pos, false);
    while (extractedNode != nullptr && !extractedNode->IsExpression()) {
        extractedNode = extractedNode->Parent();
    }
    return extractedNode != nullptr;
}

void NormalizeWholeDeclarationExtraction(public_lib::Context *ctx, TextRange &extractionPos,
                                         ir::AstNode *&extractedNode)
{
    VariableBindingInfo selectionBinding;
    if (ResolveVariableBinding(extractedNode, selectionBinding) && selectionBinding.declaration != nullptr &&
        selectionBinding.initializer != nullptr &&
        MatchesSelectionWithOptionalSemicolon(ctx, extractionPos, selectionBinding.declaration)) {
        extractedNode = selectionBinding.initializer;
        extractionPos = {selectionBinding.initializer->Start().index, selectionBinding.initializer->End().index};
    }
}

bool IsDeclarationLeadingSelection(const RefactorContext &context, const std::string &actionName)
{
    const TextRange trimmedSpan = GetTrimmedSelectionSpan(context);
    if (actionName != std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name) || trimmedSpan.end <= trimmedSpan.pos) {
        return false;
    }
    VariableBindingInfo binding;
    auto *touch = GetTouchingToken(context.context, trimmedSpan.pos, false);
    if (touch == nullptr) {
        touch = FindTouchingTokenNearSpan(context);
    }
    for (auto *current = touch; current != nullptr; current = current->Parent()) {
        if (!ResolveVariableBinding(current, binding) || binding.declaration == nullptr) {
            continue;
        }
        return binding.declaration->Start().index == trimmedSpan.pos &&
               trimmedSpan.end > binding.declaration->End().index;
    }
    return false;
}

std::optional<RefactorEditInfo> TryBuildEarlyGlobalFunctionExtraction(const RefactorContext &context,
                                                                      public_lib::Context *ctx, TextRange extractionPos)
{
    if (auto topLevelEdits = RefactorEditInfo();
        TryBuildTopLevelDeclarationLeadingFunctionExtraction(context, ctx, extractionPos, topLevelEdits)) {
        return topLevelEdits;
    }
    if (auto wholeDeclEdits = TryBuildWholeGlobalDeclarationFunctionExtraction(context, ctx, extractionPos);
        wholeDeclEdits.has_value()) {
        return wholeDeclEdits.value();
    }
    return TryBuildDeclarationLeadingGlobalFunctionExtraction(context, ctx, extractionPos);
}

static void UpdateRenameLocFromFinalText(const std::string &finalText, const std::string &uniqueVarName,
                                         const std::optional<TextRange> &insertedRange, size_t &renameLoc)
{
    if (auto updatedLoc = RecomputeRenameLoc(finalText, uniqueVarName, renameLoc, insertedRange);
        updatedLoc.has_value()) {
        renameLoc = updatedLoc.value();
    }
    if (uniqueVarName.rfind("this.", 0) == 0) {
        return;
    }
    const size_t first = finalText.find(uniqueVarName);
    if (first == std::string::npos) {
        return;
    }
    const size_t second = finalText.find(uniqueVarName, first + uniqueVarName.size());
    if (second == std::string::npos) {
        return;
    }
    renameLoc = second + (uniqueVarName.size() > 1 ? 1 : 0);
}

static void NormalizeRenameLocInsideIdentifier(const std::string &finalText, const std::string &uniqueVarName,
                                               size_t &renameLoc)
{
    if (uniqueVarName.empty() || renameLoc >= finalText.size()) {
        return;
    }
    if (uniqueVarName.rfind("this.", 0) == 0) {
        const std::string suffix = uniqueVarName.substr(std::string("this.").size());
        if (!suffix.empty() && renameLoc + suffix.size() <= finalText.size() &&
            finalText.compare(renameLoc, suffix.size(), suffix) == 0) {
            renameLoc += suffix.size() > 1 ? 1 : 0;
        }
        return;
    }
    if (renameLoc + uniqueVarName.size() <= finalText.size() &&
        finalText.compare(renameLoc, uniqueVarName.size(), uniqueVarName) == 0) {
        renameLoc += uniqueVarName.size() > 1 ? 1 : 0;
    }
}

static void UpdateRenameLocByExpectedTokenRule(const std::string &finalText, const std::string &uniqueVarName,
                                               size_t &renameLoc)
{
    if (finalText.empty() || uniqueVarName.empty()) {
        return;
    }
    if (uniqueVarName.rfind("this.", 0) == 0) {
        const size_t base = finalText.find(uniqueVarName);
        if (base == std::string::npos) {
            return;
        }
        renameLoc = base + std::string("this.").size() + 1;
        return;
    }
    const size_t first = finalText.find(uniqueVarName);
    if (first == std::string::npos) {
        return;
    }
    const size_t second = finalText.find(uniqueVarName, first + uniqueVarName.size());
    const size_t targetPos = (second == std::string::npos) ? first : second;
    renameLoc = targetPos + (uniqueVarName.size() > 1 ? 1 : 0);
}

struct RenameLocFinalTextInputs {
    std::string_view sourceText;
    const std::vector<const TextChange *> &orderedChanges;
    bool renameLocIsFinal {false};
    size_t adjustedInsertPos {0};
    const std::string &implicitPrefix;
    const TextChange *insertChange {nullptr};
    const std::string &uniqueVarName;
    size_t &renameLoc;
};

static void UpdateRenameLocFromFinalTextIfNeeded(const RenameLocFinalTextInputs &inputs)
{
    if (inputs.renameLocIsFinal || inputs.orderedChanges.empty()) {
        return;
    }
    std::string finalText = BuildFinalText(inputs.sourceText, inputs.orderedChanges);
    ApplyImplicitPrefix(finalText, inputs.adjustedInsertPos, inputs.implicitPrefix);
    auto insertedRange =
        inputs.adjustedInsertPos <= finalText.size()
            ? ComputeInsertedRange(inputs.adjustedInsertPos, inputs.insertChange, inputs.implicitPrefix)
            : std::optional<TextRange> {};
    UpdateRenameLocFromFinalText(finalText, inputs.uniqueVarName, insertedRange, inputs.renameLoc);
    NormalizeRenameLocInsideIdentifier(finalText, inputs.uniqueVarName, inputs.renameLoc);
}

struct UsageRenameLocInputs {
    const std::string &actionName;
    std::string_view sourceText;
    const std::vector<FileTextChanges> &edits;
    const std::string &uniqueVarName;
    const std::string &implicitPrefix;
    size_t adjustedInsertPos {0};
    size_t &renameLoc;
};

static void UpdateUsageRenameLocIfNeeded(const UsageRenameLocInputs &inputs)
{
    if (!IsVariableExtractionAction(inputs.actionName)) {
        return;
    }
    if (auto usageLoc = ComputeVariableUsageRenameLocFromEdits(inputs.edits, inputs.uniqueVarName);
        usageLoc.has_value()) {
        inputs.renameLoc = usageLoc.value();
        if (!inputs.implicitPrefix.empty() && inputs.adjustedInsertPos <= inputs.renameLoc) {
            size_t implicitShift = inputs.implicitPrefix.size();
            if (inputs.implicitPrefix[0] == '\n' && inputs.adjustedInsertPos < inputs.sourceText.size() &&
                IsLineBreakChar(inputs.sourceText[inputs.adjustedInsertPos]) && implicitShift > 0) {
                --implicitShift;
            }
            inputs.renameLoc += implicitShift;
        }
    }
}

struct RenameLocExpectedTokenInputs {
    const std::string &actionName;
    std::string_view sourceText;
    const std::vector<const TextChange *> &orderedChanges;
    size_t adjustedInsertPos {0};
    const std::string &implicitPrefix;
    const std::string &uniqueVarName;
    size_t &renameLoc;
};

static void UpdateRenameLocByExpectedTokenIfNeeded(const RenameLocExpectedTokenInputs &inputs)
{
    if (!IsVariableExtractionAction(inputs.actionName)) {
        return;
    }
    if (inputs.orderedChanges.empty()) {
        return;
    }
    std::string finalText = BuildFinalText(inputs.sourceText, inputs.orderedChanges);
    ApplyImplicitPrefix(finalText, inputs.adjustedInsertPos, inputs.implicitPrefix);
    UpdateRenameLocByExpectedTokenRule(finalText, inputs.uniqueVarName, inputs.renameLoc);
}

static size_t ResolveInitialValueExtractionRenameLoc(const ValueExtractionRenameLocInputs &inputs,
                                                     bool &renameLocIsFinal)
{
    size_t renameLoc = inputs.extractedText->Start().index;
    renameLocIsFinal = false;
    if (auto renameLocForExpr =
            ComputeRenameLocForExprStmt(inputs.exprStmt, inputs.generatedText, inputs.uniqueVarName, inputs.insertPos);
        renameLocForExpr.has_value()) {
        renameLoc = renameLocForExpr.value();
        renameLocIsFinal = true;
    }
    return renameLoc;
}

static void ApplyImplicitPrefixRenameShift(const ValueExtractionRenameLocInputs &inputs, size_t adjustedInsertPos,
                                           size_t &renameLoc)
{
    if (inputs.implicitPrefix.empty() || adjustedInsertPos > renameLoc) {
        return;
    }
    size_t implicitShift = inputs.implicitPrefix.size();
    if (inputs.implicitPrefix[0] == '\n' && adjustedInsertPos < inputs.sourceText.size() &&
        IsLineBreakChar(inputs.sourceText[adjustedInsertPos]) && implicitShift > 0) {
        --implicitShift;
    }
    renameLoc += implicitShift;
}

static void ApplyConstantEncloseRenameLocAdjustments(const ValueExtractionRenameLocInputs &inputs,
                                                     const std::vector<const TextChange *> &orderedChanges,
                                                     size_t adjustedInsertPos, size_t &renameLoc)
{
    if (!IsActionNameOrKind(inputs.actionName, EXTRACT_CONSTANT_ACTION_ENCLOSE)) {
        return;
    }
    auto finalText = BuildFinalText(inputs.sourceText, orderedChanges);
    ApplyImplicitPrefix(finalText, adjustedInsertPos, inputs.implicitPrefix);
    const size_t first = finalText.find(inputs.uniqueVarName);
    if (first != std::string::npos) {
        const size_t second = finalText.find(inputs.uniqueVarName, first + inputs.uniqueVarName.size());
        if (second != std::string::npos && second > 0) {
            const size_t prev = second - 1;
            if (std::isalnum(static_cast<unsigned char>(finalText[prev])) != 0 || finalText[prev] == '_') {
                renameLoc = second;
            }
        }
    }
    if (inputs.extractedText != nullptr &&
        (inputs.extractedText->IsStringLiteral() || inputs.extractedText->IsTemplateLiteral() ||
         inputs.extractedText->IsCharLiteral()) &&
        renameLoc > 0) {
        --renameLoc;
    }
}

struct ValueExtractionRenamePostUpdateInputs {
    const std::vector<const TextChange *> &orderedChanges;
    bool renameLocIsFinal {false};
    size_t adjustedInsertPos {0};
    const TextChange *insertChange {nullptr};
};

static void ApplyValueExtractionRenameLocPostUpdates(const ValueExtractionRenameLocInputs &inputs,
                                                     const ValueExtractionRenamePostUpdateInputs &postInputs,
                                                     size_t &renameLoc)
{
    UpdateRenameLocFromFinalTextIfNeeded({inputs.sourceText, postInputs.orderedChanges, postInputs.renameLocIsFinal,
                                          postInputs.adjustedInsertPos, inputs.implicitPrefix, postInputs.insertChange,
                                          inputs.uniqueVarName, renameLoc});
    UpdateUsageRenameLocIfNeeded({inputs.actionName, inputs.sourceText, inputs.edits, inputs.uniqueVarName,
                                  inputs.implicitPrefix, postInputs.adjustedInsertPos, renameLoc});
    UpdateRenameLocByExpectedTokenIfNeeded({inputs.actionName, inputs.sourceText, postInputs.orderedChanges,
                                            postInputs.adjustedInsertPos, inputs.implicitPrefix, inputs.uniqueVarName,
                                            renameLoc});
}

size_t ResolveValueExtractionRenameLoc(const ValueExtractionRenameLocInputs &inputs)
{
    bool renameLocIsFinal = false;
    size_t renameLoc = ResolveInitialValueExtractionRenameLoc(inputs, renameLocIsFinal);
    const TextChange *insertChange = nullptr;
    size_t insertShift = 0;
    auto orderedChanges = CollectOrderedTextChanges(inputs.edits, inputs.insertPos, insertChange, insertShift);
    AdjustRenameLocFromChanges(orderedChanges, renameLocIsFinal, renameLoc);
    const size_t adjustedInsertPos = inputs.insertPos + insertShift;
    ApplyImplicitPrefixRenameShift(inputs, adjustedInsertPos, renameLoc);
    ApplyValueExtractionRenameLocPostUpdates(
        inputs, {orderedChanges, renameLocIsFinal, adjustedInsertPos, insertChange}, renameLoc);
    ApplyConstantEncloseRenameLocAdjustments(inputs, orderedChanges, adjustedInsertPos, renameLoc);
    return renameLoc;
}

static bool TryGetFunctionExtractionCandidate(const RefactorContext &context, FunctionExtraction &candidate)
{
    auto candidates = GetPossibleFunctionExtractions(context);
    if (candidates.empty()) {
        return false;
    }
    candidate = candidates.front();
    CollectFunctionParameters(candidate);
    return true;
}

static bool IsMultilineExpressionSelection(ir::AstNode *extractedNode, TextRange extractionPos,
                                           bool selectionHasNewline)
{
    return selectionHasNewline && extractedNode != nullptr && extractedNode->IsExpression() &&
           extractionPos.pos >= extractedNode->Start().index && extractionPos.end <= extractedNode->End().index;
}

struct MultilineSelectionState {
    TextRange extractionPos;
    bool selectionHasNewline {false};
    bool multilineExpressionSelection {false};
};

static void NormalizeMultilineFunctionSelection(const RefactorContext &context, public_lib::Context *ctx,
                                                ir::AstNode *extractedNode, MultilineSelectionState &state)
{
    state.selectionHasNewline = HasSourceNewlineInRange(ctx, state.extractionPos);
    state.multilineExpressionSelection =
        IsMultilineExpressionSelection(extractedNode, state.extractionPos, state.selectionHasNewline);
    if (state.selectionHasNewline && !state.multilineExpressionSelection) {
        state.extractionPos = GetCallPositionOfExtraction(context);
        state.selectionHasNewline = HasSourceNewlineInRange(ctx, state.extractionPos);
        state.multilineExpressionSelection = false;
        return;
    }
    if (extractedNode != nullptr && extractedNode->IsExpression() &&
        (state.extractionPos.pos > extractedNode->Start().index ||
         state.extractionPos.end < extractedNode->End().index)) {
        state.extractionPos = GetCallPositionOfExtraction(context);
    }
}

struct FunctionExtractionSelectionState {
    TextRange extractionPos;
    ir::AstNode *extractedNode {nullptr};
    bool selectionHasNewline {false};
    bool multilineExpressionSelection {false};
    bool declarationLeadingSelection {false};
};

static bool NormalizeFunctionExtractionSelection(const RefactorContext &context, public_lib::Context *ctx,
                                                 const std::string &actionName, FunctionExtractionSelectionState &state)
{
    const TextRange trimmedSpan = GetTrimmedSelectionSpan(context);
    if (!TryRewriteExtractionToWholeDeclarationInitializer(context, ctx, state.extractionPos, state.extractedNode)) {
        (void)TryRewriteExtractionToTextInitializer(context, ctx, state.extractionPos, state.extractedNode);
    }
    NormalizeWholeDeclarationExtraction(ctx, state.extractionPos, state.extractedNode);
    state.declarationLeadingSelection = IsDeclarationLeadingSelection(context, actionName);
    MultilineSelectionState multilineState {state.extractionPos, state.selectionHasNewline,
                                            state.multilineExpressionSelection};
    NormalizeMultilineFunctionSelection(context, ctx, state.extractedNode, multilineState);
    state.extractionPos = multilineState.extractionPos;
    state.selectionHasNewline = multilineState.selectionHasNewline;
    state.multilineExpressionSelection = multilineState.multilineExpressionSelection;
    if (!state.selectionHasNewline && state.extractedNode != nullptr && state.extractedNode->IsExpression() &&
        trimmedSpan.pos >= state.extractedNode->Start().index && trimmedSpan.end <= state.extractedNode->End().index) {
        state.extractionPos = trimmedSpan;
    }
    MaybeIncludeTrailingSemicolonForReturnSelection(ctx, state.extractedNode, state.extractionPos);
    RefactorEditInfo helperEdits;
    if ((!state.selectionHasNewline || state.multilineExpressionSelection || state.declarationLeadingSelection) &&
        TryBuildHelperExtraction(context, state.extractedNode, actionName, helperEdits)) {
        return false;
    }
    return true;
}

static bool ShouldBuildHelperOnlyFunctionExtraction(bool selectionHasNewline, bool multilineExpressionSelection,
                                                    bool declarationLeadingSelection)
{
    return !selectionHasNewline || multilineExpressionSelection || declarationLeadingSelection;
}

struct HelperOnlyFunctionExtractionInputs {
    const RefactorContext &context;
    ir::AstNode *extractedNode {nullptr};
    const std::string &actionName;
    bool selectionHasNewline {false};
    bool multilineExpressionSelection {false};
    bool declarationLeadingSelection {false};
};

static std::optional<RefactorEditInfo> TryBuildHelperOnlyFunctionExtraction(
    const HelperOnlyFunctionExtractionInputs &inputs)
{
    RefactorEditInfo helperEdits;
    if (ShouldBuildHelperOnlyFunctionExtraction(inputs.selectionHasNewline, inputs.multilineExpressionSelection,
                                                inputs.declarationLeadingSelection) &&
        TryBuildHelperExtraction(inputs.context, inputs.extractedNode, inputs.actionName, helperEdits)) {
        return helperEdits;
    }
    return std::nullopt;
}

struct FunctionExtractionCallData {
    const FunctionIOInfo *ioInfoPtr {nullptr};
    const std::vector<std::string> *capturedParams {nullptr};
    const std::vector<std::string> *capturedArgs {nullptr};
    std::unordered_set<std::string> protectedValueNames;
    FunctionIOInfo statementIo;
    FunctionIOInfo expressionIo;
    bool treatAsStatements {false};
    size_t insertPos {0};
};

static void RebindFunctionExtractionCallDataReferences(FunctionExtractionCallData &data)
{
    data.ioInfoPtr = nullptr;
    data.capturedParams = nullptr;
    data.capturedArgs = nullptr;
    if (data.treatAsStatements) {
        data.ioInfoPtr = &data.statementIo;
        return;
    }
    if (!data.expressionIo.callArgs.empty()) {
        data.capturedParams = &data.expressionIo.paramDecls;
        data.capturedArgs = &data.expressionIo.callArgs;
    }
}

struct FunctionExtractionCallOptions {
    bool includeNonGlobal {false};
    bool preferQualifiedNamespaceRefs {false};
};

struct ShouldTreatExtractionAsStatementsInputs {
    const RefactorContext &context;
    public_lib::Context *ctx {nullptr};
    TextRange extractionPos {};
    bool multilineExpressionSelection {false};
    ir::AstNode *extractedNode {nullptr};
    TextRange originalSelection {};
};

static bool ShouldTreatExtractionAsStatements(const ShouldTreatExtractionAsStatementsInputs &inputs)
{
    auto isSingleLineExpressionFragment = [ctx = inputs.ctx, originalSelection = inputs.originalSelection]() -> bool {
        if (ctx == nullptr || ctx->sourceFile == nullptr || originalSelection.end <= originalSelection.pos ||
            originalSelection.end > ctx->sourceFile->source.size() || HasSourceNewlineInRange(ctx, originalSelection)) {
            return false;
        }
        std::string selected(
            ctx->sourceFile->source.substr(originalSelection.pos, originalSelection.end - originalSelection.pos));
        selected = TrimAsciiWhitespace(selected);
        return !selected.empty() && selected.find(';') == std::string::npos && selected.rfind("let ", 0) != 0 &&
               selected.rfind("const ", 0) != 0;
    };
    auto isInsideExtractedExpression = [extractedNode = inputs.extractedNode,
                                        originalSelection = inputs.originalSelection]() -> bool {
        return extractedNode != nullptr && extractedNode->IsExpression() &&
               originalSelection.pos >= extractedNode->Start().index &&
               originalSelection.end <= extractedNode->End().index;
    };
    auto isInsideAnyInitializerRange = [context = inputs.context, extractedNode = inputs.extractedNode,
                                        originalSelection = inputs.originalSelection]() -> bool {
        VariableBindingInfo binding;
        auto isInBinding = [originalSelection](const VariableBindingInfo &b) {
            return b.initializer != nullptr && originalSelection.pos >= b.initializer->Start().index &&
                   originalSelection.end <= b.initializer->End().index;
        };
        if (ResolveVariableBinding(extractedNode, binding) && isInBinding(binding)) {
            return true;
        }
        auto *touch = GetTouchingToken(context.context, originalSelection.pos, false);
        for (auto *current = touch; current != nullptr; current = current->Parent()) {
            if (ResolveVariableBinding(current, binding) && isInBinding(binding)) {
                return true;
            }
        }
        return false;
    };
    if (isSingleLineExpressionFragment() || isInsideExtractedExpression() || isInsideAnyInitializerRange()) {
        return false;
    }
    return (!inputs.multilineExpressionSelection && HasSourceNewlineInRange(inputs.ctx, inputs.extractionPos)) ||
           (inputs.extractedNode != nullptr && !inputs.extractedNode->IsExpression());
}

static void PopulateProtectedValueNames(std::unordered_set<std::string> &protectedValueNames,
                                        const std::vector<std::string> &callArgs)
{
    for (const auto &arg : callArgs) {
        protectedValueNames.insert(arg);
    }
}

static ir::AstNode *ResolveFunctionInsertAnchorNode(const RefactorContext &context, const std::string &actionName,
                                                    size_t insertPos)
{
    auto *insertTouchNode = GetTouchingToken(context.context, insertPos, false);
    ir::AstNode *insertAnchorNode = insertTouchNode;
    if (actionName == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name)) {
        return nullptr;
    }
    if (const auto namespaceDepth = GetNamespaceActionDepth(actionName, EXTRACT_FUNCTION_ACTION_ENCLOSE.name,
                                                            EXTRACT_FUNCTION_NAMESPACE_ACTION_PREFIX);
        namespaceDepth.has_value()) {
        auto *touchNode = GetTouchingToken(context.context, context.span.pos, false);
        auto *namespaceScope = FindNamespaceScopeByDepth(touchNode, namespaceDepth.value());
        if (namespaceScope != nullptr) {
            insertAnchorNode = namespaceScope;
        }
    }
    return insertAnchorNode;
}

static size_t AdjustInsertPosForMultilineSelection(const RefactorContext &context, public_lib::Context *ctx,
                                                   TextRange extractionPos, bool multilineExpressionSelection,
                                                   const std::string &actionName)
{
    size_t insertPos = NormalizeFunctionInsertPos(context, ctx, actionName);
    if (!multilineExpressionSelection || ctx == nullptr || ctx->sourceFile == nullptr) {
        return insertPos;
    }
    const auto &source = ctx->sourceFile->source;
    size_t probe = std::min(extractionPos.end, source.size());
    while (probe < source.size()) {
        if (source[probe] == ';') {
            return ExtendToLineEnd(util::StringView(source), probe + 1U);
        }
        ++probe;
    }
    return insertPos;
}

static FunctionExtractionCallOptions ResolveFunctionExtractionCallOptions(const std::string &actionName,
                                                                          bool treatAsStatements)
{
    const bool includeNonGlobal =
        actionName == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name) ||
        IsNamespaceAction(actionName, EXTRACT_FUNCTION_ACTION_ENCLOSE.name, EXTRACT_FUNCTION_NAMESPACE_ACTION_PREFIX);
    const bool preferQualifiedNamespaceRefs =
        actionName == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name) && !treatAsStatements;
    return {includeNonGlobal, preferQualifiedNamespaceRefs};
}

static void InitializeStatementFunctionExtractionCallData(const RefactorContext &context, TextRange extractionPos,
                                                          const FunctionExtractionCallOptions &options,
                                                          ir::AstNode *insertAnchorNode,
                                                          FunctionExtractionCallData &data)
{
    data.statementIo = AnalyzeFunctionIO(context, extractionPos, options.includeNonGlobal, insertAnchorNode,
                                         options.preferQualifiedNamespaceRefs);
    data.ioInfoPtr = &data.statementIo;
    PopulateProtectedValueNames(data.protectedValueNames, data.statementIo.callArgs);
}

struct FunctionExtractionCallDataInitInputs {
    const RefactorContext &context;
    TextRange extractionPos {};
    const FunctionExtractionCallOptions &options;
    ir::AstNode *insertAnchorNode {nullptr};
    ir::AstNode *extractedNode {nullptr};
};

static void InitializeExpressionFunctionExtractionCallData(const FunctionExtractionCallDataInitInputs &inputs,
                                                           FunctionExtractionCallData &data)
{
    if (inputs.extractedNode != nullptr &&
        (inputs.extractedNode->IsArrowFunctionExpression() || inputs.extractedNode->IsFunctionExpression())) {
        return;
    }
    data.expressionIo = AnalyzeFunctionIO(inputs.context, inputs.extractionPos, inputs.options.includeNonGlobal,
                                          inputs.insertAnchorNode, inputs.options.preferQualifiedNamespaceRefs);
    if (!data.expressionIo.callArgs.empty()) {
        data.capturedParams = &data.expressionIo.paramDecls;
        data.capturedArgs = &data.expressionIo.callArgs;
        PopulateProtectedValueNames(data.protectedValueNames, data.expressionIo.callArgs);
    }
}

struct FunctionExtractionCallDataInputs {
    const RefactorContext &context;
    public_lib::Context *ctx {nullptr};
    const std::string &actionName;
    TextRange extractionPos {};
    bool multilineExpressionSelection {false};
    ir::AstNode *extractedNode {nullptr};
};

static FunctionExtractionCallData BuildFunctionExtractionCallData(const FunctionExtractionCallDataInputs &inputs)
{
    FunctionExtractionCallData data;
    data.treatAsStatements = ShouldTreatExtractionAsStatements(
        {inputs.context, inputs.ctx, inputs.extractionPos, inputs.multilineExpressionSelection, inputs.extractedNode,
         GetTrimmedSelectionSpan(inputs.context)});
    const FunctionExtractionCallOptions options =
        ResolveFunctionExtractionCallOptions(inputs.actionName, data.treatAsStatements);
    data.insertPos = AdjustInsertPosForMultilineSelection(inputs.context, inputs.ctx, inputs.extractionPos,
                                                          inputs.multilineExpressionSelection, inputs.actionName);
    auto *insertAnchorNode = ResolveFunctionInsertAnchorNode(inputs.context, inputs.actionName, data.insertPos);
    const FunctionExtractionCallDataInitInputs initInputs {inputs.context, inputs.extractionPos, options,
                                                           insertAnchorNode, inputs.extractedNode};
    if (data.treatAsStatements) {
        InitializeStatementFunctionExtractionCallData(initInputs.context, initInputs.extractionPos, initInputs.options,
                                                      initInputs.insertAnchorNode, data);
        RebindFunctionExtractionCallDataReferences(data);
        return data;
    }
    InitializeExpressionFunctionExtractionCallData(initInputs, data);
    RebindFunctionExtractionCallDataReferences(data);
    return data;
}

static bool ShouldReturnExtractedFunctionCallResult(bool treatAsStatements, ir::AstNode *extractedNode,
                                                    TextRange extractionPos, const RefactorContext &context)
{
    const bool extractedIsReturnStatement = extractedNode != nullptr && extractedNode->IsReturnStatement();
    const bool extractedIsWholeExpressionStatementExpr =
        extractedNode != nullptr && extractedNode->Parent() != nullptr &&
        extractedNode->Parent()->IsExpressionStatement() && extractedNode->Start().index <= extractionPos.pos &&
        extractedNode->End().index >= extractionPos.end;
    if (treatAsStatements) {
        return false;
    }
    if (extractedIsReturnStatement || extractedIsWholeExpressionStatementExpr) {
        return false;
    }
    return ShouldReturnExtractedExpressionResult(context, extractedNode);
}

struct MultilineExpressionRewriteInputs {
    const RefactorContext &context;
    public_lib::Context *ctx {nullptr};
    TextRange extractionPos {};
    bool multilineExpressionSelection {false};
    bool treatAsStatements {false};
    const std::string &returnTypeAnnotation;
    std::string functionText;
};

static std::string MaybeRewriteMultilineExpressionFunctionText(MultilineExpressionRewriteInputs inputs)
{
    if (!inputs.multilineExpressionSelection || inputs.treatAsStatements || inputs.ctx == nullptr ||
        inputs.ctx->sourceFile == nullptr) {
        return inputs.functionText;
    }
    const auto &source = inputs.ctx->sourceFile->source;
    std::string exprText(source.substr(inputs.extractionPos.pos, inputs.extractionPos.end - inputs.extractionPos.pos));
    std::string trimmedExpr = TrimAsciiWhitespace(exprText);
    if (trimmedExpr.empty()) {
        return inputs.functionText;
    }
    const std::string newLine =
        inputs.context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();
    std::string bodyExpr = std::move(exprText);
    TrimTrailingNewlines(bodyExpr);
    if (bodyExpr.empty()) {
        return inputs.functionText;
    }

    size_t firstLineIndent = 0;
    while (firstLineIndent < bodyExpr.size() && IsIndentChar(bodyExpr[firstLineIndent])) {
        ++firstLineIndent;
    }
    const std::string indentStep(ResolveIndentSize(inputs.context), SPACE_CHAR);
    std::string normalizedBody = "return " + bodyExpr.substr(firstLineIndent);
    size_t scanPos = 0;
    while ((scanPos = normalizedBody.find('\n', scanPos)) != std::string::npos) {
        ++scanPos;
        size_t lineIndent = 0;
        while (scanPos + lineIndent < normalizedBody.size() && IsIndentChar(normalizedBody[scanPos + lineIndent])) {
            ++lineIndent;
        }
        if (lineIndent >= firstLineIndent) {
            normalizedBody.erase(scanPos, firstLineIndent);
        }
    }
    std::string suffixTrimmed = TrimAsciiWhitespace(normalizedBody);
    if (!suffixTrimmed.empty() && suffixTrimmed.back() != ';') {
        normalizedBody.append(";");
    }

    return newLine + "function newFunction()" + inputs.returnTypeAnnotation + " {" + newLine + indentStep +
           normalizedBody + newLine + "}" + newLine + newLine;
}

struct FinalizeFunctionCallInputs {
    public_lib::Context *ctx {nullptr};
    TextRange extractionPos {};
    const std::string &functionText;
    const FunctionExtraction &candidate;
    const FunctionExtractionCallData &callData;
    ir::AstNode *extractedNode {nullptr};
};

static std::string FinalizeFunctionCallText(const FinalizeFunctionCallInputs &inputs)
{
    std::vector<std::string> callArgs = BuildFunctionCallArgs(
        inputs.candidate, inputs.callData.treatAsStatements, inputs.callData.statementIo, inputs.callData.capturedArgs);
    const bool extractedIsReturnStatement =
        inputs.extractedNode != nullptr && inputs.extractedNode->IsReturnStatement();
    const bool shouldReturnCallResult =
        extractedIsReturnStatement ||
        (inputs.callData.treatAsStatements && inputs.callData.statementIo.hasReturnStatement &&
         !inputs.callData.statementIo.returnVar.has_value());
    const bool extractedFunctionIsAsync = inputs.ctx != nullptr && inputs.ctx->parserProgram != nullptr &&
                                          HasAwaitInRange(inputs.ctx->parserProgram->Ast(), inputs.extractionPos);
    std::string funcCallText = ReplaceWithFunctionCall(
        {inputs.functionText, callArgs,
         inputs.callData.treatAsStatements ? inputs.callData.statementIo.returnVar : std::nullopt,
         inputs.callData.treatAsStatements, shouldReturnCallResult,
         shouldReturnCallResult && extractedFunctionIsAsync});
    if (!inputs.callData.treatAsStatements && !funcCallText.empty() && funcCallText.back() != ';' &&
        inputs.ctx != nullptr && inputs.ctx->sourceFile != nullptr &&
        inputs.extractionPos.end <= inputs.ctx->sourceFile->source.size() &&
        inputs.extractionPos.end > inputs.extractionPos.pos &&
        RangeEndsWithStatementSemicolon(inputs.ctx->sourceFile->source, inputs.extractionPos)) {
        funcCallText.push_back(';');
    }
    return funcCallText;
}

struct PreparedFunctionExtractionState {
    TextRange extractionPos {};
    ir::AstNode *extractedNode {nullptr};
    FunctionExtractionSelectionState selectionState {};
    FunctionExtraction candidate;
    FunctionExtractionCallData callData;
    std::optional<RefactorEditInfo> helperEdits;
};

static PreparedFunctionExtractionState BuildPreparedFunctionExtractionState(
    TextRange extractionPos, ir::AstNode *extractedNode, const FunctionExtractionSelectionState &selectionState)
{
    PreparedFunctionExtractionState prepared;
    prepared.extractionPos = extractionPos;
    prepared.extractedNode = extractedNode;
    prepared.selectionState = selectionState;
    return prepared;
}

static std::optional<PreparedFunctionExtractionState> TryPrepareHelperOnlyFunctionExtractionState(
    const RefactorContext &context, ir::AstNode *extractedNode, const std::string &actionName,
    const FunctionExtractionSelectionState &selectionState)
{
    const HelperOnlyFunctionExtractionInputs helperInputs = {
        context,
        extractedNode,
        actionName,
        selectionState.selectionHasNewline,
        selectionState.multilineExpressionSelection,
        selectionState.declarationLeadingSelection,
    };
    auto helperEdits = TryBuildHelperOnlyFunctionExtraction(helperInputs);
    if (!helperEdits.has_value()) {
        return std::nullopt;
    }
    auto prepared = BuildPreparedFunctionExtractionState(selectionState.extractionPos, selectionState.extractedNode,
                                                         selectionState);
    prepared.helperEdits = std::move(helperEdits);
    return prepared;
}

static std::optional<PreparedFunctionExtractionState> PrepareFunctionExtractionState(const RefactorContext &context,
                                                                                     public_lib::Context *ctx,
                                                                                     const std::string &actionName)
{
    auto *extractedNode = FindExtractedFunction(context);
    if (extractedNode == nullptr) {
        return std::nullopt;
    }
    TextRange extractionPos = GetTrimmedSelectionSpan(context);
    FunctionExtractionSelectionState selectionState {extractionPos, extractedNode};
    if (!NormalizeFunctionExtractionSelection(context, ctx, actionName, selectionState)) {
        return TryPrepareHelperOnlyFunctionExtractionState(context, selectionState.extractedNode, actionName,
                                                           selectionState);
    }
    extractionPos = selectionState.extractionPos;
    extractedNode = selectionState.extractedNode;
    if (extractedNode == nullptr) {
        return std::nullopt;
    }
    if (auto prepared = TryPrepareHelperOnlyFunctionExtractionState(context, extractedNode, actionName, selectionState);
        prepared.has_value()) {
        return prepared;
    }
    FunctionExtraction candidate;
    if (!TryGetFunctionExtractionCandidate(context, candidate)) {
        return std::nullopt;
    }
    FunctionExtractionCallData callData = BuildFunctionExtractionCallData(
        {context, ctx, actionName, extractionPos, selectionState.multilineExpressionSelection, extractedNode});
    RebindFunctionExtractionCallDataReferences(callData);
    return PreparedFunctionExtractionState {extractionPos,        extractedNode,       selectionState,
                                            std::move(candidate), std::move(callData), std::nullopt};
}

struct ResolveFunctionExtractionReturnTypeInputs {
    public_lib::Context *ctx {nullptr};
    const RefactorContext &context;
    ir::AstNode *extractedNode {nullptr};
    bool extractedIsReturnStatement {false};
    bool returnExtractedExpressionResult {false};
    bool extractedFunctionReturnsValue {false};
    TextRange extractionPos {};
};

static bool IsFunctionLikeExtractedNode(const ir::AstNode *node)
{
    return node != nullptr && (node->IsArrowFunctionExpression() || node->IsFunctionExpression());
}

static ir::AstNode *ResolveReturnTypeProbeNode(ir::AstNode *node)
{
    if (node != nullptr && node->IsExpressionStatement()) {
        auto *expr = node->AsExpressionStatement()->GetExpression();
        if (expr != nullptr) {
            return expr;
        }
    }
    return node;
}

static std::string ResolveReturnTypeFromExtractedExpressionResult(
    const ResolveFunctionExtractionReturnTypeInputs &inputs)
{
    if (!inputs.returnExtractedExpressionResult || inputs.extractedNode == nullptr) {
        return "";
    }
    ir::AstNode *probe = ResolveReturnTypeProbeNode(inputs.extractedNode);
    if (std::string fromTypeAnnotation = InferFromConsumerTypeAnnotation(inputs.context, inputs.ctx, probe);
        !fromTypeAnnotation.empty()) {
        return fromTypeAnnotation;
    }
    if (inputs.ctx != nullptr && inputs.ctx->GetChecker() != nullptr) {
        return InferTypeFromChecker(inputs.ctx->GetChecker()->AsETSChecker(), probe);
    }
    return "";
}

static std::string ResolveReturnTypeFromReturnStatementArgument(const ResolveFunctionExtractionReturnTypeInputs &inputs)
{
    if (!inputs.extractedIsReturnStatement || inputs.extractedNode == nullptr) {
        return "";
    }
    auto *retStmt = inputs.extractedNode->AsReturnStatement();
    auto *argument = retStmt == nullptr ? nullptr : retStmt->Argument();
    if (argument == nullptr) {
        return "";
    }
    if (argument->IsNumberLiteral()) {
        return ": Int";
    }
    if (argument->IsStringLiteral()) {
        return ": string";
    }
    if (argument->IsBooleanLiteral()) {
        return ": boolean";
    }
    return "";
}

static std::string ResolveReturnTypeFallbacks(const ResolveFunctionExtractionReturnTypeInputs &inputs)
{
    if (inputs.returnExtractedExpressionResult) {
        if (std::string typeAnnotation =
                ResolveVariableTypeAnnotation(inputs.ctx, inputs.context, inputs.extractedNode);
            !typeAnnotation.empty()) {
            return typeAnnotation;
        }
    }
    if (inputs.extractedFunctionReturnsValue) {
        return InferReturnTypeAnnotationFromSelectionFallback(inputs.context, inputs.ctx, inputs.extractionPos);
    }
    return "";
}

static std::string ResolveFunctionExtractionReturnType(const ResolveFunctionExtractionReturnTypeInputs &inputs)
{
    if (IsFunctionLikeExtractedNode(inputs.extractedNode)) {
        return "";
    }
    std::string returnTypeAnnotation;
    if (inputs.extractedFunctionReturnsValue) {
        returnTypeAnnotation = InferExtractedReturnTypeAnnotationImpl(inputs.context, inputs.extractedNode);
    }
    if (returnTypeAnnotation.empty()) {
        returnTypeAnnotation = ResolveReturnTypeFromExtractedExpressionResult(inputs);
    }
    returnTypeAnnotation = NormalizeReturnTypeAnnotation(returnTypeAnnotation);
    if (returnTypeAnnotation.empty()) {
        returnTypeAnnotation = ResolveReturnTypeFromReturnStatementArgument(inputs);
    }
    if (returnTypeAnnotation.empty()) {
        returnTypeAnnotation = ResolveReturnTypeFallbacks(inputs);
    }
    return returnTypeAnnotation;
}

RefactorEditInfo GetRefactorEditsToExtractFunction(const RefactorContext &context, const std::string &actionName)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (actionName == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name)) {
        TextRange extractionPos = GetTrimmedSelectionSpan(context);
        if (auto earlyEdits = TryBuildEarlyGlobalFunctionExtraction(context, ctx, extractionPos);
            earlyEdits.has_value()) {
            return earlyEdits.value();
        }
    }
    auto prepared = PrepareFunctionExtractionState(context, ctx, actionName);
    if (!prepared.has_value()) {
        return RefactorEditInfo();
    }
    if (prepared->helperEdits.has_value()) {
        return prepared->helperEdits.value();
    }
    const auto &selectionState = prepared->selectionState;
    const TextRange extractionPos = prepared->extractionPos;
    auto *extractedNode = prepared->extractedNode;
    auto &candidate = prepared->candidate;
    auto &callData = prepared->callData;
    RebindFunctionExtractionCallDataReferences(callData);
    const bool extractedIsReturnStatement = extractedNode->IsReturnStatement();
    const bool returnExtractedExpressionResult =
        ShouldReturnExtractedFunctionCallResult(callData.treatAsStatements, extractedNode, extractionPos, context);
    const bool extractedFunctionReturnsValue = extractedIsReturnStatement || returnExtractedExpressionResult;
    std::string returnTypeAnnotation = ResolveFunctionExtractionReturnType(
        {ctx, context, extractedNode, extractedIsReturnStatement, returnExtractedExpressionResult,
         extractedFunctionReturnsValue, extractionPos});
    const std::unordered_set<std::string> *protectedValueNamesPtr =
        callData.protectedValueNames.empty() ? nullptr : &callData.protectedValueNames;
    FunctionTextBuildOptions buildOptions {callData.ioInfoPtr, callData.capturedParams, returnExtractedExpressionResult,
                                           returnTypeAnnotation, protectedValueNamesPtr};
    std::string functionText = BuildFunctionText(candidate, context, actionName, extractionPos, buildOptions);
    functionText = MaybeRewriteMultilineExpressionFunctionText(
        {context, ctx, extractionPos, selectionState.multilineExpressionSelection, callData.treatAsStatements,
         returnTypeAnnotation, std::move(functionText)});
    std::string funcCallText =
        FinalizeFunctionCallText({ctx, extractionPos, functionText, candidate, callData, extractedNode});
    std::vector<FileTextChanges> edits = BuildFunctionExtractionTextChanges(
        {context, actionName, functionText, callData.insertPos, extractionPos, funcCallText});
    size_t renameLoc = ComputeFunctionRenameLoc(edits, extractionPos);
    const auto src = reinterpret_cast<public_lib::Context *>(context.context)->sourceFile;
    return RefactorEditInfo(std::move(edits), std::optional<std::string>(src->filePath),
                            std::optional<size_t>(renameLoc));
}
static bool ShouldReturnExtractedExpressionResult(const RefactorContext &context, ir::AstNode *extractedNode)
{
    if (extractedNode == nullptr) {
        return true;
    }
    if (extractedNode->IsExpressionStatement()) {
        auto *expr = extractedNode->AsExpressionStatement()->GetExpression();
        if (expr == nullptr) {
            return false;
        }
        const TextRange trimmed = GetTrimmedSelectionSpan(context);
        if (trimmed.pos == expr->Start().index && trimmed.end == expr->End().index) {
            return false;
        }
        return true;
    }
    if (!extractedNode->IsExpression()) {
        return true;
    }
    auto isWholeExpressionStatementSelection = [context](ir::AstNode *exprStmt) {
        if (exprStmt == nullptr || !exprStmt->IsExpressionStatement()) {
            return false;
        }
        auto *expr = exprStmt->AsExpressionStatement()->GetExpression();
        if (expr == nullptr) {
            return false;
        }
        const TextRange trimmed = GetTrimmedSelectionSpan(context);
        return trimmed.pos == expr->Start().index && trimmed.end == expr->End().index;
    };
    // If the selected expression is a standalone call statement, the extracted function body
    // should keep it as a statement instead of forcing a return value.
    for (auto *current = extractedNode; current != nullptr; current = current->Parent()) {
        if (!current->IsExpressionStatement()) {
            continue;
        }
        if (isWholeExpressionStatementSelection(current)) {
            return false;
        }
        break;
    }
    return true;
}

static std::string InferReturnTypeAnnotationFromStatementsWithChecker(const RefactorContext &context, TextRange range)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return "";
    }
    auto *checker = ctx->GetChecker() == nullptr ? nullptr : ctx->GetChecker()->AsETSChecker();
    if (checker == nullptr) {
        return "";
    }

    std::string inferred;
    ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *node) {
        if (node == nullptr || !node->IsReturnStatement() || !IsContainedInRange(node, range)) {
            return false;
        }
        auto *retStmt = node->AsReturnStatement();
        if (retStmt == nullptr || retStmt->Argument() == nullptr) {
            inferred = ": void";
            return true;
        }
        auto type = GetTypeOfSymbolAtLocation(checker, retStmt->Argument());
        if (type == nullptr) {
            return false;
        }
        std::string typeText = type->ToString();
        if (typeText.empty()) {
            return false;
        }
        inferred = ": " + typeText;
        return true;
    });
    return inferred;
}

static bool RewriteAssignedReturnVarToDirectReturn(std::string &bodyText, const std::optional<std::string> &returnVar)
{
    if (!returnVar.has_value() || returnVar->empty()) {
        return false;
    }
    const std::string prefix = returnVar.value() + " =";
    size_t start = 0;
    while (start < bodyText.size() && IsIndentChar(bodyText[start])) {
        ++start;
    }
    if (start + prefix.size() > bodyText.size() || bodyText.compare(start, prefix.size(), prefix) != 0) {
        return false;
    }
    size_t rhsStart = start + prefix.size();
    while (rhsStart < bodyText.size() && IsIndentChar(bodyText[rhsStart])) {
        ++rhsStart;
    }
    if (rhsStart >= bodyText.size()) {
        return false;
    }
    bodyText = "return " + bodyText.substr(rhsStart);
    size_t end = bodyText.size();
    while (end > 0 && std::isspace(static_cast<unsigned char>(bodyText[end - 1])) != 0) {
        --end;
    }
    if (end > 0 && bodyText[end - 1] == '}') {
        bodyText.insert(end, ";");
    } else {
        EnsureTrailingSemicolon(bodyText);
    }
    return true;
}

struct ExtractedFunctionCodeParts {
    const std::string &bodyText;
    const std::string &params;
    const std::string &returnTypeAnnotation;
    const RefactorContext &context;
    std::string_view actionName;
    const FunctionBodyOptions &baseBodyOptions;
    bool isAsyncFunction {false};
};

std::string GenerateExtractedFunctionCode(const ExtractedFunctionCodeParts &parts)
{
    const bool isClassAction = parts.actionName == EXTRACT_FUNCTION_ACTION_CLASS.name;
    std::string baseName = isClassAction ? "newMethod" : "newFunction";
    std::string functionName = GenerateUniqueFuncName(parts.context, baseName, std::string(parts.actionName));
    const std::string newLine =
        parts.context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();

    std::ostringstream oss;
    if (parts.baseBodyOptions.addLeadingNewLine) {
        oss << newLine;
    }
    FunctionBodyOptions bodyOptions = parts.baseBodyOptions;
    bodyOptions.newLine = newLine;
    if (isClassAction) {
        oss << bodyOptions.indent << "private " << (parts.isAsyncFunction ? "async " : "") << functionName << "("
            << parts.params << ")" << parts.returnTypeAnnotation << " {" << newLine
            << BuildFunctionBody(parts.bodyText, bodyOptions) << bodyOptions.indent << "}" << newLine;
    } else {
        oss << bodyOptions.indent << (parts.isAsyncFunction ? "async " : "") << "function " << functionName << "("
            << parts.params << ")" << parts.returnTypeAnnotation << " {" << newLine
            << BuildFunctionBody(parts.bodyText, bodyOptions) << bodyOptions.indent << "}" << newLine << newLine;
    }
    return oss.str();
}

static bool TryResolveFunctionExtractionRange(public_lib::Context *ctx, TextRange extractionRange, size_t &start,
                                              size_t &end)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr || ctx->sourceFile->source.empty()) {
        return false;
    }
    start = extractionRange.pos;
    end = extractionRange.end;
    const auto &source = ctx->sourceFile->source;
    return start < source.size() && end <= source.size() && start < end;
}

static bool HasAwaitInRange(ir::AstNode *ast, TextRange range)
{
    if (ast == nullptr || range.end <= range.pos) {
        return false;
    }
    bool hasAwait = false;
    ast->FindChild([&](ir::AstNode *node) {
        if (node == nullptr || !node->IsAwaitExpression()) {
            return false;
        }
        if (node->Start().index >= range.pos && node->End().index <= range.end) {
            hasAwait = true;
            return true;
        }
        return false;
    });
    return hasAwait;
}

static std::string EnsureAsyncReturnTypeAnnotation(const std::string &returnTypeAnnotation)
{
    if (returnTypeAnnotation.empty()) {
        return ": Promise<void>";
    }
    constexpr std::string_view prefix = ": ";
    if (returnTypeAnnotation.rfind(prefix, 0) != 0) {
        return returnTypeAnnotation;
    }
    std::string innerType = returnTypeAnnotation.substr(prefix.size());
    if (innerType.rfind("Promise<", 0) == 0) {
        return returnTypeAnnotation;
    }
    return ": Promise<" + innerType + ">";
}

static std::string TryExtractTypeParamsFromNodeText(public_lib::Context *ctx, ir::AstNode *candidate)
{
    if (ctx == nullptr || candidate == nullptr) {
        return "";
    }
    const std::string text = GetNodeText(ctx, candidate);
    if (text.empty()) {
        return "";
    }
    const std::string_view view(text);
    const size_t parenPos = view.find('(');
    if (parenPos == std::string_view::npos || parenPos == 0) {
        return "";
    }
    size_t gtPos = parenPos;
    while (gtPos > 0 && IsIndentChar(view[gtPos - 1])) {
        --gtPos;
    }
    if (gtPos == 0 || view[gtPos - 1] != '>') {
        return "";
    }
    size_t endPos = gtPos - 1;
    size_t depth = 1;
    while (endPos > 0) {
        --endPos;
        if (view[endPos] == '>') {
            ++depth;
            continue;
        }
        if (view[endPos] != '<') {
            continue;
        }
        --depth;
        if (depth == 0) {
            return std::string(view.substr(endPos, gtPos - endPos));
        }
    }
    return "";
}

static bool CanCarryEnclosingFunctionTypeParams(const ir::AstNode *node)
{
    return node != nullptr &&
           (node->IsMethodDefinition() || node->IsFunctionDeclaration() || node->IsFunctionExpression() ||
            node->IsArrowFunctionExpression() || node->IsScriptFunction());
}

static std::string ExtractEnclosingFunctionTypeParams(public_lib::Context *ctx, ir::AstNode *node)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return "";
    }
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (CanCarryEnclosingFunctionTypeParams(current)) {
            if (std::string extracted = TryExtractTypeParamsFromNodeText(ctx, current); !extracted.empty()) {
                return extracted;
            }
        }
        auto *parent = current->Parent();
        if (std::string extracted = TryExtractTypeParamsFromNodeText(ctx, parent); !extracted.empty()) {
            return extracted;
        }
    }
    return "";
}

static std::vector<std::string> SplitTypeParamEntries(std::string_view listText)
{
    std::vector<std::string> entries;
    size_t entryStart = 0;
    size_t depth = 0;
    for (size_t i = 0; i < listText.size(); ++i) {
        const char ch = listText[i];
        if (ch == '<') {
            ++depth;
            continue;
        }
        if (ch == '>') {
            if (depth > 0) {
                --depth;
            }
            continue;
        }
        if (ch != ',' || depth != 0) {
            continue;
        }
        std::string entry(listText.substr(entryStart, i - entryStart));
        size_t start = 0;
        while (start < entry.size() && std::isspace(static_cast<unsigned char>(entry[start])) != 0) {
            ++start;
        }
        size_t end = entry.size();
        while (end > start && std::isspace(static_cast<unsigned char>(entry[end - 1])) != 0) {
            --end;
        }
        if (start < end) {
            entries.emplace_back(entry.substr(start, end - start));
        }
        entryStart = i + 1;
    }
    std::string tail(listText.substr(entryStart));
    size_t start = 0;
    while (start < tail.size() && std::isspace(static_cast<unsigned char>(tail[start])) != 0) {
        ++start;
    }
    size_t end = tail.size();
    while (end > start && std::isspace(static_cast<unsigned char>(tail[end - 1])) != 0) {
        --end;
    }
    if (start < end) {
        entries.emplace_back(tail.substr(start, end - start));
    }
    return entries;
}

static std::string ExtractTypeParamName(std::string_view entry)
{
    size_t start = 0;
    while (start < entry.size() && std::isspace(static_cast<unsigned char>(entry[start])) != 0) {
        ++start;
    }
    size_t end = start;
    while (end < entry.size() && IsIdentifierContinuation(entry[end])) {
        ++end;
    }
    if (start == end) {
        return "";
    }
    return std::string(entry.substr(start, end - start));
}

static ir::AstNode *FindEnclosingClassDeclarationNode(ir::AstNode *node)
{
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (current->IsClassDeclaration()) {
            return current;
        }
    }
    return nullptr;
}

static std::optional<std::pair<size_t, size_t>> FindClassTypeParamBounds(std::string_view view)
{
    const size_t classPos = view.find("class ");
    if (classPos == std::string_view::npos) {
        return std::nullopt;
    }
    const size_t bodyPos = view.find('{', classPos);
    if (bodyPos == std::string_view::npos) {
        return std::nullopt;
    }
    const size_t ltPos = view.find('<', classPos);
    if (ltPos == std::string_view::npos || ltPos >= bodyPos) {
        return std::nullopt;
    }
    return std::make_pair(ltPos, bodyPos);
}

static std::string ExtractBalancedTypeParams(std::string_view view, size_t ltPos, size_t bodyPos)
{
    size_t depth = 0;
    for (size_t i = ltPos; i < bodyPos; ++i) {
        if (view[i] == '<') {
            ++depth;
            continue;
        }
        if (view[i] != '>') {
            continue;
        }
        if (depth == 0) {
            return "";
        }
        --depth;
        if (depth == 0) {
            return std::string(view.substr(ltPos, i - ltPos + 1));
        }
    }
    return "";
}

static std::string ExtractEnclosingClassTypeParams(public_lib::Context *ctx, ir::AstNode *node)
{
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return "";
    }
    ir::AstNode *classDeclNode = FindEnclosingClassDeclarationNode(node);
    if (classDeclNode == nullptr) {
        return "";
    }
    const std::string text = GetNodeText(ctx, classDeclNode);
    if (text.empty()) {
        return "";
    }
    const std::string_view view(text);
    const auto bounds = FindClassTypeParamBounds(view);
    if (!bounds.has_value()) {
        return "";
    }
    return ExtractBalancedTypeParams(view, bounds->first, bounds->second);
}

static std::string FilterTypeParamsByUsage(const std::string &typeParams, std::string_view usageText)
{
    if (typeParams.size() < K_TYPE_PARAM_DELIMITER_PAIR_LENGTH || typeParams.front() != '<' ||
        typeParams.back() != '>') {
        return typeParams;
    }
    const std::string_view listText(typeParams.data() + 1, typeParams.size() - K_TYPE_PARAM_DELIMITER_PAIR_LENGTH);
    const auto entries = SplitTypeParamEntries(listText);
    if (entries.empty()) {
        return "";
    }
    std::vector<std::string> usedEntries;
    usedEntries.reserve(entries.size());
    for (const auto &entry : entries) {
        const std::string paramName = ExtractTypeParamName(entry);
        if (paramName.empty() || ContainsIdentifierToken(usageText, paramName)) {
            usedEntries.push_back(entry);
        }
    }
    if (usedEntries.empty()) {
        return "";
    }
    return "<" + JoinWithComma(usedEntries) + ">";
}

static std::string MergeTypeParamLists(const std::string &lhs, const std::string &rhs)
{
    std::vector<std::string> merged;
    auto appendEntries = [&merged](const std::string &text) {
        if (text.size() < K_TYPE_PARAM_DELIMITER_PAIR_LENGTH || text.front() != '<' || text.back() != '>') {
            return;
        }
        const std::string_view listText(text.data() + 1, text.size() - K_TYPE_PARAM_DELIMITER_PAIR_LENGTH);
        auto entries = SplitTypeParamEntries(listText);
        merged.insert(merged.end(), entries.begin(), entries.end());
    };
    appendEntries(lhs);
    appendEntries(rhs);
    if (merged.empty()) {
        return "";
    }
    return "<" + JoinWithComma(merged) + ">";
}

static void AdjustIndentForClassFunctionExtraction(std::string_view src, const std::string &actionName,
                                                   ir::AstNode *scopeNode, std::string &indent, size_t &indentStep)
{
    if (actionName != std::string(EXTRACT_FUNCTION_ACTION_CLASS.name)) {
        return;
    }
    auto *classDef = FindEnclosingClassDefinition(scopeNode);
    if (classDef == nullptr) {
        return;
    }
    auto [classLineStart, classIndentEnd] = ComputeLineIndent(util::StringView(src), classDef->Start().index);
    auto [exprLineStart, exprIndentEnd] = ComputeLineIndent(util::StringView(src), scopeNode->Start().index);
    if (classIndentEnd < classLineStart || exprIndentEnd < exprLineStart) {
        return;
    }
    const size_t classIndent = classIndentEnd - classLineStart;
    const size_t exprIndent = exprIndentEnd - exprLineStart;
    size_t methodIndent = 0;
    bool hasMethodAncestor = false;
    for (auto *current = scopeNode; current != nullptr; current = current->Parent()) {
        if (!current->IsMethodDefinition()) {
            continue;
        }
        auto [methodLineStart, methodIndentEnd] = ComputeLineIndent(util::StringView(src), current->Start().index);
        if (methodIndentEnd >= methodLineStart) {
            methodIndent = methodIndentEnd - methodLineStart;
            hasMethodAncestor = true;
        }
        break;
    }
    if (hasMethodAncestor && exprIndent > methodIndent) {
        indentStep = exprIndent - methodIndent;
        indent.assign(methodIndent, SPACE_CHAR);
        return;
    }
    if (exprIndent > classIndent) {
        indentStep = exprIndent - classIndent;
    }
    indent.assign(exprIndent, SPACE_CHAR);
}

struct NamespaceFunctionIndentInputs {
    std::string_view src;
    const std::string &actionName;
    ir::AstNode *scopeNode {nullptr};
    size_t insertPos {0};
    std::string &indent;
    size_t indentStep {0};
};

static void AdjustIndentForNamespaceFunctionExtraction(const NamespaceFunctionIndentInputs &inputs)
{
    const bool isNamespaceAction = IsNamespaceAction(inputs.actionName, EXTRACT_FUNCTION_ACTION_ENCLOSE.name,
                                                     EXTRACT_FUNCTION_NAMESPACE_ACTION_PREFIX) ||
                                   (inputs.actionName == std::string(EXTRACT_FUNCTION_ACTION_ENCLOSE.name) &&
                                    IsNamespaceContext(inputs.scopeNode));
    if (!isNamespaceAction || inputs.insertPos >= inputs.src.size()) {
        return;
    }
    auto [lineStart, lineIndentEnd] = ComputeLineIndent(util::StringView(inputs.src), inputs.insertPos);
    size_t firstNonIndent = lineIndentEnd;
    while (firstNonIndent < inputs.src.size() && !IsLineBreakChar(inputs.src[firstNonIndent]) &&
           IsIndentChar(inputs.src[firstNonIndent])) {
        ++firstNonIndent;
    }
    if (lineStart < inputs.src.size() && firstNonIndent < inputs.src.size() && inputs.src[firstNonIndent] == '}') {
        inputs.indent.append(inputs.indentStep, SPACE_CHAR);
    }
}

static size_t ResolveFunctionTrimIndent(std::string_view source, size_t start, bool treatAsStatements)
{
    if (!treatAsStatements) {
        return 0;
    }
    auto [lineStart, indentEnd] = ComputeLineIndent(util::StringView(source), start);
    return indentEnd >= lineStart ? indentEnd - lineStart : 0;
}

static std::optional<std::string> ResolveCapturedFunctionParams(const FunctionIOInfo *ioInfo,
                                                                const std::vector<std::string> *capturedParams)
{
    if (ioInfo != nullptr) {
        return JoinWithComma(ioInfo->paramDecls);
    }
    if (capturedParams != nullptr && !capturedParams->empty()) {
        return JoinWithComma(*capturedParams);
    }
    return std::nullopt;
}

struct ExtractedFunctionResolveInputs {
    const FunctionExtraction &candidate;
    ir::AstNode *ast {nullptr};
    size_t start {0};
    size_t end {0};
    const FunctionIOInfo *ioInfo {nullptr};
    const std::vector<std::string> *capturedParams {nullptr};
};

struct ExtractedFunctionBodyOptions {
    const FunctionIOInfo *ioInfo {nullptr};
    bool treatAsStatements {false};
    bool returnExtractedExpressionResult {false};
    const std::unordered_set<std::string> *protectedValueNames {nullptr};
};

struct FunctionBodyResolveOptions {
    bool treatAsStatements {false};
    const FunctionIOInfo *ioInfo {nullptr};
    bool rewrittenAsDirectReturn {false};
    bool returnExtractedExpressionResult {false};
};

static std::string ResolveExtractedFunctionParams(const ExtractedFunctionResolveInputs &inputs)
{
    if (auto resolved = ResolveCapturedFunctionParams(inputs.ioInfo, inputs.capturedParams); resolved.has_value()) {
        return resolved.value();
    }
    bool needParams = false;
    auto functionParams = CollectFunctionParams(inputs.ast, inputs.start, inputs.end, needParams);
    return needParams ? GetParamsText(inputs.candidate, functionParams) : "";
}

struct ExtractedFunctionBodyTextInputs {
    public_lib::Context *ctx {nullptr};
    const std::string &actionName;
    TextRange extractionRange {};
    size_t start {0};
    size_t end {0};
    const ExtractedFunctionBodyOptions &options;
    bool &rewrittenAsDirectReturn;
};

static std::string ResolveExtractedFunctionBodyText(const ExtractedFunctionBodyTextInputs &inputs)
{
    const auto &src = inputs.ctx->sourceFile->source;
    std::string bodyText(src.begin() + inputs.start, src.begin() + inputs.end);
    bodyText = RemoveMarkerComments(bodyText);
    if (inputs.actionName == std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name)) {
        bodyText = QualifyTypeReferencesForGlobalExtractedBody(inputs.ctx, inputs.extractionRange, std::move(bodyText),
                                                               true, inputs.options.protectedValueNames);
    }
    inputs.rewrittenAsDirectReturn = false;
    if (inputs.options.treatAsStatements && inputs.options.ioInfo != nullptr) {
        inputs.rewrittenAsDirectReturn =
            RewriteAssignedReturnVarToDirectReturn(bodyText, inputs.options.ioInfo->returnVar);
    }
    if (!inputs.options.treatAsStatements && !inputs.options.returnExtractedExpressionResult) {
        EnsureTrailingSemicolon(bodyText);
    }
    return bodyText;
}

struct FunctionBodyOptionInputs {
    const RefactorContext &context;
    public_lib::Context *ctx {nullptr};
    const std::string &actionName;
    size_t start {0};
    size_t insertPos {0};
    const FunctionBodyResolveOptions &options;
};

static FunctionBodyOptions ResolveFunctionBodyOptions(const FunctionBodyOptionInputs &inputs)
{
    const auto &src = inputs.ctx->sourceFile->source;
    std::string indent = GetInsertionIndent(inputs.ctx, inputs.insertPos);
    const bool addLeadingNewLine = !IsLineStartAtPosition(src, inputs.insertPos);
    const size_t trimIndent = ResolveFunctionTrimIndent(src, inputs.start, inputs.options.treatAsStatements);
    size_t indentStep = ResolveIndentSize(inputs.context);
    auto *scopeNode = GetTouchingToken(inputs.context.context, inputs.start, false);
    AdjustIndentForClassFunctionExtraction(src, inputs.actionName, scopeNode, indent, indentStep);
    AdjustIndentForNamespaceFunctionExtraction(
        {src, inputs.actionName, scopeNode, inputs.insertPos, indent, indentStep});
    return FunctionBodyOptions {"",
                                indent,
                                addLeadingNewLine,
                                !inputs.options.treatAsStatements && inputs.options.returnExtractedExpressionResult,
                                inputs.options.ioInfo == nullptr || inputs.options.rewrittenAsDirectReturn
                                    ? std::nullopt
                                    : inputs.options.ioInfo->returnVar,
                                trimIndent,
                                indentStep};
}

static std::string BuildTypeUsageContext(public_lib::Context *ctx, const std::string &params,
                                         const std::string &bodyText, size_t start)
{
    std::string typeUsageContext = params + "\n" + bodyText;
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return typeUsageContext;
    }
    if (auto varType = ExtractVariableDeclaredTypeFromInitializer(ctx->sourceFile->source, start);
        varType.has_value()) {
        typeUsageContext.append("\n").append(varType.value());
    } else if (auto classPropType = ExtractClassPropertyDeclaredTypeFromInitializer(ctx->sourceFile->source, start);
               classPropType.has_value()) {
        typeUsageContext.append("\n").append(classPropType.value());
    }
    return typeUsageContext;
}

static std::string ResolveFunctionTypeParams(public_lib::Context *ctx, ir::AstNode *scopeNode,
                                             const std::string &actionName, std::string_view typeUsageContext)
{
    if (actionName != std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name) &&
        actionName != std::string(EXTRACT_FUNCTION_ACTION_CLASS.name)) {
        return "";
    }
    std::string typeParams = ExtractEnclosingFunctionTypeParams(ctx, scopeNode);
    typeParams = FilterTypeParamsByUsage(typeParams, typeUsageContext);
    if (actionName != std::string(EXTRACT_FUNCTION_ACTION_GLOBAL.name)) {
        return typeParams;
    }
    std::string classTypeParams = ExtractEnclosingClassTypeParams(ctx, scopeNode);
    classTypeParams = FilterTypeParamsByUsage(classTypeParams, typeUsageContext);
    return MergeTypeParamLists(typeParams, classTypeParams);
}

static std::string InsertTypeParamsIntoFunctionText(std::string functionText, const std::string &typeParams)
{
    if (typeParams.empty()) {
        return functionText;
    }
    size_t nameStart = std::string::npos;
    const size_t fnPos = functionText.find("function ");
    if (fnPos != std::string::npos) {
        nameStart = fnPos + std::string("function ").size();
    } else {
        const size_t privatePos = functionText.find("private ");
        if (privatePos == std::string::npos) {
            return functionText;
        }
        nameStart = privatePos + std::string("private ").size();
    }
    while (nameStart < functionText.size() && std::isspace(static_cast<unsigned char>(functionText[nameStart])) != 0) {
        ++nameStart;
    }
    const size_t parenPos = functionText.find('(', nameStart);
    if (parenPos == std::string::npos || parenPos <= nameStart) {
        return functionText;
    }
    functionText.insert(parenPos, typeParams);
    return functionText;
}

static std::string BuildTypeArgsFromTypeParams(const std::string &typeParams)
{
    if (typeParams.size() < K_TYPE_PARAM_DELIMITER_PAIR_LENGTH || typeParams.front() != '<' ||
        typeParams.back() != '>') {
        return "";
    }
    const std::string_view listText(typeParams.data() + 1, typeParams.size() - K_TYPE_PARAM_DELIMITER_PAIR_LENGTH);
    const auto entries = SplitTypeParamEntries(listText);
    std::vector<std::string> args;
    args.reserve(entries.size());
    for (const auto &entry : entries) {
        const std::string name = ExtractTypeParamName(entry);
        if (!name.empty()) {
            args.push_back(name);
        }
    }
    if (args.empty()) {
        return "";
    }
    return "<" + JoinWithComma(args) + ">";
}

static std::string ResolveStatementReturnTypeAnnotation(const RefactorContext &context, TextRange extractionRange,
                                                        const FunctionIOInfo *ioInfo)
{
    if (ioInfo == nullptr) {
        return "";
    }
    if (!ioInfo->returnVarTypeAnnotation.empty()) {
        return ioInfo->returnVarTypeAnnotation;
    }
    if (!ioInfo->hasReturnStatement || ioInfo->returnVar.has_value()) {
        return "";
    }
    return InferReturnTypeAnnotationFromStatementsWithChecker(context, extractionRange);
}

static std::string ResolveExtractedFunctionReturnTypeAnnotation(const RefactorContext &context,
                                                                TextRange extractionRange,
                                                                const FunctionTextBuildOptions &options,
                                                                bool treatAsStatements, bool isAsyncFunction)
{
    std::string resolved = options.returnTypeAnnotation;
    if (resolved.empty() && treatAsStatements) {
        resolved = ResolveStatementReturnTypeAnnotation(context, extractionRange, options.ioInfo);
    }
    if (isAsyncFunction) {
        resolved = EnsureAsyncReturnTypeAnnotation(resolved);
    }
    return resolved;
}

struct ExtractedFunctionSourceParts {
    FunctionBodyOptions bodyOptions;
    bool isAsyncFunction {false};
    std::string bodyText;
    std::string params;
    std::string resolvedReturnTypeAnnotation;
};

static std::string BuildExtractedFunctionSourceText(const RefactorContext &context, const std::string &actionName,
                                                    const ExtractedFunctionSourceParts &parts)
{
    return GenerateExtractedFunctionCode({parts.bodyText, parts.params, parts.resolvedReturnTypeAnnotation, context,
                                          actionName, parts.bodyOptions, parts.isAsyncFunction});
}

std::string BuildFunctionText(const FunctionExtraction &candidate, const RefactorContext &context,
                              const std::string &actionName, TextRange extractionRange,
                              const FunctionTextBuildOptions &options)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    size_t start = 0;
    size_t end = 0;
    if (!TryResolveFunctionExtractionRange(ctx, extractionRange, start, end)) {
        return "";
    }

    const auto ast = ctx->parserProgram->Ast();
    const bool treatAsStatements = options.ioInfo != nullptr;
    std::string params =
        ResolveExtractedFunctionParams({candidate, ast, start, end, options.ioInfo, options.capturedParams});
    bool rewrittenAsDirectReturn = false;
    ExtractedFunctionBodyOptions bodyOptionsInput {
        options.ioInfo, treatAsStatements, options.returnExtractedExpressionResult, options.protectedValueNames};
    std::string bodyText = ResolveExtractedFunctionBodyText(
        {ctx, actionName, extractionRange, start, end, bodyOptionsInput, rewrittenAsDirectReturn});
    size_t insertPos = NormalizeFunctionInsertPos(context, ctx, actionName);
    auto *scopeNode = GetTouchingToken(context.context, start, false);
    FunctionBodyResolveOptions bodyResolveOptions {treatAsStatements, options.ioInfo, rewrittenAsDirectReturn,
                                                   options.returnExtractedExpressionResult};
    FunctionBodyOptions bodyOptions =
        ResolveFunctionBodyOptions({context, ctx, actionName, start, insertPos, bodyResolveOptions});
    std::string typeUsageContext = BuildTypeUsageContext(ctx, params, bodyText, start);
    std::string typeParams = ResolveFunctionTypeParams(ctx, scopeNode, actionName, typeUsageContext);
    const bool isAsyncFunction = HasAwaitInRange(ast, extractionRange);
    std::string resolvedReturnTypeAnnotation = ResolveExtractedFunctionReturnTypeAnnotation(
        context, extractionRange, options, treatAsStatements, isAsyncFunction);
    ExtractedFunctionSourceParts parts {bodyOptions, isAsyncFunction, std::move(bodyText), std::move(params),
                                        std::move(resolvedReturnTypeAnnotation)};
    std::string functionText = BuildExtractedFunctionSourceText(context, actionName, parts);
    return InsertTypeParamsIntoFunctionText(std::move(functionText), typeParams);
}

std::string ReplaceWithFunctionCall(const FunctionCallReplacementInputs &inputs)
{
    std::string functionName = "newFunction";
    std::string callTypeArgs;
    bool callViaThis = false;
    {
        auto parseSignature = [&functionName, &callTypeArgs, &inputs](size_t startPos) {
            size_t nameStart = startPos;
            while (nameStart < inputs.functionText.size() &&
                   std::isspace(static_cast<unsigned char>(inputs.functionText[nameStart])) != 0) {
                ++nameStart;
            }
            size_t nameEnd = nameStart;
            while (nameEnd < inputs.functionText.size() && IsIdentifierContinuation(inputs.functionText[nameEnd])) {
                ++nameEnd;
            }
            if (nameEnd <= nameStart) {
                return;
            }
            functionName = inputs.functionText.substr(nameStart, nameEnd - nameStart);
            size_t parenPos = inputs.functionText.find('(', nameEnd);
            size_t ltPos = inputs.functionText.find('<', nameEnd);
            if (ltPos != std::string::npos && parenPos != std::string::npos && ltPos < parenPos) {
                callTypeArgs = BuildTypeArgsFromTypeParams(inputs.functionText.substr(ltPos, parenPos - ltPos));
            }
        };

        auto pos = inputs.functionText.find("function ");
        if (pos != std::string::npos) {
            parseSignature(pos + strlen("function "));
        } else {
            pos = inputs.functionText.find("private ");
            if (pos != std::string::npos) {
                pos += strlen("private ");
                parseSignature(pos);
                callViaThis = true;
            }
        }
    }
    std::string callArgsText = JoinWithComma(inputs.callArgs);
    std::string callText = (callViaThis ? "this." : "") + functionName + callTypeArgs + "(" + callArgsText + ")";
    if (inputs.returnVar.has_value()) {
        callText = inputs.returnVar.value() + " = " + callText;
    } else if (inputs.returnCallResult) {
        callText = std::string("return ") + (inputs.awaitCallResult ? "await " : "") + callText;
    }
    if (inputs.needsStatement && !callText.empty() && callText.back() != ';') {
        callText.push_back(';');
    }
    return callText;
}
}  // namespace ark::es2panda::lsp
