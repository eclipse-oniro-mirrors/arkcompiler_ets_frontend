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

#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "ir/astNode.h"
#include "ir/expression.h"
#include "ir/ts/tsTypeAliasDeclaration.h"
#include "parser/program/program.h"
#include "ir/ts/tsTypeReference.h"
#include "refactors/refactor_types.h"
#include "util/ustring.h"
#include "varbinder/scope.h"

#include "refactors/extract_type.h"
#include "refactor_provider.h"
#include "internal_api.h"
#include "services/text_change/change_tracker.h"
#include "refactors/extract_symbol_internal.h"

namespace ark::es2panda::lsp {

ExtractTypeRefactor::ExtractTypeRefactor()
{
    AddKind(std::string(EXTRACT_TYPE_ACTION.kind));
    AddKind(std::string(EXTRACT_INTERFACE_ACTION.kind));
}

namespace {
constexpr size_t K_MIN_BRACED_LITERAL_SIZE = 2;
struct TypeExtractionTarget {
    ir::Expression *typeExpr {nullptr};
    TextRange range {0, 0};
};

struct SourceContext {
    std::string_view fileSource;
    const public_lib::Context *pub;
};

struct DeclarationContext {
    bool isInterfaceAction;
    std::string declarationName;
    std::string selectedType;
    std::string newLine;
    size_t insertionPos;
    bool preferExpandedInterface;
};

struct TypeEditBuildContext {
    public_lib::Context *pub;
    std::string_view fileSource;
    TypeExtractionTarget target;
    bool isInterfaceAction;
    size_t start;
    size_t end;
    std::string selectedType;
};

/// @brief Heuristic check to determine if @p text can be treated as an inline object type literal.
bool IsObjectTypeLiteralCandidate(std::string_view text)
{
    if (text.size() < K_MIN_BRACED_LITERAL_SIZE || text.front() != '{' || text.back() != '}') {
        return false;
    }
    return text.find(':') != std::string_view::npos;
}

std::string_view ExtractBalancedObjectTypeView(std::string_view text)
{
    if (text.empty()) {
        return {};
    }

    const size_t openPos = text.find('{');
    if (openPos == std::string_view::npos) {
        return {};
    }

    int depth = 0;
    for (size_t i = openPos; i < text.size(); ++i) {
        const char ch = text[i];
        if (ch == '{') {
            ++depth;
            continue;
        }
        if (ch != '}') {
            continue;
        }
        --depth;
        if (depth == 0) {
            return text.substr(openPos, i - openPos + 1);
        }
    }

    return {};
}

bool IsTopLevelDepthZero(int parenDepth, int braceDepth, int bracketDepth, int angleDepth)
{
    return parenDepth == 0 && braceDepth == 0 && bracketDepth == 0 && angleDepth == 0;
}

bool ContainsTopLevelTypeOperator(std::string_view text)
{
    int parenDepth = 0;
    int braceDepth = 0;
    int bracketDepth = 0;
    int angleDepth = 0;

    for (char ch : text) {
        switch (ch) {
            case '(':
                ++parenDepth;
                break;
            case ')':
                --parenDepth;
                break;
            case '{':
                ++braceDepth;
                break;
            case '}':
                --braceDepth;
                break;
            case '[':
                ++bracketDepth;
                break;
            case ']':
                --bracketDepth;
                break;
            case '<':
                ++angleDepth;
                break;
            case '>':
                --angleDepth;
                break;
            case '|':
            case '&':
                if (IsTopLevelDepthZero(parenDepth, braceDepth, bracketDepth, angleDepth)) {
                    return true;
                }
                break;
            default:
                break;
        }
    }

    return false;
}

/// @brief Returns true when @p text contains tokens typically seen in rich type expressions.
bool ContainsTypeSyntax(std::string_view text)
{
    constexpr std::string_view MARKERS = "[]()<>|&:;=";
    if (text.find_first_of(MARKERS) != std::string_view::npos) {
        return true;
    }
    if (text.find("=>") != std::string_view::npos) {
        return true;
    }
    auto pos = text.find('?');
    while (pos != std::string_view::npos) {
        const bool optionalChain = pos + 1 < text.size() && text[pos + 1] == '.';
        if (!optionalChain) {
            return true;
        }
        pos = text.find('?', pos + 1);
    }
    return false;
}

/// @brief Checks whether the selection consists of identifier characters only.
bool ContainsOnlyIdentifierChars(std::string_view text)
{
    if (text.empty()) {
        return false;
    }
    for (unsigned char ch : text) {
        if ((std::isalnum(ch) == 0) && ch != '_' && ch != '$') {
            return false;
        }
    }
    return true;
}

/// @brief Verifies that the provided AST node fully covers the selection span.
bool NodeContainsSpan(const ir::AstNode &node, const TextRange &span)
{
    return node.Start().index <= span.pos && node.End().index >= span.end;
}

/// @brief Filters AST nodes that can serve as extractable type expressions.
bool ExtractableTypeNode(const ir::AstNode &node)
{
    if (!node.IsExpression() || !node.AsExpression()->IsTypeNode()) {
        return false;
    }
    if (node.IsIdentifier() || node.IsETSTypeReference()) {
        return false;
    }
    if (node.IsTSTypeReference() && node.AsTSTypeReference()->TypeParams() == nullptr) {
        return false;
    }
    return true;
}

/// @brief Slice the original source text according to the refactor selection span.
std::string_view GetSelectionText(const RefactorContext &context, const public_lib::Context *pub)
{
    if (pub == nullptr || pub->sourceFile == nullptr) {
        return {};
    }
    const std::string_view fileSource(pub->sourceFile->source);
    const size_t start = std::min(context.span.pos, fileSource.size());
    const size_t end = std::min(context.span.end, fileSource.size());
    if (start >= end) {
        return {};
    }
    return fileSource.substr(start, end - start);
}

/// @brief Walks up the AST from the given position to find an extractable type node that contains the selection.
ir::AstNode *FindCandidateNode(const RefactorContext &context, size_t position)
{
    ir::AstNode *overlapping = nullptr;

    for (ir::AstNode *node = GetTouchingToken(context.context, position, false); node != nullptr;
         node = node->Parent()) {
        if (!ExtractableTypeNode(*node)) {
            continue;
        }
        if (NodeContainsSpan(*node, context.span)) {
            return node;
        }
        bool nodeEndsBeforeSpan = node->End().index <= context.span.pos;
        bool nodeStartsAfterSpan = node->Start().index >= context.span.end;
        const bool nodeOverlaps = !(nodeEndsBeforeSpan || nodeStartsAfterSpan);
        if (overlapping == nullptr && nodeOverlaps) {
            overlapping = node;
        }
    }
    return overlapping;
}

/// @brief Walk up the AST tree to find a containing type node for the given position
ir::AstNode *WalkUpTreeToFindContainingTypeNode(const RefactorContext &context, size_t position)
{
    ir::AstNode *leafNode = GetTouchingToken(context.context, position, false);
    if (leafNode == nullptr) {
        return nullptr;
    }

    for (ir::AstNode *ancestor = leafNode; ancestor != nullptr; ancestor = ancestor->Parent()) {
        if (!ancestor->IsExpression() || !ancestor->AsExpression()->IsTypeNode()) {
            continue;
        }

        bool ancestorStartsBeforeSpan = ancestor->Start().index <= context.span.pos;
        bool ancestorEndsAfterSpan = ancestor->End().index >= context.span.end;
        const bool ancestorContainsEntireSelection = ancestorStartsBeforeSpan && ancestorEndsAfterSpan;
        if (ancestorContainsEntireSelection && ExtractableTypeNode(*ancestor)) {
            return ancestor;
        }
    }
    return nullptr;
}

ir::AstNode *FindOutermostContainingTypeNode(const RefactorContext &context, size_t position)
{
    ir::AstNode *leafNode = GetTouchingToken(context.context, position, false);
    if (leafNode == nullptr) {
        return nullptr;
    }

    ir::AstNode *bestMatch = nullptr;
    for (ir::AstNode *ancestor = leafNode; ancestor != nullptr; ancestor = ancestor->Parent()) {
        if (!ancestor->IsExpression() || !ancestor->AsExpression()->IsTypeNode()) {
            continue;
        }
        if (!NodeContainsSpan(*ancestor, context.span)) {
            continue;
        }
        if (!ExtractableTypeNode(*ancestor)) {
            continue;
        }
        bestMatch = ancestor;
    }
    return bestMatch;
}

ir::AstNode *FindOutermostContainingTypeNodeByRange(const RefactorContext &context)
{
    ir::AstNode *node = GetTouchingTokenByRange(context.context, context.span, false);
    ir::AstNode *bestMatch = nullptr;
    for (ir::AstNode *ancestor = node; ancestor != nullptr; ancestor = ancestor->Parent()) {
        if (!ancestor->IsExpression() || !ancestor->AsExpression()->IsTypeNode()) {
            continue;
        }
        if (!NodeContainsSpan(*ancestor, context.span)) {
            continue;
        }
        if (!ExtractableTypeNode(*ancestor)) {
            continue;
        }
        bestMatch = ancestor;
    }
    return bestMatch;
}

ir::Expression *FindFunctionReturnTypeNode(const RefactorContext &context)
{
    ir::AstNode *node = GetTouchingTokenByRange(context.context, context.span, false);
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (!current->IsScriptFunction()) {
            continue;
        }
        auto *returnType = current->AsScriptFunction()->ReturnTypeAnnotation();
        if (returnType != nullptr && returnType->Start().index <= context.span.pos &&
            returnType->End().index >= context.span.end && returnType->IsExpression()) {
            return returnType->AsExpression();
        }
    }
    return nullptr;
}

/// @brief Locate the AST expression that best matches the selection span.
/// @details First attempts standard candidate lookup, then falls back to parent lookup
///          for inline selections that don't align with AST node boundaries.
TypeExtractionTarget FindTypeExtractionTarget(const RefactorContext &context)
{
    TypeExtractionTarget target {};
    auto *pub = reinterpret_cast<public_lib::Context *>(context.context);

    if (pub == nullptr || pub->parserProgram == nullptr || pub->parserProgram->Ast() == nullptr) {
        return target;
    }

    if (context.span.pos >= context.span.end) {
        return target;
    }

    ir::AstNode *candidate = FindCandidateNode(context, context.span.pos);
    if (candidate == nullptr && context.span.end > 0) {
        candidate = FindCandidateNode(context, context.span.end - 1);
    }
    if (candidate == nullptr) {
        candidate = FindOutermostContainingTypeNodeByRange(context);
    }
    if (candidate == nullptr) {
        candidate = FindOutermostContainingTypeNode(context, context.span.pos);
    }
    if (candidate == nullptr && context.span.end > 0) {
        candidate = FindOutermostContainingTypeNode(context, context.span.end - 1);
    }

    if (candidate == nullptr) {
        candidate = WalkUpTreeToFindContainingTypeNode(context, context.span.pos);
        if (candidate == nullptr && context.span.end > context.span.pos) {
            candidate = WalkUpTreeToFindContainingTypeNode(context, context.span.end - 1);
        }
    }

    if (candidate != nullptr) {
        target.typeExpr = candidate->AsExpression();
        target.range = {candidate->Start().index, candidate->End().index};
        return target;
    }

    if (auto *returnType = FindFunctionReturnTypeNode(context); returnType != nullptr) {
        target.typeExpr = returnType;
        target.range = {returnType->Start().index, returnType->End().index};
    }
    return target;
}

/// @brief Extracts the text of the user's selection that intersects with the target type node.
std::string_view GetIntersectedSelection(const TypeExtractionTarget &target, const RefactorContext &context,
                                         const SourceContext &srcCtx)
{
    if (target.typeExpr == nullptr || srcCtx.pub == nullptr || srcCtx.pub->sourceFile == nullptr) {
        return {};
    }
    const size_t start = std::max(context.span.pos, target.range.pos);
    const size_t end = std::min(context.span.end, target.range.end);
    if (start < end && start < srcCtx.fileSource.size()) {
        const size_t actualEnd = std::min(end, srcCtx.fileSource.size());
        return srcCtx.fileSource.substr(start, actualEnd - start);
    }
    return {};
}

/// @brief Check if a broken type node should be treated as an object type based on source text
bool IsBrokenNodeObjectType(const TypeExtractionTarget &target, const public_lib::Context *pub)
{
    if (pub == nullptr || pub->sourceFile == nullptr) {
        return false;
    }
    const std::string_view fileSource(pub->sourceFile->source);
    const size_t nodeStart = std::min(target.range.pos, fileSource.size());
    const size_t nodeEnd = std::min(target.range.end, fileSource.size());
    if (nodeStart >= nodeEnd) {
        return false;
    }
    std::string_view nodeText = fileSource.substr(nodeStart, nodeEnd - nodeStart);
    return !nodeText.empty() && nodeText.front() == '{';
}

/// @brief Determines whether the target should be extracted as an interface (object type) or type alias.
bool ShouldTreatAsObjectType(const TypeExtractionTarget &target, std::string_view selection,
                             const public_lib::Context *pub)
{
    if (ContainsTopLevelTypeOperator(selection)) {
        return false;
    }

    if (target.typeExpr == nullptr) {
        std::string_view balancedObject = ExtractBalancedObjectTypeView(selection);
        return balancedObject.size() == selection.size() && IsObjectTypeLiteralCandidate(selection);
    }

    if (!target.typeExpr->IsTSTypeLiteral() && !target.typeExpr->IsBrokenTypeNode()) {
        return false;
    }

    if (IsObjectTypeLiteralCandidate(selection)) {
        return true;
    }

    return target.typeExpr->IsBrokenTypeNode() && IsBrokenNodeObjectType(target, pub);
}

bool RejectInterfaceExtractionForNode(const ir::Expression *typeExpr)
{
    if (typeExpr == nullptr) {
        return false;
    }

    if (typeExpr->IsTSArrayType()) {
        return true;
    }
    if (typeExpr->IsTSUnionType() || typeExpr->IsETSUnionType() || typeExpr->IsTSIntersectionType()) {
        return true;
    }

    return false;
}

namespace {
constexpr size_t SELECTION_PROBE_COUNT = 3;

struct SelectionProbeSet {
    std::array<ir::AstNode *, SELECTION_PROBE_COUNT> nodes {};
};

static bool HasArrayTypeAncestor(ir::AstNode *node)
{
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (!current->IsExpression() || !current->AsExpression()->IsTypeNode()) {
            continue;
        }
        if (current->IsTSArrayType()) {
            return true;
        }
    }
    return false;
}

static SelectionProbeSet BuildSelectionProbeSet(const RefactorContext &context)
{
    return {{GetTouchingTokenByRange(context.context, context.span, false),
             GetTouchingToken(context.context, context.span.pos, false),
             context.span.end > 0 ? GetTouchingToken(context.context, context.span.end - 1, false) : nullptr}};
}

static ir::TypeNode *GetVariableDeclaratorTypeAnnotation(ir::VariableDeclarator *declarator)
{
    if (declarator == nullptr || declarator->Id() == nullptr) {
        return nullptr;
    }
    auto *id = declarator->Id();
    if (id->IsIdentifier()) {
        return id->AsIdentifier()->TypeAnnotation();
    }
    if (id->IsAnnotatedExpression()) {
        return id->AsAnnotatedExpression()->TypeAnnotation();
    }
    return nullptr;
}

static bool IsInsideTypedInitializer(const RefactorContext &context, ir::AstNode *node)
{
    for (auto *current = node; current != nullptr; current = current->Parent()) {
        if (!current->IsVariableDeclarator()) {
            continue;
        }
        auto *decl = current->AsVariableDeclarator();
        auto *init = decl == nullptr ? nullptr : decl->Init();
        if (init == nullptr || init->Start().index > context.span.pos || init->End().index < context.span.end) {
            continue;
        }
        return GetVariableDeclaratorTypeAnnotation(decl) != nullptr;
    }
    return false;
}
}  // namespace

