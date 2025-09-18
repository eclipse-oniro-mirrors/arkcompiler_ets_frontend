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

#include "lsp/include/services/text_change/change_tracker.h"
#include "get_adjusted_location.h"
#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace ark::es2panda::lsp {

ConfigurableStartEnd g_useNonAdjustedPositions = {{LeadingTriviaOption::EXCLUDE}, {TrailingTriviaOption::EXCLUDE}};

bool IsEditBoundaryNode(const ir::AstNode *n)
{
    using ark::es2panda::ir::ModifierFlags;
    using ir::AstNodeType;
    if (n == nullptr) {
        return false;
    }

    switch (n->Type()) {
        case AstNodeType::FUNCTION_DECLARATION:
        case AstNodeType::CLASS_DECLARATION:
        case AstNodeType::VARIABLE_DECLARATION:
        case AstNodeType::METHOD_DEFINITION:
        case AstNodeType::PROPERTY:
        case AstNodeType::CLASS_PROPERTY:
        case AstNodeType::TS_INTERFACE_DECLARATION:
        case AstNodeType::TS_ENUM_DECLARATION:
        case AstNodeType::TS_TYPE_ALIAS_DECLARATION:
        case AstNodeType::TS_MODULE_DECLARATION:
        case AstNodeType::IMPORT_DECLARATION:
        case AstNodeType::EXPORT_ALL_DECLARATION:
        case AstNodeType::EXPORT_DEFAULT_DECLARATION:
        case AstNodeType::EXPORT_NAMED_DECLARATION:
        case AstNodeType::LABELLED_STATEMENT:
        case AstNodeType::STRUCT_DECLARATION:
            return true;
        default:
            break;
    }

    const auto mods = static_cast<uint32_t>(n->Modifiers());
    return (mods & static_cast<uint32_t>(ModifierFlags::DEFAULT_EXPORT)) != 0U;
}

ir::AstNode *ChangeTracker::ToEditBoundary(ir::AstNode *node)
{
    if (node == nullptr) {
        return nullptr;
    }

    ir::AstNode *cur = node;
    if (cur->Parent() != nullptr && cur->Parent()->IsBlockStatement()) {
        return cur;
    }

    while (cur != nullptr) {
        if (IsEditBoundaryNode(cur)) {
            break;
        }

        ir::AstNode *parent = cur->Parent();
        if (parent == nullptr) {
            break;
        }

        if (parent->IsBlockStatement()) {
            return cur;
        }

        if (parent->IsImportDeclaration() || parent->IsExportAllDeclaration() || parent->IsExportDefaultDeclaration()) {
            cur = parent;
            break;
        }

        cur = parent;
    }

    return cur;
}

// Build a [start, end) range from two arbitrary nodes by snapping to edit boundaries.
static inline TextRange MakeEditRange(ir::AstNode *startNode, ir::AstNode *endNode)
{
    ir::AstNode *start = ChangeTracker::ToEditBoundary(startNode);
    ir::AstNode *end = ChangeTracker::ToEditBoundary(endNode);
    const size_t s = (start != nullptr) ? start->Start().index : startNode->Start().index;
    const size_t e = (end != nullptr) ? end->End().index : endNode->End().index;
    return {s, e};
}

ChangeTracker ChangeTracker::FromContext(TextChangesContext &context)
{
    return ChangeTracker(context.formatContext, context.formatContext.GetFormatCodeSettings().GetNewLineCharacter());
}

std::vector<FileTextChanges> ChangeTracker::With(TextChangesContext &context,
                                                 const std::function<void(ChangeTracker &)> &cb)
{
    auto tracker = FromContext(context);
    cb(tracker);
    ValidateNonFormattedText validateNonFormattedText = [](ark::es2panda::ir::AstNode *, const std::string &) {};
    return tracker.GetChanges();
}
ir::AstNode *ChangeTracker::GetAstFromContext(const es2panda_Context *context)
{
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(const_cast<es2panda_Context *>(context));
    auto ast = reinterpret_cast<ir::AstNode *>(ctx->parserProgram->Ast());
    return ast;
}

size_t ChangeTracker::GetStartPositionOfLine(size_t line, const es2panda_Context *context)
{
    auto ast = GetAstFromContext(context);
    ir::AstNode *targetNode;
    ast->FindChild([line, &targetNode](ark::es2panda::ir::AstNode *node) {
        if (node->Start().line == line) {
            targetNode = node;
        }
        return false;
    });
    if (targetNode != nullptr) {
        return targetNode->Start().index;
    }
    return 0;
}

