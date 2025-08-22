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

#ifndef ES2PANDA_LSP_INCLUDE_API_H
#define ES2PANDA_LSP_INCLUDE_API_H

// Switch off the linter for C header
// NOLINTBEGIN
//

#include <stddef.h>
#include <cstddef>
#include <string>
#include <variant>
#include <vector>
#include "ir/astNode.h"
#include "line_column_offset.h"
#include "public/es2panda_lib.h"
#include "cancellation_token.h"
#include "user_preferences.h"
#include "class_hierarchies.h"
#include "find_references.h"
#include "find_rename_locations.h"
#include "class_hierarchy_info.h"
#include "completions.h"
#include "refactors/refactor_types.h"
#include "applicable_refactors.h"
#include "rename.h"
#include "todo_comments.h"
#include "types.h"
#include "formatting/formatting_settings.h"
#include "user_preferences.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SafeDeleteLocation {
    std::string uri;
    size_t start;
    size_t length;
} SafeDeleteLocation;

typedef struct DefinitionInfo {
    DefinitionInfo() = default;
    DefinitionInfo(std::string f, size_t s, size_t l) : fileName(f), start(s), length(l) {}
    std::string fileName;
    size_t start;
    size_t length;
} DefinitionInfo;

typedef struct ReferenceInfo {
    ReferenceInfo() = default;
    ReferenceInfo(std::string f, size_t s, size_t l) : fileName(f), start(s), length(l) {}
    std::string fileName;
    size_t start;
    size_t length;
} ReferenceInfo;

typedef struct References {
    std::vector<ReferenceInfo> referenceInfos;
} References;

typedef struct Position {
    size_t line_;       // Line number
    size_t character_;  // Character position in the line

    Position(unsigned int line_num = 0, unsigned int character_pos = 0) : line_(line_num), character_(character_pos) {}
} Position;

typedef struct Range {
    Position start;  // Start position
    Position end;    // End position

    Range(Position start_pos = Position(), Position end_pos = Position()) : start(start_pos), end(end_pos) {}
} Range;

typedef struct Location {
    std::string uri_;  // The URI of the document
    Range range_;      // The range of the diagnostic in the document
    Location(std::string uri = "", const Range range = Range()) : uri_(uri), range_(range) {}
} Location;

enum class DiagnosticSeverity { Error = 1, Warning = 2, Information = 3, Hint = 4 };

enum class DiagnosticTag { Unnecessary = 1, Deprecated = 2 };

typedef struct CodeDescription {
    std::string href_;
    CodeDescription(std::string href = "") : href_(href) {}
} CodeDescription;

typedef struct DiagnosticRelatedInformation {
    Location location_;
    std::string message_;

    DiagnosticRelatedInformation(const Location location = Location(), const std::string message = "")
        : location_(location), message_(message)
    {
    }
} DiagnosticRelatedInformation;

typedef struct Diagnostic {
    Range range_;                                                   // The range at which the message applies.
    DiagnosticSeverity severity_;                                   // The diagnostic's severity.
    std::variant<int, std::string> code_;                           // The diagnostic's code.
    CodeDescription codeDescription_;                               // The error code description.
    std::string source_;                                            // The source of the diagnostic.
    std::string message_;                                           // The diagnostic's message.
    std::vector<DiagnosticTag> tags_;                               // Additional metadata about the diagnostic.
    std::vector<DiagnosticRelatedInformation> relatedInformation_;  // Related diagnostics.
    std::variant<int, std::string> data_;                           // Additional data.

    Diagnostic(const Range range, const std::vector<DiagnosticTag> tags,
               const std::vector<DiagnosticRelatedInformation> relatedInformation,
               DiagnosticSeverity severity = DiagnosticSeverity::Warning,
               const std::variant<int, std::string> code = 100, std::string message = "default message",
               const CodeDescription codeDescription = {}, std::string source = "default source",
               const std::variant<int, std::string> data = {})
        : range_(range),
          severity_(severity),
          code_(code),
          codeDescription_(codeDescription),
          source_(source),
          message_(message),
          tags_(tags),
          relatedInformation_(relatedInformation),
          data_(data)
    {
    }
} Diagnostic;

typedef struct DiagnosticReferences {
    std::vector<Diagnostic> diagnostic;
} DiagnosticReferences;

enum class CommentKind { SINGLE_LINE, MULTI_LINE };