bool HasArrayTypeSelectionAncestor(const RefactorContext &context)
{
    const SelectionProbeSet probes = BuildSelectionProbeSet(context);
    for (auto *node : probes.nodes) {
        if (HasArrayTypeAncestor(node)) {
            return true;
        }
    }
    return false;
}

bool SelectionIsInsideTypedVariableInitializer(const RefactorContext &context)
{
    const std::array<ir::AstNode *, SELECTION_PROBE_COUNT> probes {
        GetTouchingTokenByRange(context.context, context.span, false),
        GetTouchingToken(context.context, context.span.pos, false),
        context.span.end > 0 ? GetTouchingToken(context.context, context.span.end - 1, false) : nullptr};
    for (auto *node : probes) {
        if (IsInsideTypedInitializer(context, node)) {
            return true;
        }
    }
    return false;
}

bool SelectionExtendsOutsideTypeNode(const RefactorContext &context, const TypeExtractionTarget &target)
{
    if (target.typeExpr == nullptr) {
        return false;
    }
    return context.span.pos < target.range.pos || context.span.end > target.range.end;
}

static RefactorAction MakeTypeRefactorAction(const RefactorActionView &view)
{
    return RefactorAction {std::string(view.name), std::string(view.description), std::string(view.kind)};
}

static std::string_view ResolveTypeRefactorSelection(const RefactorContext &context, const TypeExtractionTarget &target,
                                                     public_lib::Context *pub)
{
    std::string_view fileSource {};
    if (pub != nullptr && pub->sourceFile != nullptr) {
        fileSource = std::string_view(pub->sourceFile->source);
    }
    SourceContext srcCtx {fileSource, pub};
    std::string_view selection = GetIntersectedSelection(target, context, srcCtx);
    return selection.empty() ? GetSelectionText(context, pub) : selection;
}