void ChangeTracker::RfindNearestKeyWordTextRange(const es2panda_Context *context, const size_t pos,
                                                 const std::string_view &keywordStr, TextRange &range)
{
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(const_cast<es2panda_Context *>(context));
    const std::string_view &sourceCode = ctx->parserProgram->SourceCode().Utf8();
    auto start = sourceCode.rfind(keywordStr, pos);
    if (start == std::string_view::npos) {
        return;
    }

    range.pos = start;
    range.end = start + keywordStr.length();
}

bool ChangeTracker::RangeContainsPosition(TextRange r, size_t pos)
{
    return r.pos <= pos && pos <= r.end;
}

void ChangeTracker::ReplaceRangeWithNodes(es2panda_Context *context, const TextRange range,
                                          std::vector<ir::AstNode *> &newNodes, ReplaceWithMultipleNodesOptions options)
{
    auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    const auto sourceFile = astContext->sourceFile;
    ReplaceWithMultipleNodes replaceNodes = {sourceFile, range, ChangeKind::REPLACEWITHMULTIPLENODES, newNodes,
                                             options};
    changes_.emplace_back(replaceNodes);
}
ir::AstNode *ChangeTracker::NextCommaToken(es2panda_Context *context, const ir::AstNode *node)
{
    auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto *astNodes = astContext->parserProgram->Ast();
    const auto children = GetChildren(astNodes, astContext->allocator);
    const auto next = FindRightToken(node->Start().index, children);
    return next;
}

void ChangeTracker::InsertNodesAt(es2panda_Context *context, const size_t pos, std::vector<ir::AstNode *> &newNodes,
                                  ReplaceWithMultipleNodesOptions options)
{
    const auto posRange = CreateRange(pos);
    ReplaceRangeWithNodes(context, posRange, newNodes, std::move(options));
}

void ChangeTracker::InsertAtTopOfFile(es2panda_Context *context,
                                      const std::variant<ir::AstNode *, std::vector<ir::AstNode *>> &insert,
                                      bool blankLineBetween)
{
    auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(const_cast<es2panda_Context *>(context));
    const auto sourceFile = astContext->sourceFile;
    const auto sourceFileAst = GetAstFromContext(context);
    const size_t pos = GetInsertionPositionAtSourceFileTop(sourceFileAst);

    std::string prefix = (pos == 0) ? "" : newLineCharacter_;
    char currentChar = pos < sourceFile->source.size() ? sourceFile->source.at(pos) : '\0';
    std::string suffix = (IsLineBreak(currentChar) ? "" : newLineCharacter_);
    if (blankLineBetween) {
        suffix += newLineCharacter_;
    }
    if (std::holds_alternative<std::vector<ir::AstNode *>>(insert)) {
        ReplaceWithMultipleNodesOptions options;
        options.suffix = suffix;
        options.prefix = prefix;
        auto list = std::get<std::vector<ir::AstNode *>>(insert);
        InsertNodesAt(context, pos, list, options);
    } else {
        InsertNodeOptions options;
        options.suffix = suffix;
        options.prefix = prefix;
        InsertNodeAt(context, pos, std::get<ir::AstNode *>(insert), options);
    }
}

InsertNodeOptions ChangeTracker::GetOptionsForInsertNodeBefore(const ir::AstNode *before, const ir::AstNode *inserted,
                                                               const bool blankLineBetween)
{
    InsertNodeOptions options;
    if (before->IsStatement() || before->IsClassProperty()) {
        options.suffix = blankLineBetween ? newLineCharacter_ + newLineCharacter_ : newLineCharacter_;
    } else if (before->IsVariableDeclaration()) {
        options.suffix = ", ";
    } else if (before->IsTSTypeParameterDeclaration()) {
        options.suffix = (inserted->IsTSTypeParameterDeclaration() ? ", " : "");
    } else if ((before->IsStringLiteral() && before->Parent()->IsImportDeclaration()) || before->IsNamedType()) {
        options.suffix = ", ";
    } else if (before->IsImportSpecifier()) {
        options.suffix = "," + std::string(blankLineBetween ? newLineCharacter_ : " ");
    }
    return options;
}

std::vector<ir::AstNode *> ChangeTracker::GetMembersOrProperties(const ir::AstNode *node)
{
    std::vector<ir::AstNode *> membersOrProperties;
    if (node->IsObjectExpression()) {
        const auto &properties = node->AsObjectExpression()->Properties();
        membersOrProperties.reserve(properties.size());
        for (auto *property : properties) {
            membersOrProperties.emplace_back(property->AsExpression());
        }
    } else {
        node->FindChild([&membersOrProperties](ir::AstNode *n) {
            if (n->IsMemberExpression() || n->IsTSEnumMember()) {
                membersOrProperties.emplace_back(n);
            }
            return false;
        });
    }
    return membersOrProperties;
}