typedef struct CommentRange {
    CommentRange() {}
    CommentRange(size_t p, size_t e, CommentKind k) : pos_(p), end_(e), kind_(k) {}
    size_t pos_;
    size_t end_;
    CommentKind kind_;
} CommentRange;

enum class AccessKind { READ, WRITE, READWRITE };

typedef struct ReferenceLocation {
    std::string uri;
    size_t start;  // Start position
    size_t end;    // End position
    bool isDefinition;
    AccessKind accessKind;
    bool isImport;
} ReferenceLocation;

typedef struct FileNodeInfo {
    std::string tokenName;
    std::string tokenId;
    FileNodeInfo(const std::string &token, const std::string &id) : tokenName(token), tokenId(id) {}
} FileNodeInfo;

typedef struct ReferenceLocationList {
    std::vector<ReferenceLocation> referenceLocation;
} ReferenceLocationList;

enum class HighlightSpanKind { NONE, DEFINITION, REFERENCE, WRITTEN_REFERENCE };

typedef struct HighlightSpan {
    std::string fileName_;
    bool isInString_;
    TextSpan textSpan_;
    TextSpan contextSpan_;
    HighlightSpanKind kind_;
    HighlightSpan(std::string fileName = "fileName", bool isInString = false, TextSpan textSpan = {0, 0},
                  TextSpan contextSpan = {0, 0}, HighlightSpanKind kind = HighlightSpanKind::NONE)
        : fileName_(fileName), isInString_(isInString), textSpan_(textSpan), contextSpan_(contextSpan), kind_(kind)
    {
    }
} HighlightSpan;

typedef struct DocumentHighlights {
    std::string fileName_;
    std::vector<HighlightSpan> highlightSpans_;
    DocumentHighlights(std::string fileName = "fileName", std::vector<HighlightSpan> highlightSpans = {})
        : fileName_(fileName), highlightSpans_(highlightSpans)
    {
    }
} DocumentHighlights;

struct FieldListProperty {
    std::string kind;
    std::optional<std::vector<std::string>> modifierKinds;
    std::string displayName;
    size_t start;
    size_t end;

    FieldListProperty(std::string k, std::optional<std::vector<std::string>> m, std::string d, size_t s, size_t e)
        : kind(std::move(k)), modifierKinds(std::move(m)), displayName(std::move(d)), start(s), end(e)
    {
    }
};

struct FieldsInfo {
    std::string name;
    std::vector<FieldListProperty> properties;
    bool operator<(const FieldsInfo &other) const
    {
        return name < other.name;
    }
    FieldsInfo() = default;
    FieldsInfo(const FieldsInfo &fi) : name(fi.name), properties(fi.properties) {}
};

struct LspClassPropertyInfo {
    FieldsInfo fieldsInfo;
    LspClassPropertyInfo(FieldsInfo f) : fieldsInfo(std::move(f)) {}
};

typedef struct DocumentHighlightsReferences {
    std::vector<DocumentHighlights> documentHighlights_;
} DocumentHighlightsReferences;

struct DocTagInfo {
private:
    std::string name_;
    std::string text_;

public:
    explicit DocTagInfo(std::string name = "", std::string text = "") : name_ {std::move(name)}, text_ {std::move(text)}
    {
    }

    std::string GetName() const
    {
        return name_;
    }
    std::string GetText() const
    {
        return text_;
    }

    bool operator==(const DocTagInfo &other) const
    {
        return name_ == other.name_ && text_ == other.text_;
    }
    bool operator!=(const DocTagInfo &other) const
    {
        return !(*this == other);
    }
};

struct QuickInfo {
private:
    std::string kind_;
    std::string kindModifiers_;
    TextSpan textSpan_;
    std::vector<SymbolDisplayPart> displayParts_;
    std::vector<SymbolDisplayPart> document_;
    std::vector<DocTagInfo> tags_;
    std::string fileName_;

public:
    explicit QuickInfo(std::string kind = "", std::string kindModifiers = "", TextSpan span = TextSpan(0, 0),
                       std::vector<SymbolDisplayPart> displayParts = {}, std::vector<SymbolDisplayPart> document = {},
                       std::vector<DocTagInfo> tags = {}, std::string fileName = "")
        : kind_ {std::move(kind)},
          kindModifiers_ {std::move(kindModifiers)},
          textSpan_ {span},
          displayParts_ {std::move(displayParts)},
          document_ {std::move(document)},
          tags_ {std::move(tags)},
          fileName_ {std::move(fileName)}
    {
    }