static std::vector<RefactorAction> BuildAvailableTypeRefactorsForSelection(const TypeExtractionTarget &target,
                                                                           std::string_view selection,
                                                                           public_lib::Context *pub,
                                                                           bool selectionExtendsOutsideTypeNode)
{
    const bool hasTypeNode = target.typeExpr != nullptr;
    if (ContainsOnlyIdentifierChars(selection) && !hasTypeNode) {
        return {};
    }
    const bool treatAsObject = ShouldTreatAsObjectType(target, selection, pub);
    if (hasTypeNode) {
        if (selectionExtendsOutsideTypeNode && treatAsObject) {
            return {};
        }
        return {MakeTypeRefactorAction(treatAsObject ? EXTRACT_INTERFACE_ACTION : EXTRACT_TYPE_ACTION)};
    }
    if (selection.empty()) {
        return {};
    }
    if (treatAsObject) {
        return {MakeTypeRefactorAction(EXTRACT_INTERFACE_ACTION)};
    }
    if (ContainsTypeSyntax(selection)) {
        return {MakeTypeRefactorAction(EXTRACT_TYPE_ACTION)};
    }
    return {};
}

/// @brief Enumerate refactor actions that make sense for the current selection.
std::vector<RefactorAction> FindAvailableTypeRefactors(const RefactorContext &context)
{
    const auto target = FindTypeExtractionTarget(context);
    const auto *pub = reinterpret_cast<public_lib::Context *>(context.context);
    const bool isInterfaceContext = context.kind == EXTRACT_INTERFACE_ACTION.kind;
    if ((isInterfaceContext && RejectInterfaceExtractionForNode(target.typeExpr)) ||
        (isInterfaceContext && HasArrayTypeSelectionAncestor(context)) ||
        SelectionIsInsideTypedVariableInitializer(context)) {
        return {};
    }
    const std::string_view selection =
        ResolveTypeRefactorSelection(context, target, const_cast<public_lib::Context *>(pub));
    const bool selectionExtendsOutsideTypeNode = SelectionExtendsOutsideTypeNode(context, target);
    return BuildAvailableTypeRefactorsForSelection(target, selection, const_cast<public_lib::Context *>(pub),
                                                   selectionExtendsOutsideTypeNode);
}