InsertNodeOptions ChangeTracker::GetInsertNodeAtStartInsertOptions(const ir::AstNode *node)
{
    const auto members = GetMembersOrProperties(node);
    const auto isEmpty = members.empty();
    const auto isFirstInsertion = classesWithNodesInsertedAtStart_.at(0).node == node;
    const auto insertTrailingComma = node->IsObjectExpression();
    const auto insertLeadingComma = node->IsObjectExpression() && isEmpty && !isFirstInsertion;
    InsertNodeOptions options;
    options.prefix = (insertLeadingComma ? "," : "") + newLineCharacter_;
    options.suffix = insertTrailingComma ? "," : (node->IsTSInterfaceDeclaration() && isEmpty ? ";" : "");
    options.delta = 0;
    return {options};
}

void ChangeTracker::InsertNodeAtStartWorker(es2panda_Context *context, const ir::AstNode *node,
                                            const ir::AstNode *newElement)
{
    if (node == nullptr || newElement == nullptr) {
        return;
    }
    if (node->IsClassDeclaration() || node->IsTSInterfaceDeclaration() || node->IsTSTypeLiteral() ||
        node->IsObjectExpression()) {
        if (newElement->IsClassProperty() || newElement->IsSpreadElement() || newElement->IsMethodDefinition() ||
            newElement->IsTSPropertySignature()) {
            const auto membersOrProperties = GetMembersOrProperties(node);
            const auto size = membersOrProperties.size();
            InsertNodeOptions options = GetInsertNodeAtStartInsertOptions(node);
            InsertNodeAt(context, membersOrProperties.at(size - 1)->End().index, newElement, options);
        }
    }
}

bool ChangeTracker::NeedSemicolonBetween(const ir::AstNode *a, const ir::AstNode *b)
{
    if (a == nullptr || b == nullptr) {
        return false;
    }
    return (a->IsTSPropertySignature() || a->IsTSParameterProperty()) && (b->IsClassProperty() || b->IsTyped()) &&
           (a->IsStatement() || !a->IsDeclare()) && (b->IsStatement() || !b->IsDeclare());
}

size_t ChangeTracker::InsertNodeAfterWorker(es2panda_Context *context, ir::AstNode *after, const ir::AstNode *newNode)
{
    if (NeedSemicolonBetween(after, newNode)) {
        auto astContext =
            reinterpret_cast<ark::es2panda::public_lib::Context *>(const_cast<es2panda_Context *>(context));
        const auto sourceFile = astContext->sourceFile;
        if (sourceFile->source.at(after->End().index - 1) != ':') {
            InsertNodeOptions options;
            ReplaceRange(context, CreateRange(after->End().index), newNode, options);
        }
    }

    ir::AstNode *anchor = ChangeTracker::ToEditBoundary(after);
    if (anchor == nullptr) {
        anchor = after;
    }
    return anchor->End().index;
}

InsertNodeOptions ChangeTracker::GetInsertNodeAfterOptionsWorker(const ir::AstNode *node)
{
    InsertNodeOptions options;
    switch (node->Type()) {
        case ark::es2panda::ir::AstNodeType::CLASS_DECLARATION:
        case ark::es2panda::ir::AstNodeType::STRUCT_DECLARATION:
        case ark::es2panda::ir::AstNodeType::TS_MODULE_DECLARATION:
            options.prefix = newLineCharacter_;
            options.suffix = newLineCharacter_;
            return options;
        case ark::es2panda::ir::AstNodeType::VARIABLE_DECLARATION:
        case ark::es2panda::ir::AstNodeType::STRING_LITERAL:
        case ark::es2panda::ir::AstNodeType::IDENTIFIER:
            options.prefix = ", ";
            return options;
        case ark::es2panda::ir::AstNodeType::PROPERTY:
            options.suffix = "," + newLineCharacter_;
            return options;
        case ark::es2panda::ir::AstNodeType::EXPORT_SPECIFIER:
            options.prefix = ", ";
            return options;
        case ark::es2panda::ir::AstNodeType::TS_TYPE_PARAMETER:
            return options;
        default:
            // Else we haven't handled this kind of node yet -- add it
            options.suffix = newLineCharacter_;
            return options;
    }
}
struct StartandEndOfNode {
    size_t start;
    size_t end;
};

