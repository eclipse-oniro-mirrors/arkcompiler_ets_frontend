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

#ifndef ES2PANDA_LSP_REFACTORS_EXTRACT_SYMBOL_INTERNAL_H
#define ES2PANDA_LSP_REFACTORS_EXTRACT_SYMBOL_INTERNAL_H

#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "checker/ETSchecker.h"
#include "lsp/include/refactors/extract_symbol.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/types.h"
#include "public/public.h"
#include "services/text_change/change_tracker.h"
#include "util/ustring.h"

namespace ark::es2panda::lsp {

struct VariableBindingInfo {
    ir::VariableDeclaration *declaration {nullptr};
    ir::VariableDeclarator *declarator {nullptr};
    ir::Identifier *identifier {nullptr};
    ir::Expression *initializer {nullptr};
};

struct HelperPieces {
    bool insertHelper {false};
    size_t insertPos {0};
    std::string helperText;
    TextRange replaceRange {};
    std::string replacementText;
};

struct FunctionIOInfo {
    std::vector<std::string> paramDecls;
    std::vector<std::string> callArgs;
    std::optional<std::string> returnVar;
    bool hasReturnStatement {false};
    std::string returnVarTypeAnnotation;
};

struct FunctionBodyOptions {
    std::string newLine;
    std::string indent;
    bool addLeadingNewLine {false};
    bool returnEachLine {false};
    std::optional<std::string> returnVar;
    size_t trimIndent {0};
    size_t indentSize {FormatCodeSettings().GetIndentSize()};
};

struct FunctionTextBuildOptions {
    const FunctionIOInfo *ioInfo {nullptr};
    const std::vector<std::string> *capturedParams {nullptr};
    bool returnExtractedExpressionResult {false};
    std::string returnTypeAnnotation;
    const std::unordered_set<std::string> *protectedValueNames {nullptr};
};

struct FunctionCallReplacementInputs {
    const std::string &functionText;
    const std::vector<std::string> &callArgs;
    const std::optional<std::string> &returnVar;
    bool needsStatement {false};
    bool returnCallResult {false};
    bool awaitCallResult {false};
};

struct PlaceholderBuildInfo {
    bool globalConstantDeclShaped {false};
    bool globalConstantInitializerSelection {false};
    std::string placeholder;
};
std::string BuildExtractionDeclaration(const RefactorContext &context, ir::AstNode *extractedText,
                                       const std::string &actionName, std::string_view uniqueVarName);
PlaceholderBuildInfo BuildExtractionPlaceholder(const RefactorContext &context, public_lib::Context *ctx,
                                                ir::AstNode *extractedText, const std::string &actionName);

struct ExtractedVariableTypeAnnotationState {
    const RefactorContext &context;
    public_lib::Context *ctx;
    ir::AstNode *extractedText;
    const std::string &actionName;
    const PlaceholderBuildInfo &placeholderInfo;
    TextRange trimmed;
    ir::AstNode *coveringExpr;
    ir::AstNode *exactSelectionExpr;
    ir::AstNode *selectionExpr;
    bool isVariableExtraction;
    bool isConstantEnclose;
    bool isGlobalConstant;
    bool isArrowSelectionText;
};

struct ValueExtractionRenameLocInputs {
    const std::string &actionName;
    std::string_view sourceText;
    ir::AstNode *extractedText {nullptr};
    const std::vector<FileTextChanges> &edits;
    ir::AstNode *exprStmt {nullptr};
    size_t insertPos {0};
    const std::string &generatedText;
    const std::string &uniqueVarName;
    const std::string &implicitPrefix;
};

struct ScopeContext {
    bool hasEncloseScope {false};
    bool hasClassScope {false};
    std::string classScopeName;
    std::vector<std::string> namespaceScopeNames;
};

constexpr char LINE_FEED = '\n';
constexpr char CARRIAGE_RETURN = '\r';
constexpr char SPACE_CHAR = ' ';
constexpr char TAB_CHAR = '\t';
constexpr size_t CRLF_LENGTH = 2;
constexpr size_t K_TYPE_PARAM_DELIMITER_PAIR_LENGTH = 2;
constexpr size_t K_HELPER_RESERVE_PADDING = 96;
#ifndef _WIN32
inline constexpr std::string_view WINDOWS_LINE_BREAK = "\r\n";
#endif
#ifdef _WIN32
constexpr std::string_view WINDOWS_LINE_BREAK = "\r\n";
#endif

inline bool IsLineBreakChar(char ch)
{
    return ch == LINE_FEED || ch == CARRIAGE_RETURN;
}

inline bool IsIndentChar(char ch)
{
    return ch == SPACE_CHAR || ch == TAB_CHAR;
}

bool ResolveVariableBinding(ir::AstNode *node, VariableBindingInfo &out);
std::pair<size_t, size_t> ComputeLineIndent(util::StringView source, size_t pos);
TextRange GetTrimmedSelectionSpan(const RefactorContext &context);
bool HasImportDeclarationOverlap(const RefactorContext &context, TextRange range);
std::string GetNodeText(public_lib::Context *ctx, const ir::AstNode *node);
bool IsContainedInRange(const ir::AstNode *node, TextRange span);
ir::ScriptFunction *FindScriptFunction(ir::AstNode *node);
bool IsNamespaceContext(ir::AstNode *node);
std::string GetInsertionIndent(public_lib::Context *ctx, size_t insertPos);
std::string JoinWithComma(const std::vector<std::string> &items);
ir::ClassDefinition *FindEnclosingClassDefinition(ir::AstNode *node);
ir::ClassDefinition *FindNamespaceScopeByDepth(ir::AstNode *node, size_t namespaceDepth);
std::optional<std::string> ExtractVariableDeclaredTypeFromInitializer(std::string_view source, size_t initializerStart);
std::optional<std::string> ExtractClassPropertyDeclaredTypeFromInitializer(std::string_view source,
                                                                           size_t initializerStart);
std::string GenerateUniqueFuncName(const RefactorContext &context, const std::string &baseName,
                                   const std::string &actionName);
bool IsIdentifierContinuation(char ch);
bool ContainsIdentifierToken(std::string_view text, std::string_view token);

inline constexpr std::string_view EXTRACT_FUNCTION_NAMESPACE_ACTION_PREFIX = "extract_function_scope_ns_";
inline constexpr std::string_view EXTRACT_CONSTANT_NAMESPACE_ACTION_PREFIX = "extract_constant_scope_ns_";

std::vector<ir::ClassDefinition *> CollectEnclosingNamespaceScopes(ir::AstNode *node);
std::string IdentifierNameMutf8(const ir::Identifier *ident);
bool IsLineStartAtPosition(std::string_view source, size_t pos);
size_t NormalizeInsertPos(std::string_view source, size_t pos);
bool IsSyntheticScriptFunctionUnderGlobalClass(const ir::AstNode *node);
bool IsNamespaceModuleParent(const ir::AstNode *node);
bool IsProgramParent(const ir::AstNode *node);
bool HasSelectionNewline(const RefactorContext &context, std::string_view source);
bool IsClassContext(ir::AstNode *node);
bool IsSwitchCaseTestSelection(ir::AstNode *node, TextRange selection);
ir::AstNode *FindWholeVariableDeclarationSelectionNode(const RefactorContext &context, TextRange selection);
ir::AstNode *ResolveInitializerExpressionFromDeclarationSelection(const RefactorContext &context, TextRange selection);
ir::AstNode *ResolveInitializerExpressionContainingSelection(const RefactorContext &context, TextRange selection);
ir::AstNode *FindStatementOverlappingSelection(public_lib::Context *ctx, TextRange span);
bool IsImportSelectionNode(const ir::AstNode *node);
TextRange GetCallPositionOfExtraction(const RefactorContext &context);
bool HasUnexportedNamespaceInterfaceDependencyInSelection(const RefactorContext &context, TextRange range);
bool HasNamespacePrivateTypeAnnotationDependencyForExpression(const RefactorContext &context, TextRange range);
bool HasNamespacePrivateSymbolDependencyForGlobalExtraction(const RefactorContext &context, TextRange range);
bool HasLocalValueDependencyInSelection(const RefactorContext &context, TextRange range);
bool HasExternalLocalWriteDependencyInSelection(const RefactorContext &context, TextRange range);
bool HasUseStaticDirective(std::string_view source);
bool IsActionNameOrKind(std::string_view actionName, const RefactorActionView &action);
bool IsVariableExtractionAction(const std::string &actionName);
bool IsNamespaceAction(std::string_view actionName, std::string_view encloseName, std::string_view prefix);
bool IsConstantExtractionAction(const std::string &actionName);
std::optional<size_t> GetNamespaceActionDepth(std::string_view actionName, std::string_view encloseName,
                                              std::string_view prefix);
bool IsConstantExtractionInClassAction(const std::string &actionName);
std::string FormatDeclarationForInsert(public_lib::Context *ctx, size_t insertPos, std::string declaration);
std::string GetIndentAtPosition(public_lib::Context *ctx, size_t pos);
std::optional<size_t> FindVariableDeclKeywordStart(std::string_view source, size_t nodeStart);
bool IsLineBreak(char ch);
size_t FindLineStart(std::string_view source, size_t pos);
size_t FindInsertionPosBeforeTightLeadingComment(std::string_view source, size_t declarationStart);
void GetLineBounds(std::string_view source, size_t pos, size_t &lineStart, size_t &lineEnd);
bool IsBlankLine(std::string_view source, size_t lineStart, size_t lineEnd);
ir::AstNode *ResolveScopeDepthProbeNode(const RefactorContext &context, size_t pos);
size_t CountIndentScopeDepth(const ir::AstNode *node);
bool HasNewlineInRange(std::string_view source, TextRange range);
bool IsMemberPropertyIdentifier(const ir::Identifier *ident);
bool IsDeclarationIdentifier(const ir::Identifier *ident);
bool IsMultiDecl(ir::AstNode *node, public_lib::Context *context);
bool IsNodeInScope(ir::AstNode *node);
std::string TrimAsciiWhitespace(std::string_view s);

std::string QualifyTypeReferencesForGlobalExtractedBody(public_lib::Context *ctx, TextRange range, std::string bodyText,
                                                        bool qualifyValueRefs,
                                                        const std::unordered_set<std::string> *protectedValueNames);
std::string InferFromConsumerTypeAnnotation(const RefactorContext &context, public_lib::Context *ctx,
                                            ir::AstNode *node);
std::string InferExtractedReturnTypeAnnotationImpl(const RefactorContext &context, ir::AstNode *extractedNode);
std::string NormalizeReturnTypeAnnotation(std::string annotation);
std::string InferReturnTypeAnnotationFromSelectionFallback(const RefactorContext &context, public_lib::Context *ctx,
                                                           TextRange extractionPos);
void EnsureTrailingSemicolon(std::string &text);
std::string InferTypeFromChecker(checker::ETSChecker *checker, ir::AstNode *node);
ir::TypeNode *TypeAnnoFromDeclaratorId(ir::Expression *idExpr);
std::string ResolveVariableTypeAnnotation(public_lib::Context *ctx, const RefactorContext &context,
                                          ir::AstNode *extractedText);
ir::AstNode *UnwrapExpressionStatement(ir::AstNode *node);
std::string TryResolveCallSelectionTypeAnnotation(const ExtractedVariableTypeAnnotationState &state,
                                                  bool &isCallSelection);
ir::AstNode *FindExactSelectionExpression(const RefactorContext &context, TextRange selection);
ir::VariableDeclarator *FindContainingDeclaratorByRange(const RefactorContext &context, TextRange selection);
ir::AstNode *ResolveExpressionCoveringRange(const RefactorContext &context, TextRange initRange);
std::string ResolveTypeAnnotationFromContainingDeclarator(const RefactorContext &context, TextRange selection,
                                                          public_lib::Context *ctx);
std::string ResolveExtractedVariableTypeAnnotationFromSemanticNode(const ExtractedVariableTypeAnnotationState &state,
                                                                   std::string extractedVarTypeAnnotation);
bool IsObjectLiteralInitializerExtraction(const ir::AstNode *node);
bool IsArrowFunctionSelection(const RefactorContext &context);
std::optional<TextRange> ResolveInitializerRhsRange(const RefactorContext &context, TextRange hint);
std::string GetConstantString(std::string_view src, ir::AstNode *extractedText);
std::pair<std::string, bool> BuildClassConstantPrefix(const std::string &varName, ir::AstNode *startedNode,
                                                      const std::string &typeAnnotation);
std::string BuildMultiDeclPrefix(const std::string &varName);
std::string BuildStandardDeclPrefix(const std::string &varName, bool isConstantExtraction,
                                    const std::string &typeAnnotation);
bool IsObjectLiteralConstantExtraction(const std::string &actionName, const ir::AstNode *extractedText);
std::string ResolveExtractedVariableTypeAnnotation(const RefactorContext &context, public_lib::Context *ctx,
                                                   ir::AstNode *extractedText, const std::string &actionName,
                                                   const PlaceholderBuildInfo &placeholderInfo);
std::string GenerateInlineEdits(const RefactorContext &context, ir::AstNode *&extractedText,
                                const std::string &actionName, const std::string &uniqueVarName);
size_t ResolveValueExtractionRenameLoc(const ValueExtractionRenameLocInputs &inputs);
size_t ComputeRenameLocFromEdits(const std::vector<FileTextChanges> &edits, size_t renameLoc);
std::optional<size_t> ComputeFunctionCallRenameLocFromEdits(const std::vector<FileTextChanges> &edits,
                                                            TextRange extractionPos);
bool HasSourceNewlineInRange(public_lib::Context *ctx, TextRange range);
void MaybeIncludeTrailingSemicolonForReturnSelection(public_lib::Context *ctx, ir::AstNode *extractedNode,
                                                     TextRange &range);
size_t NormalizeFunctionInsertPos(const RefactorContext &context, public_lib::Context *ctx,
                                  const std::string &actionName);
std::vector<std::string> BuildFunctionCallArgs(const FunctionExtraction &candidate, bool treatAsStatements,
                                               const FunctionIOInfo &ioInfo,
                                               const std::vector<std::string> *capturedArgs);
size_t ResolveGlobalConstantInsertionPosFromSource(std::string_view source, size_t limit, size_t fallbackPos,
                                                   size_t globalFallbackPos);
std::string RemoveMarkerComments(std::string_view text);
std::string TrimSemicolonSeparatedText(std::string_view text);
bool ShouldDeleteWholeExprStmtBySpan(const SourceFile *src, TextRange selectionSpan, ir::AstNode *stmt);
std::string BuildDeclarationCoreFromInsertedText(std::string_view insertedText);
bool TryBuildCommentAdjacentReplacement(std::string_view source, TextRange extractedRange,
                                        const std::string &declarationCore, TextRange &replaceRange,
                                        std::string &replacementText);
ir::AstNode *FindEnclosingExprStmtBySpan(ir::AstNode *node, TextRange span);
ir::AstNode *TryResolveExprStmtByNode(const RefactorContext &context, const SourceFile *src, ir::AstNode *node);
ir::AstNode *ResolveExprStmtForValueExtraction(const RefactorContext &context, ir::AstNode *extractedText,
                                               const std::string &actionName, ir::AstNode *exprStmt,
                                               const SourceFile *src);
struct TryApplyExprStmtExtractionEditInputs {
    const SourceFile *src {nullptr};
    ir::AstNode *exprStmt {nullptr};
    const std::pair<size_t, std::string> &insertionData;
    TextRange extractedRange {};
    const std::string &actionName;
};
bool TryApplyExprStmtExtractionEdit(ChangeTracker &tracker, const TryApplyExprStmtExtractionEditInputs &inputs);
struct FunctionExtractionTextChangeInputs {
    const RefactorContext &context;
    const std::string &actionName;
    const std::string &functionText;
    size_t insertPos {0};
    TextRange extractionPos {};
    const std::string &funcCallText;
};
std::vector<FileTextChanges> BuildFunctionExtractionTextChanges(const FunctionExtractionTextChangeInputs &inputs);
bool RangeEndsWithStatementSemicolon(std::string_view source, TextRange range);
size_t ComputeFunctionRenameLoc(const std::vector<FileTextChanges> &edits, TextRange extractionPos);
bool TryBuildTopLevelDeclarationLeadingFunctionExtraction(const RefactorContext &context, public_lib::Context *ctx,
                                                          TextRange extractionPos, RefactorEditInfo &out);
bool MatchesSelectionWithOptionalSemicolon(public_lib::Context *ctx, TextRange extractionPos, const ir::AstNode *node);
bool TryRewriteExtractionToWholeDeclarationInitializer(const RefactorContext &context, public_lib::Context *ctx,
                                                       TextRange &extractionPos, ir::AstNode *&extractedNode);
bool TryRewriteExtractionToTextInitializer(const RefactorContext &context, public_lib::Context *ctx,
                                           TextRange &extractionPos, ir::AstNode *&extractedNode);
void NormalizeWholeDeclarationExtraction(public_lib::Context *ctx, TextRange &extractionPos,
                                         ir::AstNode *&extractedNode);
bool IsDeclarationLeadingSelection(const RefactorContext &context, const std::string &actionName);
std::optional<RefactorEditInfo> TryBuildEarlyGlobalFunctionExtraction(const RefactorContext &context,
                                                                      public_lib::Context *ctx,
                                                                      TextRange extractionPos);
std::string ResolveReturnTypeAnnotationForBinding(const RefactorContext &context, const VariableBindingInfo &binding,
                                                  bool allowCheckerInference, bool normalizePrimitiveTypes);
FunctionIOInfo AnalyzeFunctionIO(const RefactorContext &context, TextRange range, bool includeNonGlobal,
                                 ir::AstNode *insertAnchorNode, bool preferQualifiedNamespaceRefs);
ir::AstNode *ResolveNodeForSelection(const RefactorContext &context, public_lib::Context *ctx, bool selectionHasNewline,
                                     TextRange normalizedSpan);
std::string GenerateUniqueExtractedVarName(const RefactorContext &context, const std::string &actionName);
size_t DetermineGlobalInsertPos(public_lib::Context *ctx);
size_t ResolveIndentSize(const RefactorContext &context);
size_t ExtendToLineEnd(util::StringView source, size_t index);
void TrimTrailingNewlines(std::string &text);
TextRange GetVarAndFunctionPosToWriteNode(const RefactorContext &context, const std::string &actionName);
ir::AstNode *FindExtractedVals(const RefactorContext &context);
ir::AstNode *FindExtractedFunction(const RefactorContext &context);
std::vector<FunctionExtraction> GetPossibleFunctionExtractions(const RefactorContext &context);
std::string GetParamsText(const FunctionExtraction &candidate, const std::vector<ir::Identifier *> &functionParams);
std::vector<ir::Identifier *> CollectFunctionParams(ir::AstNode *ast, size_t start, size_t end, bool &needParams);
std::string BuildFunctionBody(const std::string &body, const FunctionBodyOptions &options);
std::string BuildFunctionText(const FunctionExtraction &candidate, const RefactorContext &context,
                              const std::string &actionName, TextRange extractionRange,
                              const FunctionTextBuildOptions &options);
std::string ReplaceWithFunctionCall(const FunctionCallReplacementInputs &inputs);
bool TryBuildHelperExtraction(const RefactorContext &context, ir::AstNode *extractedNode, const std::string &actionName,
                              RefactorEditInfo &out);
bool BuildGlobalPiecesFromDeclarationSelection(const RefactorContext &context, const VariableBindingInfo &binding,
                                               TextRange selectionSpan, HelperPieces &out);
size_t FindRenameIndex(HelperPieces &pieces);
ir::AstNode *FindTouchingTokenNearSpan(const RefactorContext &context);
bool IsInsideFinallyBlock(ir::AstNode *node);
std::vector<RefactorAction> FindAvailableRefactors(const RefactorContext &context);
RefactorEditInfo GetRefactorEditsToExtractVals(const RefactorContext &context, ir::AstNode *extractedText,
                                               const std::string &actionName);
RefactorEditInfo GetRefactorEditsToExtractFunction(const RefactorContext &context, const std::string &actionName);

}  // namespace ark::es2panda::lsp

#endif