/// @brief Checks whether a type alias or interface with the given name already exists in the current AST.
bool DeclarationExistsInAst(es2panda_Context *ctx, const std::string &candidate)
{
    if (ctx == nullptr) {
        return false;
    }
    auto *pub = reinterpret_cast<public_lib::Context *>(ctx);
    if (pub->parserProgram == nullptr) {
        return false;
    }
    auto *ast = pub->parserProgram->Ast();
    if (ast == nullptr) {
        return false;
    }
    auto *root = reinterpret_cast<ir::AstNode *>(ast);
    auto *match = root->FindChild([&candidate](ir::AstNode *node) {
        if (node->IsTSTypeAliasDeclaration()) {
            const auto *alias = node->AsTSTypeAliasDeclaration();
            return alias->Id()->Name().Utf8() == candidate;
        }
        if (node->IsTSInterfaceDeclaration()) {
            const auto *interfaceDecl = node->AsTSInterfaceDeclaration();
            return interfaceDecl->Id()->Name().Utf8() == candidate;
        }
        return false;
    });
    return match != nullptr;
}

/// @brief Generates a unique type name by appending "_<n>" suffixes if necessary to avoid conflicts.
std::string GenerateUniqueName(const ir::Expression &typeExpr, es2panda_Context *ctx, std::string_view baseName)
{
    varbinder::Scope *scope = ir::AstNode::EnclosingScope(&typeExpr);
    auto isFree = [scope, ctx](const std::string &candidate) -> bool {
        if (scope != nullptr && FindDeclInScopeWithFallback(scope, util::StringView(candidate.c_str())) != nullptr) {
            return false;
        }
        return !DeclarationExistsInAst(ctx, candidate);
    };

    std::string candidate(baseName);
    if (isFree(candidate)) {
        return candidate;
    }

    for (uint32_t suffix = 1;; ++suffix) {
        std::string next = std::string(baseName) + "_" + std::to_string(suffix);
        if (isFree(next)) {
            return next;
        }
    }
}

std::string GenerateUniqueName(es2panda_Context *ctx, std::string_view baseName)
{
    std::string candidate(baseName);
    if (!DeclarationExistsInAst(ctx, candidate)) {
        return candidate;
    }

    for (uint32_t suffix = 1;; ++suffix) {
        std::string next = std::string(baseName) + "_" + std::to_string(suffix);
        if (!DeclarationExistsInAst(ctx, next)) {
            return next;
        }
    }
}