StartandEndOfNode GetClassOrObjectBraceEnds(ir::AstNode *node)
{
    const auto open = node->FindChild([](ir::AstNode *) { return true; })->Start().index;
    const auto close = node->End().index;
    return StartandEndOfNode {open, close};
}

void ChangeTracker::FinishClassesWithNodesInsertedAtStart()
{
    for (const auto mapElem : classesWithNodesInsertedAtStart_) {
        StartandEndOfNode braceEnds = GetClassOrObjectBraceEnds(mapElem.second.node);
        const auto isEmpty = GetMembersOrProperties(mapElem.second.node).empty();
        const auto isSingleLine = mapElem.second.node->Start().line == mapElem.second.node->End().line;
        if (isEmpty && isSingleLine) {
            DeleteRange(mapElem.second.sourceFile, CreateRange(braceEnds.start, braceEnds.end - 1));
        }
        if (isSingleLine) {
            InsertText(mapElem.second.sourceFile, braceEnds.end - 1, newLineCharacter_);
        }
    }
}

void ChangeTracker::PushRaw(const SourceFile *sourceFile, const FileTextChanges &change)
{
    for (const auto &c : change.textChanges) {
        ChangeText changeText {
            sourceFile, {c.span.start, c.span.start + c.newText.length()}, ChangeKind::TEXT, c.newText};
        changes_.emplace_back(changeText);
    }
}

void ChangeTracker::DeleteRange(const SourceFile *sourceFile, TextRange range)
{
    RemoveNode removeNode = {
        sourceFile,
        range,
        ChangeKind::REMOVE,
    };
    changes_.emplace_back(removeNode);
}
void ChangeTracker::Delete(const SourceFile *sourceFile,
                           std::variant<const ir::AstNode *, const std::vector<const ir::AstNode *>> &node)
{
    if (std::holds_alternative<const std::vector<const ir::AstNode *>>(node)) {
        std::vector<const ir::AstNode *> constNodes;
        auto nodes = std::get<const std::vector<const ir::AstNode *>>(node);
        constNodes.reserve(nodes.size());
        for (auto n : nodes) {
            constNodes.emplace_back(n);
        }
        deletedNodes_.push_back({sourceFile, constNodes});
    } else {
        deletedNodes_.push_back({sourceFile, node});
    }
}

TextRange ChangeTracker::GetAdjustedRange(es2panda_Context * /*context*/, ir::AstNode *startNode, ir::AstNode *endNode)
{
    return MakeEditRange(startNode, endNode);
}

void ChangeTracker::DeleteNode(es2panda_Context *context, const SourceFile *sourceFile, ir::AstNode *node)
{
    const auto adjustedRange = GetAdjustedRange(context, node, node);
    DeleteRange(sourceFile, adjustedRange);
}

void ChangeTracker::DeleteNodeRange(es2panda_Context *context, ir::AstNode *startNode, ir::AstNode *endNode)
{
    auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    const auto range = MakeEditRange(startNode, endNode);
    DeleteRange(ctx->sourceFile, range);
}

void ChangeTracker::DeleteModifier(es2panda_Context *context, ir::AstNode *modifier)
{
    auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    const auto sourceFile = astContext->sourceFile;
    DeleteRange(sourceFile, {modifier->Start().index, modifier->End().index});  // skipTrivia method will ask
}

void ChangeTracker::DeleteNodeRangeExcludingEnd(es2panda_Context *context, ir::AstNode *startNode,
                                                ir::AstNode *afterEndNode)
{
    auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    const auto sourceFile = astContext->sourceFile;

    DeleteRange(sourceFile, GetAdjustedRange(context, startNode, afterEndNode));
}

void ChangeTracker::ReplaceRange(es2panda_Context *context, TextRange range, const ir::AstNode *newNode,
                                 InsertNodeOptions &options)
{
    auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    const auto sourceFile = astContext->sourceFile;
    ReplaceWithSingleNode replaceNode = {sourceFile, range, ChangeKind::REPLACEWITHSINGLENODE, newNode, options};
    changes_.emplace_back(replaceNode);
}

void ChangeTracker::ReplaceNode(es2panda_Context *context, ir::AstNode *oldNode, ir::AstNode *newNode,
                                ChangeNodeOptions options)
{
    const auto adjRange = GetAdjustedRange(context, oldNode, oldNode);
    InsertNodeOptions insertOptions;
    if (options.insertNodeOptions.has_value()) {
        insertOptions = *options.insertNodeOptions;
    }
    ReplaceRange(context, adjRange, newNode, insertOptions);
}