    std::string GetKind() const
    {
        return kind_;
    }
    std::string GetKindModifiers() const
    {
        return kindModifiers_;
    }
    TextSpan GetTextSpan() const
    {
        return textSpan_;
    }
    std::vector<SymbolDisplayPart> GetDisplayParts() const
    {
        return displayParts_;
    }
    std::vector<SymbolDisplayPart> GetDocument() const
    {
        return document_;
    }
    std::vector<DocTagInfo> GetTags() const
    {
        return tags_;
    }
    std::string GetFileName() const
    {
        return fileName_;
    }

    bool operator==(const QuickInfo &other) const
    {
        return kind_ == other.kind_ && kindModifiers_ == other.kindModifiers_ && textSpan_ == other.textSpan_ &&
               displayParts_ == other.displayParts_ && document_ == other.document_ && tags_ == other.tags_ &&
               fileName_ == other.fileName_;
    }
    bool operator!=(const QuickInfo &other) const
    {
        return !(*this == other);
    }
};

struct CompletionEntryDetails {
private:
    std::string name_;
    std::string kind_;
    std::string kindModifiers_;
    std::vector<SymbolDisplayPart> displayParts_;
    std::vector<SymbolDisplayPart> document_;
    std::vector<SymbolDisplayPart> source_;
    std::vector<SymbolDisplayPart> sourceDisplay_;
    std::string fileName_;

public:
    explicit CompletionEntryDetails(std::string name = "", std::string kind = "", std::string kindModifiers = "",
                                    std::vector<SymbolDisplayPart> displayParts = {},
                                    std::vector<SymbolDisplayPart> document = {},
                                    std::vector<SymbolDisplayPart> source = {},
                                    std::vector<SymbolDisplayPart> sourceDisplay = {}, std::string fileName = "")
        : name_ {std::move(name)},
          kind_ {std::move(kind)},
          kindModifiers_ {std::move(kindModifiers)},
          displayParts_ {std::move(displayParts)},
          document_ {std::move(document)},
          source_ {std::move(source)},
          sourceDisplay_ {std::move(sourceDisplay)},
          fileName_ {std::move(fileName)}
    {
    }

    std::string GetName() const
    {
        return name_;
    }
    std::string GetKind() const
    {
        return kind_;
    }
    std::string GetKindModifiers() const
    {
        return kindModifiers_;
    }
    std::vector<SymbolDisplayPart> GetDisplayParts() const
    {
        return displayParts_;
    }
    std::vector<SymbolDisplayPart> GetDocument() const
    {
        return document_;
    }
    std::vector<SymbolDisplayPart> GetSource() const
    {
        return source_;
    }
    std::vector<SymbolDisplayPart> GetSourceDisplay() const
    {
        return sourceDisplay_;
    }
    std::string GetFileName() const
    {
        return fileName_;
    }

    bool operator==(const CompletionEntryDetails &other) const
    {
        return name_ == other.name_ && kind_ == other.kind_ && kindModifiers_ == other.kindModifiers_ &&
               displayParts_ == other.displayParts_ && document_ == other.document_ && source_ == other.source_ &&
               sourceDisplay_ == other.sourceDisplay_ && fileName_ == other.fileName_;
    }
    bool operator!=(const CompletionEntryDetails &other) const
    {
        return !(*this == other);
    }
};

typedef struct FileDiagnostic {
    es2panda_AstNode *node;
    Diagnostic diagnostic;

    FileDiagnostic(es2panda_AstNode *n, const Diagnostic &diag, Position start, Position end)
        : node(n),
          diagnostic(Diagnostic(Range(start, end), diag.tags_, diag.relatedInformation_, diag.severity_, diag.code_,
                                diag.message_, diag.codeDescription_, diag.source_, diag.data_))
    {
    }
} FileDiagnostic;

typedef struct DeclInfo {
    std::string fileName;
    std::string fileText;
} DeclInfo;

enum class HierarchyType { OTHERS, INTERFACE, CLASS };

