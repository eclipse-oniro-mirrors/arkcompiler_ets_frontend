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

#ifndef ES2PANDA_LSP_CHANGE_TRACKER_H
#define ES2PANDA_LSP_CHANGE_TRACKER_H
#include "es2panda.h"
#include "ir/astNode.h"
#include "lsp/include/api.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/formatting/formatting_context.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/user_preferences.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
#include <cstddef>
#include <ir/typeNode.h>
#include "text_change_context.h"

namespace ark::es2panda::lsp {

enum class LeadingTriviaOption { EXCLUDE, INCLUDEALL, STARTLINE };

enum class TrailingTriviaOption { EXCLUDE, EXCLUDEWHITESPACE, INCLUDE };

struct ConfigurableStart {
    std::optional<LeadingTriviaOption> leadingTriviaOption;
};

struct ConfigurableEnd {
    std::optional<TrailingTriviaOption> trailingTriviaOption;
};

struct ConfigurableStartEnd {
    std::optional<LeadingTriviaOption> leadingTriviaOption;
    std::optional<TrailingTriviaOption> trailingTriviaOption;
};

struct InsertNodeOptions {
    std::optional<std::string> prefix;
    std::optional<std::string> suffix;
    std::optional<int> indentation;
    std::optional<int> delta;
};

struct ReplaceWithMultipleNodesOptions : InsertNodeOptions {
    std::optional<std::string> joiner;
};

struct ChangeNodeOptions {
    std::optional<ConfigurableStartEnd> configurableStartEnd;
    std::optional<InsertNodeOptions> insertNodeOptions;
};

enum class ChangeKind { REMOVE, REPLACEWITHSINGLENODE, REPLACEWITHMULTIPLENODES, TEXT };

struct ChangeText {
    const SourceFile *sourceFile;
    TextRange range;
    ChangeKind kind = ChangeKind::TEXT;
    std::string text;
};

struct ReplaceWithSingleNode {
    const SourceFile *sourceFile;
    TextRange range;
    ChangeKind kind = ChangeKind::REPLACEWITHSINGLENODE;
    const ir::AstNode *node;
    std::optional<InsertNodeOptions> options;
};

struct ReplaceWithMultipleNodes {
    const SourceFile *sourceFile;
    TextRange range;
    ChangeKind kind = ChangeKind::REPLACEWITHMULTIPLENODES;
    std::vector<ir::AstNode *> nodes;
    std::optional<ReplaceWithMultipleNodesOptions> options;
};
struct RemoveNode {
    const SourceFile *sourceFile;
    TextRange range;
    ChangeKind kind = ChangeKind::REMOVE;
};

struct NewFile {
    std::optional<SourceFile *> oldFile;
    std::string fileName;
    std::vector<const ir::Statement *> statements;
};

using Change = std::variant<ReplaceWithSingleNode, ReplaceWithMultipleNodes, RemoveNode, ChangeText>;

struct DeletedNode {
    const SourceFile *sourceFile;
    const std::variant<const ir::AstNode *, const std::vector<const ir::AstNode *>> &node;
};

struct NewFileStruct {
    std::optional<SourceFile *> oldFile;
    std::string fileName;
    std::vector<std::variant<ir::Statement *, int>> statements;
};

using ValidateNonFormattedText = std::function<void(ir::AstNode *node, const std::string &text)>;

struct ClassInsertInfo {
    ir::AstNode *node;
    SourceFile *sourceFile;
};

class ChangeTracker {
private:
    FormatContext formatContext_;
    std::string newLineCharacter_;

    ChangeTracker(FormatContext &formatContext, std::string newLineCharacter)
        : formatContext_(formatContext), newLineCharacter_(std::move(newLineCharacter)) {};