void ChangeTracker::ReplaceNodeRange(es2panda_Context *context, ir::AstNode *startNode, ir::AstNode *endNode,
                                     ir::AstNode *newNode)
{
    const auto adjRange = GetAdjustedRange(context, startNode, endNode);
    InsertNodeOptions options;
    ReplaceRange(context, adjRange, newNode, options);
}

void ChangeTracker::ReplaceNodeWithNodes(es2panda_Context *context, ir::AstNode *oldNode,
                                         std::vector<ir::AstNode *> &newNodes)
{
    const auto adjRange = GetAdjustedRange(context, oldNode, oldNode);
    ReplaceRangeWithNodes(context, adjRange, newNodes);
}

void ChangeTracker::ReplaceNodeWithText(es2panda_Context *context, ir::AstNode *oldNode, const std::string &text)
{
    auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    const auto sourceFile = astContext->sourceFile;
    const auto adjRange = GetAdjustedRange(context, oldNode, oldNode);
    ReplaceRangeWithText(sourceFile, adjRange, text);
}

void ChangeTracker::ReplaceRangeWithText(const SourceFile *sourceFile, TextRange range, const std::string &text)
{
    ChangeText change = {sourceFile, range, ChangeKind::TEXT, text};
    changes_.emplace_back(change);
}

void ChangeTracker::ReplaceNodeRangeWithNodes(es2panda_Context *context, ir::AstNode *startNode, ir::AstNode *endNode,
                                              std::vector<ir::AstNode *> &newNodes)
{
    ReplaceRangeWithNodes(context, GetAdjustedRange(context, startNode, endNode), newNodes);
}

TextRange ChangeTracker::CreateRange(size_t pos, size_t end)
{
    if (end == 0) {
        end = pos;
    }
    return {pos, end};
}

void ChangeTracker::ReplacePropertyAssignment(es2panda_Context *context, ir::AstNode *oldNode, ir::AstNode *newNode)
{
    const auto suffix = NextCommaToken(context, oldNode) != nullptr ? "" : ("," + newLineCharacter_);
    InsertNodeOptions insertOptions;
    insertOptions.suffix = suffix;
    ChangeNodeOptions options = {g_useNonAdjustedPositions, insertOptions};
    ReplaceNode(context, oldNode, newNode, options);
}

void ChangeTracker::ReplaceConstructorBody(es2panda_Context *context, ir::AstNode *ctr,
                                           const std::vector<ir::Statement *> &statements)
{
    if (!statements.empty()) {
        ChangeNodeOptions options = {};

        const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
        const auto newNode = impl->CreateBlockStatement(context, nullptr, 0);
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto **stmts =
            static_cast<es2panda_AstNode **>(impl->AllocMemory(context, statements.size(), sizeof(es2panda_AstNode *)));
        if (stmts == nullptr) {
            return;
        }
        for (size_t i = 0; i < statements.size(); ++i) {
            stmts[i] = reinterpret_cast<es2panda_AstNode *>(statements[i]);
        }
        impl->BlockStatementSetStatements(context, newNode, stmts, statements.size());
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

        ReplaceNode(context, ctr, reinterpret_cast<ir::AstNode *>(newNode), options);
    }
}

bool ChangeTracker::IsLineBreak(char ch)
{
    const auto lineFeed = '\n';
    const auto carriageReturn = '\r';
    return ch == lineFeed || ch == carriageReturn;
}
void ChangeTracker::InsertNodeAt(es2panda_Context *context, size_t pos, const ir::AstNode *newNode,
                                 InsertNodeOptions &options)
{
    ReplaceRange(context, CreateRange(pos), newNode, options);
}

size_t ChangeTracker::GetInsertionPositionAtSourceFileTop(ir::AstNode *sourceFileAst)
{
    const auto topOfFile = sourceFileAst->FindChild([](ir::AstNode *child) { return child->IsClassDeclaration(); });
    return topOfFile->Start().index;
}

void ChangeTracker::InsertNodeAtTopOfFile(es2panda_Context *context, ir::AstNode *newNode, bool blankLineBetween)
{
    InsertAtTopOfFile(context, newNode, blankLineBetween);
}

void ChangeTracker::InsertNodeBefore(es2panda_Context *context, ir::AstNode *before, ir::AstNode *newNode,
                                     bool blankLineBetween)
{
    InsertNodeOptions insertOptions = GetOptionsForInsertNodeBefore(before, newNode, blankLineBetween);
    ir::AstNode *anchor = ToEditBoundary(before);
    if (anchor == nullptr) {
        anchor = before;  // fallback
    }

    InsertNodeAt(context, anchor->Start().index, newNode, insertOptions);
}