struct TypeHierarchies {
    TypeHierarchies() = default;
    TypeHierarchies(std::string f, std::string n, HierarchyType t, size_t p)
        : fileName(std::move(f)), name(std::move(n)), type(t), pos(p)
    {
    }
    bool operator==(const TypeHierarchies &other) const
    {
        return fileName == other.fileName && name == other.name && type == other.type && pos == other.pos;
    }
    bool operator!=(const TypeHierarchies &other) const
    {
        return !(*this == other);
    }
    bool operator<(const TypeHierarchies &other) const
    {
        return std::tie(fileName, name, type, pos) < std::tie(other.fileName, other.name, other.type, other.pos);
    }
    std::string fileName;
    std::string name;
    HierarchyType type = HierarchyType::OTHERS;
    size_t pos = 0;
    std::vector<TypeHierarchies> subOrSuper;
};

struct TypeHierarchiesInfo {
    TypeHierarchiesInfo() = default;
    TypeHierarchiesInfo(std::string f, std::string n, HierarchyType t, size_t p)
        : fileName(std::move(f)), name(std::move(n)), type(t), pos(p)
    {
    }
    std::string fileName;
    std::string name;
    HierarchyType type = HierarchyType::OTHERS;
    size_t pos = 0;
    TypeHierarchies superHierarchies;
    TypeHierarchies subHierarchies;
};

struct InstallPackageActionInfo {
    std::string type_;
    std::optional<std::string> file;
    std::optional<std::string> packageName;
};

struct CodeActionInfo {
    std::string description_;
    std::vector<FileTextChanges> changes_;
    std::vector<InstallPackageActionInfo> commands_;
};

struct CombinedCodeActionsInfo {
    std::vector<FileTextChanges> changes_;
    std::vector<InstallPackageActionInfo> commands_;
};

struct CodeFixActionInfo : CodeActionInfo {
    std::string fixName_;
    std::string fixId_ = {};
    std::string fixAllDescription_ = {};
};

struct CodeFixActionInfoList {
    std::vector<CodeFixActionInfo> infos_;
};

struct CodeFixOptions {
    ark::es2panda::lsp::CancellationToken token;
    ark::es2panda::lsp::FormatCodeSettings options;
    ark::es2panda::lsp::UserPreferences preferences;
};

struct NodeInfo {
    NodeInfo(std::string n, ark::es2panda::ir::AstNodeType k) : name(n), kind(k) {}
    std::string name;
    ark::es2panda::ir::AstNodeType kind;
};