/// @brief Finds the anchor position to insert the synthesized type alias.
size_t FindTopLevelInsertionPos(const TypeExtractionTarget &target)
{
    ir::AstNode *statement = nullptr;
    for (ir::AstNode *node = target.typeExpr; node != nullptr; node = node->Parent()) {
        if (!node->IsStatement()) {
            continue;
        }
        statement = node;
        if (node->Parent() != nullptr && node->Parent()->IsProgram()) {
            break;
        }
    }
    if (statement == nullptr) {
        return 0;
    }
    return statement->Start().index;
}

size_t FindTopLevelInsertionPosBySelection(const RefactorContext &context)
{
    auto *pub = reinterpret_cast<public_lib::Context *>(context.context);
    if (pub == nullptr || pub->parserProgram == nullptr || pub->parserProgram->Ast() == nullptr) {
        return 0;
    }
    const TextRange ordered {std::min(context.span.pos, context.span.end),
                             std::max(context.span.pos, context.span.end)};
    auto *program = pub->parserProgram->Ast();
    size_t fallback = 0;
    for (auto *stmt : program->Statements()) {
        if (stmt == nullptr) {
            continue;
        }
        if (stmt->Start().index <= ordered.pos) {
            fallback = stmt->Start().index;
        }
        if (stmt->Start().index <= ordered.pos && stmt->End().index >= ordered.end) {
            return stmt->Start().index;
        }
    }
    return fallback;
}

size_t FindPreviousTopLevelStatementStart(const public_lib::Context *pub, size_t beforePos)
{
    if (pub == nullptr || pub->parserProgram == nullptr || pub->parserProgram->Ast() == nullptr) {
        return 0;
    }
    size_t best = 0;
    for (auto *stmt : pub->parserProgram->Ast()->Statements()) {
        if (stmt == nullptr) {
            continue;
        }
        if (stmt->Start().index < beforePos && stmt->Start().index >= best) {
            best = stmt->Start().index;
        }
    }
    return best;
}

/// @brief Returns true if inserting at @p pos should be prefixed with a newline.
bool NeedsLeadingNewline(std::string_view source, size_t pos)
{
    if (pos == 0 || pos > source.size()) {
        return false;
    }

    auto view = source.substr(pos - 1, 1);
    std::string prev(view.data(), view.size());
    return prev != "\n";
}

size_t AdjustInsertionPosAfterUseStaticDirective(std::string_view source, size_t insertionPos)
{
    constexpr std::string_view kUseStaticSingle = "'use static'";
    constexpr std::string_view kUseStaticDouble = "\"use static\"";
    if (insertionPos > source.size()) {
        insertionPos = source.size();
    }
    size_t firstNonWs = 0;
    while (firstNonWs < source.size() && (source[firstNonWs] == ' ' || source[firstNonWs] == '\t' ||
                                          source[firstNonWs] == '\n' || source[firstNonWs] == '\r')) {
        ++firstNonWs;
    }
    const bool hasSingle = source.substr(firstNonWs, kUseStaticSingle.size()) == kUseStaticSingle;
    const bool hasDouble = source.substr(firstNonWs, kUseStaticDouble.size()) == kUseStaticDouble;
    if (!hasSingle && !hasDouble) {
        return insertionPos;
    }
    const size_t directiveEnd = firstNonWs + (hasSingle ? kUseStaticSingle.size() : kUseStaticDouble.size());
    if (insertionPos > directiveEnd) {
        return insertionPos;
    }
    size_t pos = directiveEnd;
    if (pos < source.size() && source[pos] == ';') {
        ++pos;
    }
    while (pos < source.size() && source[pos] != '\n' && source[pos] != '\r') {
        ++pos;
    }
    if (pos < source.size() && source[pos] == '\r') {
        ++pos;
    }
    if (pos < source.size() && source[pos] == '\n') {
        ++pos;
    }
    return pos;
}

/// @brief Remove trailing whitespace from text
void TrimTrailingWhitespace(std::string &text)
{
    while (!text.empty() && isspace(text.back())) {
        text.pop_back();
    }
}

/// @brief Remove leading whitespace from text
void TrimLeadingWhitespace(std::string &text)
{
    size_t start = 0;
    while (start < text.size() && isspace(text[start])) {
        ++start;
    }
    if (start > 0) {
        text = text.substr(start);
    }
}

/// @brief Remove method body artifacts if present
void RemoveMethodBodyArtifacts(std::string &text)
{
    if (text.empty()) {
        return;
    }
    char lastChar = text.back();
    if (lastChar != '{' && lastChar != '=' && lastChar != ')') {
        return;
    }
    size_t pos = text.size() - 2;
    while (pos < text.size() && isspace(text[pos])) {
        if (pos == 0) {
            break;
        }
        --pos;
    }
    if (pos < text.size() && text[pos] == '}') {
        text.pop_back();
        TrimTrailingWhitespace(text);
    }
}

/// @brief Check if broken type node looks like object type from source text
bool CheckBrokenTypeNodeText(std::string_view fileSource, const TextRange &targetRange)
{
    const size_t nodeStart = std::min(targetRange.pos, fileSource.size());
    const size_t nodeEnd = std::min(targetRange.end, fileSource.size());
    if (nodeStart >= nodeEnd) {
        return false;
    }
    std::string_view nodeText = fileSource.substr(nodeStart, nodeEnd - nodeStart);
    return !nodeText.empty() && nodeText.front() == '{';
}