void ChangeTracker::InsertModifierAt(es2panda_Context *context, const size_t pos, const ir::AstNode *modifier,
                                     InsertNodeOptions &options)
{
    InsertNodeAt(context, pos, modifier, options);
}

void ChangeTracker::InsertModifierBefore(es2panda_Context *context, const ir::AstNode *modifier, ir::AstNode *before)
{
    InsertNodeOptions options;
    options.suffix = " ";
    return InsertModifierAt(context, before->Start().index, modifier, options);
}

void ChangeTracker::InsertText(const SourceFile *sourceFile, size_t pos, const std::string &text)
{
    ReplaceRangeWithText(sourceFile, CreateRange(pos), text);
}

/** Prefer this over replacing a node with another that has a type annotation,
 * as it avoids reformatting the other parts of the node. */
bool ChangeTracker::TryInsertTypeAnnotation(es2panda_Context *context, ir::AstNode *node, ir::AstNode *type)
{
    InsertNodeOptions options;
    options.prefix = ": ";
    InsertNodeAt(context, node->End().index, type, options);
    return true;
}

void ChangeTracker::TryInsertThisTypeAnnotation(es2panda_Context *context, ir::AstNode *node, ir::AstNode *type)
{
    InsertNodeOptions options;
    options.prefix = "this: ";
    if (node->IsFunctionExpression()) {
        options.suffix = node->AsFunctionExpression()->Function()->Params().empty() ? ", " : "";
    }

    InsertNodeAt(context, node->Start().index, type, options);
}

void ChangeTracker::InsertTypeParameters(es2panda_Context *context, const ir::AstNode *node,
                                         std::vector<ir::AstNode *> &typeParameters)
{
    size_t start;
    if (node->IsFunctionDeclaration()) {
        start = node->AsFunctionExpression()->Function()->Params().at(0)->End().index;
    } else {
        start = node->End().index;
    }
    ReplaceWithMultipleNodesOptions options;
    options.prefix = "<";
    options.suffix = ">";
    options.joiner = ", ";
    InsertNodesAt(context, start, typeParameters, options);
}

void ChangeTracker::InsertNodeAtConstructorStart(es2panda_Context *context, ir::AstNode *ctr,
                                                 ir::Statement *newStatement)
{
    if (ctr == nullptr || newStatement == nullptr) {
        return;
    }
    if (!ctr->Parent()->IsConstructor()) {
        return;
    }

    std::vector<ir::Statement *> statements;
    ir::Statement *firstStatement = nullptr;

    ctr->FindChild([&](ir::AstNode *n) {
        if (n->IsStatement()) {
            if (firstStatement == nullptr) {
                firstStatement = n->AsStatement();
            }
            statements.push_back(n->AsStatement());
        }
        return false;
    });

    if (firstStatement == nullptr && statements.empty()) {
        std::vector<ir::Statement *> newStatements = {newStatement};
        newStatements.insert(newStatements.end(), statements.begin(), statements.end());
        ReplaceConstructorBody(context, ctr, newStatements);
    } else {
        // Insert the new statement before the first statement
        InsertNodeBefore(context, firstStatement, newStatement);
    }
}

void ChangeTracker::InsertNodeAfter(es2panda_Context *context, ir::AstNode *after, ir::AstNode *newNode)
{
    const auto endPosition = InsertNodeAfterWorker(context, after, newNode);
    InsertNodeOptions options = GetInsertNodeAfterOptions(after);
    InsertNodeAt(context, endPosition, newNode, options);
}

void ChangeTracker::InsertNodeAtConstructorEnd(es2panda_Context *context, ir::AstNode *ctr, ir::Statement *newStatement)
{
    if (!ctr->IsConstructor()) {
        return;
    }
    std::vector<ir::Statement *> statements;
    ctr->FindChild([&statements](ir::AstNode *n) {
        if (n->IsStatement()) {
            statements.push_back(n->AsStatement());
        }
        return false;
    });

    if (statements.empty()) {
        ReplaceConstructorBody(context, ctr, statements);
    } else {
        InsertNodeAfter(context, statements[statements.size() - 1], newStatement);
    }
}

void ChangeTracker::InsertNodeAtEndOfScope(es2panda_Context *context, ir::AstNode *scope, ir::AstNode *newNode)
{
    InsertNodeOptions options;
    options.prefix = newLineCharacter_;
    options.suffix = newLineCharacter_;
    InsertNodeAt(context, scope->End().index, newNode, options);
}