    ir::AstNode *GetAstFromContext(const es2panda_Context *context);
    size_t GetStartPositionOfLine(size_t line, const es2panda_Context *context);
    bool RangeContainsPosition(TextRange r, size_t pos);
    void ReplaceRangeWithNodes(es2panda_Context *context, const TextRange range, std::vector<ir::AstNode *> &newNodes,
                               ReplaceWithMultipleNodesOptions options = {});
    ir::AstNode *NextCommaToken(es2panda_Context *context, const ir::AstNode *node);
    void InsertNodesAt(es2panda_Context *context, size_t pos, std::vector<ir::AstNode *> &newNodes,
                       ReplaceWithMultipleNodesOptions options = {});
    void InsertAtTopOfFile(es2panda_Context *context,
                           const std::variant<ir::AstNode *, std::vector<ir::AstNode *>> &insert,
                           bool blankLineBetween);
    InsertNodeOptions GetOptionsForInsertNodeBefore(const ir::AstNode *before, const ir::AstNode *inserted,
                                                    bool blankLineBetween);
    std::vector<ir::AstNode *> GetMembersOrProperties(const ir::AstNode *node);
    InsertNodeOptions GetInsertNodeAtStartInsertOptions(const ir::AstNode *node);
    void InsertNodeAtStartWorker(es2panda_Context *context, const ir::AstNode *node, const ir::AstNode *newElement);
    bool NeedSemicolonBetween(const ir::AstNode *a, const ir::AstNode *b);
    size_t InsertNodeAfterWorker(es2panda_Context *context, ir::AstNode *after, const ir::AstNode *newNode);
    InsertNodeOptions GetInsertNodeAfterOptionsWorker(const ir::AstNode *node);
    void InsertNodeInListAfterMultiLine(bool multilineList, es2panda_Context *context, const SourceFile *sourceFile,
                                        size_t end, const ir::AstNode *newNode);
    std::vector<FileTextChanges> GetTextChangesFromChanges(std::vector<Change> &changes);
    std::vector<DeletedNode> deletedNodes_;
    std::vector<Change> changes_;
    std::vector<NewFile> newFiles_;
    std::map<int, ClassInsertInfo> classesWithNodesInsertedAtStart_;

public:
    std::vector<DeletedNode> GetDeletedNodesList() const
    {
        return deletedNodes_;
    }
    std::vector<Change> GetChangeList() const
    {
        return changes_;
    }
    std::vector<NewFile> GetNewFilesList() const
    {
        return newFiles_;
    }
    std::map<int, ClassInsertInfo> GetClassesWithNodesInsertedAtStartList() const
    {
        return classesWithNodesInsertedAtStart_;
    }

    static ChangeTracker FromContext(TextChangesContext &context);
    static std::vector<FileTextChanges> With(TextChangesContext &context,
                                             const std::function<void(ChangeTracker &)> &cb);

    void PushRaw(const SourceFile *sourceFile, const FileTextChanges &change);
    void DeleteRange(const SourceFile *sourceFile, TextRange range);
    std::vector<FileTextChanges> GetChanges();
    void Delete(const SourceFile *sourceFile,
                std::variant<const ir::AstNode *, const std::vector<const ir::AstNode *>> &node);
    TextRange GetAdjustedRange(es2panda_Context *context, ir::AstNode *startNode, ir::AstNode *endNode);
    void DeleteNode(es2panda_Context *context, const SourceFile *sourceFile, ir::AstNode *node);

    void DeleteNodeRange(es2panda_Context *context, ir::AstNode *startNode, ir::AstNode *endNode);