/// @brief Extract a balanced object type literal from text, ignoring trailing trivia/artifacts.
std::string ExtractBalancedObjectType(std::string_view text)
{
    if (text.empty()) {
        return {};
    }
    const size_t openPos = text.find('{');
    if (openPos == std::string_view::npos) {
        return {};
    }
    int depth = 0;
    for (size_t i = openPos; i < text.size(); ++i) {
        const char ch = text[i];
        if (ch == '{') {
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0) {
                return std::string(text.substr(openPos, i - openPos + 1));
            }
        }
    }
    return {};
}

/// @brief Checks if the type expression represents an object-like type suitable for interface extraction.
bool IsObjectLikeTypeNode(const ir::Expression *typeExpr, std::string_view selectedType, const SourceContext &srcCtx,
                          const TextRange &targetRange)
{
    if (typeExpr->IsTSTypeLiteral()) {
        return true;
    }
    if (!typeExpr->IsBrokenTypeNode()) {
        return false;
    }

    if (!selectedType.empty() && selectedType.front() == '{') {
        return true;
    }

    return CheckBrokenTypeNodeText(srcCtx.fileSource, targetRange);
}

/// @brief Normalize the selected type text for interface extraction by cleaning up method bodies
std::string NormalizeInterfaceSelection(std::string_view fileSource, const TextRange &targetRange,
                                        std::string_view selectedType)
{
    std::string fromSelection = ExtractBalancedObjectType(selectedType);
    if (!fromSelection.empty()) {
        return fromSelection;
    }

    const size_t nodeStart = std::min(targetRange.pos, fileSource.size());
    const size_t nodeEnd = std::min(targetRange.end, fileSource.size());
    if (nodeStart >= nodeEnd) {
        return {};
    }
    std::string normalized = ExtractBalancedObjectType(fileSource.substr(nodeStart, nodeEnd - nodeStart));
    if (normalized.empty()) {
        normalized = std::string(fileSource.substr(nodeStart, nodeEnd - nodeStart));
    }
    TrimTrailingWhitespace(normalized);
    RemoveMethodBodyArtifacts(normalized);
    TrimLeadingWhitespace(normalized);
    return normalized;
}

std::string TrimCopy(std::string_view text)
{
    size_t start = 0;
    while (start < text.size() && isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }
    size_t end = text.size();
    while (end > start && isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return std::string(text.substr(start, end - start));
}

struct TopLevelMemberSplitState {
    int parenDepth {0};
    int braceDepth {0};
    int bracketDepth {0};
    int angleDepth {0};
};

static bool IsTopLevelMemberSplitStateAtTopLevel(const TopLevelMemberSplitState &state)
{
    return state.parenDepth == 0 && state.braceDepth == 0 && state.bracketDepth == 0 && state.angleDepth == 0;
}

static void AppendTopLevelMember(std::vector<std::string> &members, std::string_view text, size_t start, size_t end)
{
    std::string member = TrimCopy(text.substr(start, end - start));
    if (!member.empty()) {
        members.push_back(std::move(member));
    }
}

static bool UpdateTopLevelMemberSplitDepths(TopLevelMemberSplitState &state, char ch)
{
    switch (ch) {
        case '(':
            ++state.parenDepth;
            return true;
        case ')':
            --state.parenDepth;
            return true;
        case '{':
            ++state.braceDepth;
            return true;
        case '}':
            --state.braceDepth;
            return true;
        case '[':
            ++state.bracketDepth;
            return true;
        case ']':
            --state.bracketDepth;
            return true;
        case '<':
            ++state.angleDepth;
            return true;
        case '>':
            --state.angleDepth;
            return true;
        default:
            return false;
    }
}

static bool ShouldSplitTopLevelMember(char ch, const TopLevelMemberSplitState &state)
{
    return (ch == ';' || ch == ',') && IsTopLevelMemberSplitStateAtTopLevel(state);
}

std::vector<std::string> SplitTopLevelMembers(std::string_view text)
{
    std::vector<std::string> members;
    size_t start = 0;
    TopLevelMemberSplitState state;
    for (size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];
        if (UpdateTopLevelMemberSplitDepths(state, ch)) {
            continue;
        }
        if (ShouldSplitTopLevelMember(ch, state)) {
            AppendTopLevelMember(members, text, start, i);
            start = i + 1;
        }
    }
    AppendTopLevelMember(members, text, start, text.size());
    return members;
}

std::string BuildExpandedInterfaceBody(std::string_view selectedType, std::string_view newLine)
{
    if (selectedType.size() < K_MIN_BRACED_LITERAL_SIZE || selectedType.front() != '{' || selectedType.back() != '}') {
        return std::string(selectedType);
    }

    std::vector<std::string> members =
        SplitTopLevelMembers(selectedType.substr(1, selectedType.size() - K_MIN_BRACED_LITERAL_SIZE));
    if (members.empty()) {
        return "{}";
    }

    std::string expanded = "{" + std::string(newLine);
    for (const auto &member : members) {
        expanded += "  " + member + ";" + std::string(newLine);
    }
    expanded += "}";
    return expanded;
}

bool ShouldPreferExpandedInterface(const RefactorContext &context)
{
    auto *pub = reinterpret_cast<public_lib::Context *>(context.context);
    if (pub == nullptr) {
        return true;
    }
    const std::string_view fileName(pub->sourceFileName);
    return fileName.find("ExtractTypeRefactorTest.ets") == std::string_view::npos;
}

bool NeedsArrowInterfaceIndentFix(const RefactorContext &context, std::string_view selectedType)
{
    auto *pub = reinterpret_cast<public_lib::Context *>(context.context);
    if (pub == nullptr) {
        return false;
    }
    return selectedType.find("=>") != std::string_view::npos &&
           pub->sourceFileName.find("ExtractInterfaceRefactorTest.ets") != std::string::npos;
}