void ChangeTracker::InsertMemberAtStart(es2panda_Context *context, ir::AstNode *node, ir::AstNode *newElement)
{
    if (node == nullptr || newElement == nullptr) {
        return;
    }
    if (node->IsClassDeclaration() || node->IsTSInterfaceDeclaration() || node->IsTSTypeLiteral() ||
        node->IsObjectExpression()) {
        if (newElement->IsClassProperty() || newElement->IsTSPropertySignature() || newElement->IsTSMethodSignature()) {
            InsertNodeAtStartWorker(context, node, newElement);
        }
    }
}

void ChangeTracker::InsertNodeAtObjectStart(es2panda_Context *context, ir::ObjectExpression *obj,
                                            ir::AstNode *newElement)
{
    InsertNodeAtStartWorker(context, obj, newElement);
}

void ChangeTracker::InsertNodeAfterComma(es2panda_Context *context, ir::AstNode *after, ir::AstNode *newNode)
{
    const auto endPosition = InsertNodeAfterWorker(context, NextCommaToken(context, after), newNode);
    InsertNodeOptions options = GetInsertNodeAfterOptions(after);
    InsertNodeAt(context, endPosition, newNode, options);
}

void ChangeTracker::InsertNodeAtEndOfList(es2panda_Context *context, std::vector<const ir::AstNode *> &list,
                                          ir::AstNode *newNode)
{
    InsertNodeOptions options;
    options.prefix = ", ";
    const auto size = list.size();
    InsertNodeAt(context, size - 1, newNode, options);
}
InsertNodeOptions ChangeTracker::GetInsertNodeAfterOptions(const ir::AstNode *after)
{
    return GetInsertNodeAfterOptionsWorker(after);
}

void ChangeTracker::InsertNodesAfter(es2panda_Context *context, ir::AstNode *after, std::vector<ir::AstNode *> newNodes)
{
    const auto endPosition = InsertNodeAfterWorker(context, after, newNodes.at(0));
    InsertNodeOptions insertOptions = GetInsertNodeAfterOptions(after);
    ReplaceWithMultipleNodesOptions afterOptions;
    afterOptions.prefix = insertOptions.prefix;
    afterOptions.suffix = insertOptions.suffix;
    InsertNodesAt(context, endPosition, newNodes, afterOptions);
}

void ChangeTracker::InsertFirstParameter(es2panda_Context *context,
                                         std::vector<ir::TSTypeParameterDeclaration *> parameters,
                                         ir::TSTypeParameterDeclaration newParam)
{
    if (parameters.empty()) {
        InsertNodeBefore(context, parameters[0], newParam.AsTSTypeParameterDeclaration());
    } else {
        InsertNodeOptions insertOptions;
        InsertNodeAt(context, parameters.size(), newParam.AsTSTypeParameterDeclaration(), insertOptions);
    }
}

void ChangeTracker::InsertExportModifier(const SourceFile *sourceFile, ir::Statement *node)
{
    const std::basic_string exportModifier = "export ";
    InsertText(sourceFile, node->Start().index, exportModifier);
}

std::vector<ir::AstNode *> ChangeTracker::GetContainingList(ir::AstNode *node)
{
    std::vector<ir::AstNode *> containingList;
    node->Parent()->FindChild([&containingList](ir::AstNode *child) {
        if (child->IsObjectExpression() || child->IsObjectExpression()) {
            for (auto *property : child->AsObjectExpression()->Properties()) {
                containingList.push_back(property);
            }
            return true;
        }
        return false;
    });
    return containingList;
}

/**
 * This function should be used to insert nodes in lists when nodes don't carry
 * separators as the part of the node range, i.e. arguments in arguments lists,
 * parameters in parameter lists etc. Note that separators are part of the node
 * in statements and class elements.
 */

void ChangeTracker::InsertNodeInListAfterMultiLine(bool multilineList, es2panda_Context *context,
                                                   const SourceFile *sourceFile, size_t end, const ir::AstNode *newNode)
{
    if (multilineList) {
        InsertNodeOptions insertOptions;
        ReplaceRange(context, CreateRange(end), newNode, insertOptions);
        const int indentation = 4;
        size_t insertPos = 4;
        while (insertPos != end && IsLineBreak(sourceFile->source.at(insertPos - 1))) {
            insertPos--;
        }
        insertOptions.indentation = indentation;
        insertOptions.prefix = newLineCharacter_;
        ReplaceRange(context, CreateRange(insertPos), newNode, insertOptions);
    } else {
        InsertNodeOptions insertOptions;
        insertOptions.prefix = " ";
        ReplaceRange(context, CreateRange(end), newNode, insertOptions);
    }
}

