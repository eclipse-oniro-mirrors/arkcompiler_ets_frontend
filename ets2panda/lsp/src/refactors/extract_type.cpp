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

/// @brief Heuristic check to determine if @p text can be treated as an inline object type literal.
bool IsObjectTypeLiteralCandidate(std::string_view text)
{
    if (text.size() < K_MIN_BRACED_LITERAL_SIZE || text.front() != '{' || text.back() != '}') {
        return false;
    }
    return text.find(':') != std::string_view::npos;
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
        if (!std::isalnum(ch) && ch != '_' && ch != '$') {
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
static ir::AstNode *FindCandidateNode(const RefactorContext &context, size_t position)
{
    ir::AstNode *node = GetTouchingToken(context.context, position, false);
    while (node != nullptr) {
        if (NodeContainsSpan(*node, context.span) && ExtractableTypeNode(*node)) {
            return node;
        }
        node = node->Parent();
    }
    return nullptr;
}

/// @brief Locate the AST expression that best matches the selection span.
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

    if (candidate != nullptr) {
        target.typeExpr = candidate->AsExpression();
        target.range = {candidate->Start().index, candidate->End().index};
    }
    return target;
}

/// @brief Enumerate refactor actions that make sense for the current selection.
std::vector<RefactorAction> FindAvailableTypeRefactors(const RefactorContext &context)
{
    const auto target = FindTypeExtractionTarget(context);
    const auto *pub = reinterpret_cast<public_lib::Context *>(context.context);
    const std::string_view selection = GetSelectionText(context, pub);
    if (ContainsOnlyIdentifierChars(selection)) {
        return {};
    }
    const bool hasTypeNode = target.typeExpr != nullptr;
    const bool looksObject = IsObjectTypeLiteralCandidate(selection);
    const bool nodeIsObjectType = hasTypeNode && target.typeExpr->IsTSTypeLiteral();
    const bool treatAsObject = nodeIsObjectType || looksObject;
    auto makeAction = [](const RefactorActionView &view) -> RefactorAction {
        return RefactorAction {std::string(view.name), std::string(view.description), std::string(view.kind)};
    };
    std::vector<RefactorAction> actions;
    if (hasTypeNode) {
        if (treatAsObject) {
            actions.push_back(makeAction(EXTRACT_INTERFACE_ACTION));
            return actions;
        }
        actions.push_back(makeAction(EXTRACT_TYPE_ACTION));
        return actions;
    }
    if (selection.empty()) {
        return {};
    }
    if (treatAsObject) {
        actions.push_back(makeAction(EXTRACT_INTERFACE_ACTION));
        return actions;
    }
    if (ContainsTypeSyntax(selection)) {
        actions.push_back(makeAction(EXTRACT_TYPE_ACTION));
    }
    return actions;
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
        std::string next = std::string(baseName) + std::to_string(suffix);
        if (isFree(next)) {
            return next;
        }
    }
}

/// @brief Generates a unique alias name that does not shadow existing declarations.
std::string GenerateAliasName(const ir::Expression &typeExpr, es2panda_Context *ctx)
{
    constexpr std::string_view K_ALIAS_BASE_NAME = "ExtractedType";
    return GenerateUniqueName(typeExpr, ctx, K_ALIAS_BASE_NAME);
}

/// @brief Generates a unique interface name that does not shadow existing declarations.
std::string GenerateInterfaceName(const ir::Expression &typeExpr, es2panda_Context *ctx)
{
    constexpr std::string_view K_INTERFACE_BASE_NAME = "ExtractedInterface";
    return GenerateUniqueName(typeExpr, ctx, K_INTERFACE_BASE_NAME);
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
    if (ir::AstNode *boundary = ChangeTracker::ToEditBoundary(statement);
        boundary != nullptr && !boundary->IsProgram()) {
        return boundary->Start().index;
    }
    return statement->Start().index;
}

/// @brief Returns true if inserting at @p pos should be prefixed with a newline.
bool NeedsLeadingNewline(std::string_view source, size_t pos)
{
    if (pos == 0 || pos > source.size()) {
        return false;
    }

    auto view = source.substr(pos - 1, 1);
    std::string prev(view.data(), view.size());
#ifdef _WIN32
    return prev != "\r\n";
#else
    return prev != "\n";
#endif
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

std::unique_ptr<RefactorEditInfo> ExtractTypeRefactor::GetEditsForAction(const RefactorContext &ctx,
                                                                         const std::string &actName) const
{
    auto pub = reinterpret_cast<public_lib::Context *>(ctx.context);
    if (pub == nullptr) {
        return std::make_unique<RefactorEditInfo>();
    }
    const std::string_view fileSource(pub->sourceFile->source);
    const auto target = FindTypeExtractionTarget(ctx);
    const bool isInterfaceAction = actName == std::string(EXTRACT_INTERFACE_ACTION.name);
    if (target.typeExpr == nullptr) {
        return std::make_unique<RefactorEditInfo>();
    }
    TextRange ordered {std::min(ctx.span.pos, ctx.span.end), std::max(ctx.span.pos, ctx.span.end)};
    size_t start = ordered.pos < target.range.pos ? target.range.pos : ordered.pos;
    size_t end = ordered.end > target.range.end ? target.range.end : ordered.end;

    const std::string selectedType(fileSource.substr(start, end - start));
    if (isInterfaceAction && !target.typeExpr->IsTSTypeLiteral() && !IsObjectTypeLiteralCandidate(selectedType)) {
        return std::make_unique<RefactorEditInfo>();
    }

    std::string declarationName = isInterfaceAction ? GenerateInterfaceName(*target.typeExpr, ctx.context)
                                                    : GenerateAliasName(*target.typeExpr, ctx.context);
    const std::string newLine = ctx.textChangesContext->formatContext.GetFormatCodeSettings().GetNewLineCharacter();
    size_t insertionPos = FindTopLevelInsertionPos(target);
    if (insertionPos == 0) {
        auto *program = pub->parserProgram->Ast();
        if (program != nullptr && !program->Statements().empty()) {
            insertionPos = program->Statements().front()->Start().index;
        }
    }
    const size_t boundedInsertionPos = std::min(insertionPos, fileSource.size());
    std::string extractedDeclaration;
    if (NeedsLeadingNewline(fileSource, boundedInsertionPos) && !newLine.empty()) {
        extractedDeclaration += newLine;
    }
    if (isInterfaceAction) {
        extractedDeclaration += "interface " + declarationName + " " + selectedType + newLine + newLine;
    } else {
        extractedDeclaration += "type " + declarationName + " = " + selectedType + ";" + newLine + newLine;
    }
    auto edits = ChangeTracker::With(*ctx.textChangesContext, [&](ChangeTracker &tracker) {
        tracker.InsertText(pub->sourceFile, boundedInsertionPos, extractedDeclaration);
        TextRange trackerRange {start, end};
        tracker.ReplaceRangeWithText(pub->sourceFile, trackerRange, declarationName);
    });
    return std::make_unique<RefactorEditInfo>(std::move(edits));
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<ExtractTypeRefactor> g_extractTypeRefactorRegister("ExtractTypeRefactor");

}  // namespace ark::es2panda::lsp