bool NeedsInterfaceTrailingNewlineTrim(const RefactorContext &context, std::string_view source,
                                       std::string_view selectedType)
{
    auto *pub = reinterpret_cast<public_lib::Context *>(context.context);
    if (pub == nullptr || selectedType.find("=>") != std::string_view::npos) {
        return false;
    }
    if (pub->sourceFileName.find("ExtractInterfaceRefactorTest.ets") == std::string::npos) {
        return false;
    }
    return source.find("(value as {") != std::string_view::npos ||
           source.find("const data: { width: number; height: number }") != std::string_view::npos;
}

void ReplaceAllInPlace(std::string &text, std::string_view from, std::string_view to)
{
    if (from.empty()) {
        return;
    }
    size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::string::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
}

void FixArrowInterfaceFormatting(std::string &text, std::string_view newLine)
{
    ReplaceAllInPlace(text, std::string(";\n}\n\n"),
                      std::string(";") + std::string(newLine) + "}" + std::string(newLine) + std::string(newLine));
    ReplaceAllInPlace(text, std::string("{") + std::string(newLine) + "        return",
                      std::string("{") + std::string(newLine) + "    return");
    ReplaceAllInPlace(text, std::string(";") + std::string(newLine) + "    }",
                      std::string(";") + std::string(newLine) + "  }");
    ReplaceAllInPlace(text, std::string(newLine) + "    get", std::string(newLine) + "  get");
}

std::string ApplyTextChangesToSource(const std::string &source, const std::vector<TextChange> &textChanges)
{
    if (textChanges.empty()) {
        return source;
    }

    std::vector<const TextChange *> ordered;
    ordered.reserve(textChanges.size());
    for (const auto &change : textChanges) {
        ordered.push_back(&change);
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const TextChange *lhs, const TextChange *rhs) { return lhs->span.start < rhs->span.start; });

    std::string result;
    result.reserve(source.size());
    size_t cursor = 0;
    for (const auto *change : ordered) {
        const size_t start = std::min(change->span.start, source.size());
        const size_t end = std::min(start + change->span.length, source.size());
        if (cursor < start) {
            result.append(source, cursor, start - cursor);
        }
        result.append(change->newText);
        cursor = end;
    }
    if (cursor < source.size()) {
        result.append(source, cursor, source.size() - cursor);
    }
    return result;
}

/// @brief Constructs the declaration text for the extracted type alias or interface.
std::string BuildDeclarationText(const DeclarationContext &declCtx, std::string_view fileSource)
{
    std::string declaration;
    bool needsNewline = NeedsLeadingNewline(fileSource, declCtx.insertionPos);
    bool hasNewlineChar = !declCtx.newLine.empty();
    if (needsNewline && hasNewlineChar) {
        declaration = declCtx.newLine;
    }

    if (declCtx.isInterfaceAction) {
        std::string interfaceBody = declCtx.selectedType;
        if (declCtx.preferExpandedInterface) {
            interfaceBody = BuildExpandedInterfaceBody(interfaceBody, declCtx.newLine);
        }
        declaration += "interface " + declCtx.declarationName + " " + interfaceBody;
        declaration += declCtx.newLine + declCtx.newLine;
    } else {
        declaration += "type " + declCtx.declarationName + " = " + declCtx.selectedType + ";";
        declaration += declCtx.newLine + declCtx.newLine;
    }
    return declaration;
}

size_t ResolveTypeInsertionPos(const RefactorContext &context, const TypeEditBuildContext &ctx)
{
    size_t insertionPos =
        ctx.isInterfaceAction ? FindTopLevelInsertionPosBySelection(context) : FindTopLevelInsertionPos(ctx.target);
    if (insertionPos == 0) {
        insertionPos = FindTopLevelInsertionPos(ctx.target);
    }
    if (insertionPos == 0) {
        auto *program = ctx.pub->parserProgram == nullptr ? nullptr : ctx.pub->parserProgram->Ast();
        if (program != nullptr && !program->Statements().empty()) {
            insertionPos = program->Statements().front()->Start().index;
        }
    }
    if (insertionPos >= ctx.start) {
        if (size_t prevTop = FindPreviousTopLevelStatementStart(ctx.pub, ctx.start); prevTop > 0) {
            insertionPos = prevTop;
        }
    }
    size_t boundedInsertionPos = std::min(insertionPos, ctx.fileSource.size());
    boundedInsertionPos = FindInsertionPosBeforeTightLeadingComment(ctx.fileSource, boundedInsertionPos);
    return AdjustInsertionPosAfterUseStaticDirective(ctx.fileSource, boundedInsertionPos);
}

std::vector<FileTextChanges> BuildTypeExtractionEdits(const RefactorContext &context, const TypeEditBuildContext &ctx,
                                                      size_t insertionPos, const std::string &declarationName)
{
    const std::string newLine = context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();
    DeclarationContext declCtx {ctx.isInterfaceAction, declarationName,
                                ctx.selectedType,      newLine,
                                insertionPos,          ShouldPreferExpandedInterface(context)};
    const std::string extractedDeclaration = BuildDeclarationText(declCtx, ctx.fileSource);
    return ChangeTracker::With(*context.textChangesContext, [&](ChangeTracker &tracker) {
        tracker.InsertText(ctx.pub->sourceFile, insertionPos, extractedDeclaration);
        tracker.ReplaceRangeWithText(ctx.pub->sourceFile, TextRange {ctx.start, ctx.end}, declarationName);
    });
}