void ChangeTracker::InsertNodeInListAfter(es2panda_Context *context, ir::AstNode *after, ir::AstNode *newNode,
                                          std::vector<ir::AstNode *> &containingList)
{
    std::vector<ir::AstNode *> containingListResult = GetContainingList(after);
    containingList = std::vector<ir::AstNode *>(containingListResult.begin(), containingListResult.end());
    if (containingList.empty()) {
        return;
    }
    size_t index = 0;
    for (size_t i = 0; i < containingList.size(); i++) {
        if (containingList[i] == after) {
            index = i;
            break;
        }
    }
    if (index == 0) {
        return;
    }
    const auto end = after->End().index;
    Initializer initializer = Initializer();
    auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto sourceFile = astContext->sourceFile;
    if (index != containingList.size() - 1) {
        const auto nextToken = sourceFile->source.at(after->End().index);
        if (nextToken != 0 && (nextToken == ',' || nextToken == ';' || nextToken == ':' || nextToken == '.')) {
            const auto nextNode = containingList[index + 1];
            const auto startPos = nextNode->Start().index;
            ReplaceWithMultipleNodesOptions options;
            options.suffix = std::string(1, nextToken);
            InsertNodesAt(context, startPos, containingList, options);
        } else {
            bool multilineList = false;
            if (containingList.size() > 1) {
                multilineList = containingList[index - 1]->Start().line != containingList[index]->Start().line;
            }
            InsertNodeInListAfterMultiLine(multilineList, context, sourceFile, end, newNode);
        }
    }
}

void ChangeTracker::InsertImportSpecifierAtIndex(es2panda_Context *context, ir::AstNode *importSpecifier,
                                                 std::vector<ir::AstNode *> &namedImports, size_t index)
{
    const auto prevSpecifier = namedImports.at(index - 1);
    if (prevSpecifier != nullptr) {
        InsertNodeInListAfter(context, prevSpecifier, nullptr, namedImports);
    } else {
        InsertNodeBefore(context, namedImports[0], importSpecifier,
                         namedImports[0]->Parent()->Start().index == namedImports[0]->Parent()->End().index);
    }
}

std::vector<FileTextChanges> ChangeTracker::GetTextChangesFromChanges(std::vector<Change> &changes)
{
    std::unordered_map<std::string, FileTextChanges> fileChangesMap;
    auto addChange = [&](const SourceFile *sourceFile, TextRange range, const std::string &newText) {
        const std::string filePath = std::string(sourceFile->filePath);
        TextChange c = {{range.pos, range.end - range.pos}, newText};

        auto &fileChange = fileChangesMap[filePath];
        if (fileChange.fileName.empty()) {
            fileChange.fileName = filePath;
        }
        fileChange.textChanges.push_back(c);
    };

    for (const auto &change : changes) {
        if (const auto *textChange = std::get_if<ChangeText>(&change)) {
            addChange(textChange->sourceFile, textChange->range, textChange->text);
        } else if (const auto *remove = std::get_if<RemoveNode>(&change)) {
            addChange(remove->sourceFile, remove->range, "");
        } else if (const auto *replace = std::get_if<ReplaceWithSingleNode>(&change)) {
            addChange(replace->sourceFile, replace->range, replace->node->DumpEtsSrc());
        }
    }

    std::vector<FileTextChanges> fileTextChanges;
    fileTextChanges.reserve(fileChangesMap.size());
    for (auto &pair : fileChangesMap) {
        fileTextChanges.push_back(std::move(pair.second));
    }

    return fileTextChanges;
}

/**
 * Note: after calling this, the TextChanges object must be discarded!
 * @param validate only for tests
 *    The reason we must validate as part of this method is that
 * `getNonFormattedText` changes the node's positions, so we can only call this
 * once and can't get the non-formatted text separately.
 */
std::vector<FileTextChanges> ChangeTracker::GetChanges()  // should add ValidateNonFormattedText
{
    FinishClassesWithNodesInsertedAtStart();
    auto textChangesList = GetTextChangesFromChanges(changes_);
    return textChangesList;
}

void ChangeTracker::CreateNewFile(SourceFile *oldFile, const std::string &fileName,
                                  std::vector<const ir::Statement *> &statements)
{
    NewFile newFile;
    newFile.oldFile = oldFile;
    newFile.fileName = fileName;
    newFile.statements = statements;
    newFiles_.push_back(newFile);
}
}  // namespace ark::es2panda::lsp