    void DeleteModifier(es2panda_Context *context, ir::AstNode *modifier);
    void DeleteNodeRangeExcludingEnd(es2panda_Context *context, ir::AstNode *startNode, ir::AstNode *afterEndNode);
    void ReplaceRange(es2panda_Context *context, TextRange range, const ir::AstNode *newNode,
                      InsertNodeOptions &options);
    void ReplaceNode(es2panda_Context *context, ir::AstNode *oldNode, ir::AstNode *newNode, ChangeNodeOptions options);
    void ReplaceNodeRange(es2panda_Context *context, ir::AstNode *startNode, ir::AstNode *endNode,
                          ir::AstNode *newNode);
    void ReplaceNodeWithNodes(es2panda_Context *context, ir::AstNode *oldNode, std::vector<ir::AstNode *> &newNodes);
    void ReplaceNodeWithText(es2panda_Context *context, ir::AstNode *oldNode, const std::string &text);
    void ReplaceRangeWithText(const SourceFile *sourceFile, TextRange range, const std::string &text);
    void ReplaceNodeRangeWithNodes(es2panda_Context *context, ir::AstNode *startNode, ir::AstNode *endNode,
                                   std::vector<ir::AstNode *> &newNodes);
    TextRange CreateRange(size_t pos, size_t end = 0);
    void ReplacePropertyAssignment(es2panda_Context *context, ir::AstNode *oldNode, ir::AstNode *newNode);
    void InsertNodeAt(es2panda_Context *context, size_t pos, const ir::AstNode *newNode, InsertNodeOptions &options);
    bool IsLineBreak(char ch);
    void FinishClassesWithNodesInsertedAtStart();
    size_t GetInsertionPositionAtSourceFileTop(ir::AstNode *sourceFileAst);
    void InsertNodeAtTopOfFile(es2panda_Context *context, ir::AstNode *newNode, bool blankLineBetween);
    void InsertNodesAtTopOfFile(es2panda_Context *context, const std::vector<const ir::AstNode *> newNodes,
                                bool blankLineBetween);
    void InsertNodeBefore(es2panda_Context *context, ir::AstNode *before, ir::AstNode *newNode,
                          bool blankLineBetween = false);
    void InsertModifierAt(es2panda_Context *context, size_t pos, const ir::AstNode *modifier,
                          InsertNodeOptions &options);
    void InsertModifierBefore(es2panda_Context *context, const ir::AstNode *modifier, ir::AstNode *before);
    void InsertText(const SourceFile *sourceFile, size_t pos, const std::string &text);
    bool TryInsertTypeAnnotation(es2panda_Context *context, ir::AstNode *node, ir::AstNode *type);
    void TryInsertThisTypeAnnotation(es2panda_Context *context, ir::AstNode *node, ir::AstNode *type);
    void InsertTypeParameters(es2panda_Context *context, const ir::AstNode *node,
                              std::vector<ir::AstNode *> &typeParameters);
    void ReplaceConstructorBody(es2panda_Context *context, ir::AstNode *ctr,
                                const std::vector<ir::Statement *> &statements);
    void InsertNodeAtConstructorStart(es2panda_Context *context, ir::AstNode *ctr, ir::Statement *newStatement);
    void InsertNodeAfter(es2panda_Context *context, ir::AstNode *after, ir::AstNode *newNode);
    void InsertNodeAtConstructorEnd(es2panda_Context *context, ir::AstNode *ctr, ir::Statement *newStatement);
    void InsertNodeAtEndOfScope(es2panda_Context *context, ir::AstNode *scope, ir::AstNode *newNode);
    void InsertMemberAtStart(es2panda_Context *context, ir::AstNode *node, ir::AstNode *newElement);
    void InsertNodeAtObjectStart(es2panda_Context *context, ir::ObjectExpression *obj, ir::AstNode *newElement);
    void InsertNodeAfterComma(es2panda_Context *context, ir::AstNode *after, ir::AstNode *newNode);
    void InsertNodeAtEndOfList(es2panda_Context *context, std::vector<const ir::AstNode *> &list, ir::AstNode *newNode);
    InsertNodeOptions GetInsertNodeAfterOptions(const ir::AstNode *after);
    void InsertNodesAfter(es2panda_Context *context, ir::AstNode *after, std::vector<ir::AstNode *> newNodes);
    void InsertFirstParameter(es2panda_Context *context, std::vector<ir::TSTypeParameterDeclaration *> parameters,
                              ir::TSTypeParameterDeclaration newParam);
    void InsertExportModifier(const SourceFile *sourceFile, ir::Statement *node);
    std::vector<ir::AstNode *> GetContainingList(ir::AstNode *node);
    void InsertNodeInListAfter(es2panda_Context *context, ir::AstNode *after, ir::AstNode *newNode,
                               std::vector<ir::AstNode *> &containingList);
    void InsertImportSpecifierAtIndex(es2panda_Context *context, ir::AstNode *importSpecifier,
                                      std::vector<ir::AstNode *> &namedImports, size_t index);
    void CreateNewFile(SourceFile *oldFile, const std::string &fileName,
                       std::vector<const ir::Statement *> &statements);
    void RfindNearestKeyWordTextRange(const es2panda_Context *context, const size_t pos,
                                      const std::string_view &keywordStr, TextRange &range);
    static ir::AstNode *ToEditBoundary(ir::AstNode *node);
};

}  // namespace ark::es2panda::lsp

#endif