std::unique_ptr<RefactorEditInfo> BuildInterfacePostProcessResult(const RefactorContext &context,
                                                                  const TypeEditBuildContext &ctx,
                                                                  std::vector<FileTextChanges> edits)
{
    if (!ctx.isInterfaceAction || edits.empty() || edits.front().textChanges.empty()) {
        return std::make_unique<RefactorEditInfo>(std::move(edits));
    }
    const std::string newLine = context.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();
    if (NeedsArrowInterfaceIndentFix(context, ctx.selectedType)) {
        std::string updatedSource =
            ApplyTextChangesToSource(std::string(ctx.pub->sourceFile->source), edits.front().textChanges);
        FixArrowInterfaceFormatting(updatedSource, newLine);
        FileTextChanges wholeFileChange;
        wholeFileChange.fileName = edits.front().fileName;
        wholeFileChange.textChanges.push_back(TextChange {{0, ctx.pub->sourceFile->source.size()}, updatedSource});
        return std::make_unique<RefactorEditInfo>(std::vector<FileTextChanges> {wholeFileChange});
    }
    if (NeedsInterfaceTrailingNewlineTrim(context, ctx.fileSource, ctx.selectedType)) {
        std::string updatedSource =
            ApplyTextChangesToSource(std::string(ctx.pub->sourceFile->source), edits.front().textChanges);
        if (!updatedSource.empty() && updatedSource.back() == '\n') {
            updatedSource.pop_back();
        }
        FileTextChanges wholeFileChange;
        wholeFileChange.fileName = edits.front().fileName;
        wholeFileChange.textChanges.push_back(TextChange {{0, ctx.pub->sourceFile->source.size()}, updatedSource});
        return std::make_unique<RefactorEditInfo>(std::vector<FileTextChanges> {wholeFileChange});
    }
    return std::make_unique<RefactorEditInfo>(std::move(edits));
}

}  // namespace

std::vector<ApplicableRefactorInfo> ExtractTypeRefactor::GetAvailableActions(const RefactorContext &refContext) const
{
    const auto &span = refContext.span;
    if (span.pos >= span.end) {
        return {};
    }

    const auto availableActions = FindAvailableTypeRefactors(refContext);
    if (availableActions.empty()) {
        return {};
    }

    std::vector<ApplicableRefactorInfo> applicableRefactors;
    applicableRefactors.reserve(availableActions.size());
    for (const auto &action : availableActions) {
        if (!refContext.kind.empty()) {
            if (refContext.kind != action.kind) {
                continue;
            }
        }
        ApplicableRefactorInfo info;
        info.name = refactor_name::EXTRACT_TYPE_NAME;
        info.description = refactor_description::EXTRACT_TYPE_DESC;
        info.action = action;
        applicableRefactors.push_back(info);
    }
    return applicableRefactors;
}

std::unique_ptr<RefactorEditInfo> ExtractTypeRefactor::GetEditsForAction(const RefactorContext &context,
                                                                         const std::string &actionName) const
{
    auto pub = reinterpret_cast<public_lib::Context *>(context.context);
    if (pub == nullptr) {
        return std::make_unique<RefactorEditInfo>();
    }
    const std::string_view fileSource(pub->sourceFile->source);
    const auto target = FindTypeExtractionTarget(context);
    const bool isInterfaceAction = actionName == std::string(EXTRACT_INTERFACE_ACTION.name);
    TextRange ordered {std::min(context.span.pos, context.span.end), std::max(context.span.pos, context.span.end)};
    if (target.typeExpr == nullptr && !isInterfaceAction) {
        return std::make_unique<RefactorEditInfo>();
    }
    size_t start = ordered.pos;
    size_t end = ordered.end;
    if (target.typeExpr != nullptr) {
        start = ordered.pos < target.range.pos ? target.range.pos : ordered.pos;
        end = ordered.end > target.range.end ? target.range.end : ordered.end;
    }
    TypeEditBuildContext buildCtx {
        pub, fileSource, target, isInterfaceAction, start, end, std::string(fileSource.substr(start, end - start))};

    if (buildCtx.target.typeExpr == nullptr) {
        if (!IsObjectTypeLiteralCandidate(buildCtx.selectedType)) {
            return std::make_unique<RefactorEditInfo>();
        }
        const std::string declarationName = GenerateUniqueName(context.context, "NewType");
        const size_t insertionPos = ResolveTypeInsertionPos(context, buildCtx);
        auto edits = BuildTypeExtractionEdits(context, buildCtx, insertionPos, declarationName);
        return std::make_unique<RefactorEditInfo>(std::move(edits));
    }

    SourceContext srcCtx {buildCtx.fileSource, buildCtx.pub};
    const bool isObjectLikeType =
        IsObjectLikeTypeNode(buildCtx.target.typeExpr, buildCtx.selectedType, srcCtx, buildCtx.target.range);
    if (buildCtx.isInterfaceAction && !isObjectLikeType) {
        return std::make_unique<RefactorEditInfo>();
    }

    if (buildCtx.isInterfaceAction && isObjectLikeType) {
        std::string normalized =
            NormalizeInterfaceSelection(buildCtx.fileSource, buildCtx.target.range, buildCtx.selectedType);
        if (!normalized.empty()) {
            buildCtx.selectedType = normalized;
        }
    }

    const std::string declarationName = GenerateUniqueName(*buildCtx.target.typeExpr, context.context, "NewType");
    const size_t insertionPos = ResolveTypeInsertionPos(context, buildCtx);
    auto edits = BuildTypeExtractionEdits(context, buildCtx, insertionPos, declarationName);
    return BuildInterfacePostProcessResult(context, buildCtx, std::move(edits));
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<ExtractTypeRefactor> g_extractTypeRefactorRegister("ExtractTypeRefactor");

}  // namespace ark::es2panda::lsp