typedef struct LSPAPI {
    DefinitionInfo (*getDefinitionAtPosition)(es2panda_Context *context, size_t position);
    std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> (*getApplicableRefactors)(es2panda_Context *context,
                                                                                      const char *kind,
                                                                                      size_t position);
    DefinitionInfo (*getImplementationAtPosition)(es2panda_Context *context, size_t position);
    bool (*isPackageModule)(es2panda_Context *context);
    ark::es2panda::lsp::CompletionEntryKind (*getAliasScriptElementKind)(es2panda_Context *context, size_t position);
    References (*getFileReferences)(char const *fileName, es2panda_Context *context, bool isPackageModule);
    DeclInfo (*getDeclInfo)(es2panda_Context *context, size_t position);
    std::vector<ark::es2panda::lsp::ClassHierarchyItemInfo> (*getClassHierarchiesImpl)(
        std::vector<es2panda_Context *> *contextList, const char *fileName, size_t pos);
    bool (*getSafeDeleteInfo)(es2panda_Context *context, size_t position);
    References (*getReferencesAtPosition)(es2panda_Context *context, DeclInfo *declInfo);
    es2panda_AstNode *(*getPrecedingToken)(es2panda_Context *context, const size_t pos);
    std::string (*getCurrentTokenValue)(es2panda_Context *context, size_t position);
    std::vector<FileTextChanges> (*OrganizeImportsImpl)(es2panda_Context *context, char const *fileName);
    QuickInfo (*getQuickInfoAtPosition)(const char *fileName, es2panda_Context *context, size_t position);
    CompletionEntryDetails (*getCompletionEntryDetails)(const char *entryName, const char *fileName,
                                                        es2panda_Context *context, size_t position);
    TextSpan (*getSpanOfEnclosingComment)(es2panda_Context *context, size_t pos, bool onlyMultiLine);
    DiagnosticReferences (*getSemanticDiagnostics)(es2panda_Context *context);
    DiagnosticReferences (*getSyntacticDiagnostics)(es2panda_Context *context);
    DiagnosticReferences (*getCompilerOptionsDiagnostics)(char const *fileName,
                                                          ark::es2panda::lsp::CancellationToken cancellationToken);
    TypeHierarchiesInfo (*getTypeHierarchies)(es2panda_Context *searchContext, es2panda_Context *context,
                                              size_t position);
    DocumentHighlightsReferences (*getDocumentHighlights)(es2panda_Context *context, size_t position);
    std::vector<ark::es2panda::lsp::RenameLocation> (*findRenameLocations)(
        const std::vector<es2panda_Context *> &fileContexts, es2panda_Context *context, size_t position);
    std::set<ark::es2panda::lsp::RenameLocation> (*findRenameLocationsInCurrentFile)(es2panda_Context *context,
                                                                                     size_t position);
    bool (*needsCrossFileRename)(es2panda_Context *context, size_t position);
    std::vector<ark::es2panda::lsp::RenameLocation> (*findRenameLocationsWithCancellationToken)(
        ark::es2panda::lsp::CancellationToken *tkn, const std::vector<es2panda_Context *> &fileContexts,
        es2panda_Context *context, size_t position);
    std::vector<SafeDeleteLocation> (*FindSafeDeleteLocation)(es2panda_Context *ctx,
                                                              const std::tuple<std::string, std::string> *declInfo);
    std::vector<ark::es2panda::lsp::ReferencedNode> (*findReferences)(
        ark::es2panda::lsp::CancellationToken *tkn, const std::vector<ark::es2panda::SourceFile> &srcFiles,
        const ark::es2panda::SourceFile &srcFile, size_t position);
    ark::es2panda::lsp::RenameInfoType (*getRenameInfo)(es2panda_Context *context, size_t position,
                                                        const char *pandaLibPath);
    std::vector<FieldsInfo> (*getClassPropertyInfo)(es2panda_Context *context, size_t pos, bool shouldCollectInherited);
    DiagnosticReferences (*getSuggestionDiagnostics)(es2panda_Context *context);
    ark::es2panda::lsp::CompletionInfo (*getCompletionsAtPosition)(es2panda_Context *context, size_t position);
    ark::es2panda::lsp::ClassHierarchy (*getClassHierarchyInfo)(es2panda_Context *context, size_t position);
    std::vector<TextSpan> (*getBraceMatchingAtPosition)(char const *fileName, size_t position);
    ark::es2panda::lsp::RefactorEditInfo (*getClassConstructorInfo)(es2panda_Context *context, size_t position,
                                                                    const std::vector<std::string> &properties);
    std::vector<Location> (*getImplementationLocationAtPosition)(es2panda_Context *context, int position);
    ark::es2panda::lsp::LineAndCharacter (*toLineColumnOffset)(es2panda_Context *context, size_t position);
    std::vector<ark::es2panda::lsp::TodoComment> (*getTodoComments)(
        char const *fileName, std::vector<ark::es2panda::lsp::TodoCommentDescriptor> &descriptors,
        ark::es2panda::lsp::CancellationToken *cancellationToken);
    InlayHintList (*provideInlayHints)(es2panda_Context *context, const TextSpan *span);
    SignatureHelpItems (*getSignatureHelpItems)(es2panda_Context *context, size_t position);
    size_t (*getOffsetByColAndLine)(const std::string &sourceCode, size_t line, size_t column);
    std::pair<size_t, size_t> (*getColAndLineByOffset)(const std::string &sourceCode, size_t offset);
    std::vector<CodeFixActionInfo> (*getCodeFixesAtPosition)(es2panda_Context *context, size_t start_position,
                                                             size_t end_position, std::vector<int> &errorCodes,
                                                             CodeFixOptions &codeFixOptions);
    CombinedCodeActionsInfo (*getCombinedCodeFix)(const char *fileName, const std::string &fixId,
                                                  CodeFixOptions &codeFixOptions);
    TextSpan *(*GetNameOrDottedNameSpan)(es2panda_Context *context, int startPos);
    es2panda_AstNode *(*getProgramAst)(es2panda_Context *context);
    std::vector<NodeInfo> (*getNodeInfosByDefinitionData)(es2panda_Context *context, size_t position);
    es2panda_AstNode *(*getClassDefinition)(es2panda_AstNode *astNode, const std::string &nodeName);
    es2panda_AstNode *(*getIdentifier)(es2panda_AstNode *astNode, const std::string &nodeName);
    DefinitionInfo (*getDefinitionDataFromNode)(es2panda_Context *context, const std::vector<NodeInfo *> &nodeInfos);
} LSPAPI;
CAPI_EXPORT LSPAPI const *GetImpl();
// NOLINTEND
#ifdef __cplusplus
}
#endif
#endif