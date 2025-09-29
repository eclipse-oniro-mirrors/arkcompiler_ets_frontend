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

#ifndef ES2PANDA_LSP_COMPLETIONS_H
#define ES2PANDA_LSP_COMPLETIONS_H

#include "public/es2panda_lib.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <optional>
#include "checker/checker.h"
#include "util/arktsconfig.h"

namespace ark::es2panda::lsp {

enum class CompletionEntryKind {
    TEXT = 1,
    METHOD = 2,
    FUNCTION = 3,
    CONSTRUCTOR = 4,
    FIELD = 5,
    VARIABLE = 6,
    CLASS = 7,
    INTERFACE = 8,
    MODULE = 9,
    PROPERTY = 10,
    UNIT = 11,
    VALUE = 12,
    ENUM = 13,
    KEYWORD = 14,
    SNIPPET = 15,
    COLOR = 16,
    FILE = 17,
    REFERENCE = 18,
    FOLDER = 19,
    ENUM_MEMBER = 20,
    CONSTANT = 21,
    STRUCT = 22,
    EVENT = 23,
    OPERATOR = 24,
    TYPE_PARAMETER = 25,
    ALIAS_TYPE = 26,
    ANNOTATION = 27
};

namespace sort_text {
constexpr std::string_view LOCAL_DECLARATION_PRIORITY = "10";
constexpr std::string_view LOCATION_PRIORITY = "11";
constexpr std::string_view OPTIONAL_MEMBER = "12";
constexpr std::string_view MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT = "13";
constexpr std::string_view SUGGESTED_CLASS_MEMBERS = "14";
constexpr std::string_view GLOBALS_OR_KEYWORDS = "15";
constexpr std::string_view AUTO_IMPORT_SUGGESTIONS = "16";
constexpr std::string_view CLASS_MEMBER_SNIPPETS = "17";
}  // namespace sort_text

enum class ResolutionStatus { RESOLVED, UNRESOLVED };
struct CompletionEntryData {
private:
    const char *fileName_;
    std::string namedExport_;
    std::string importDeclaration_;
    std::string completionName_;
    ResolutionStatus status_;

public:
    explicit CompletionEntryData(const char *fileName = "", std::string namedExport = "",
                                 std::string importDeclaration = "", std::string completionName = "",
                                 ResolutionStatus status = ResolutionStatus::UNRESOLVED)
        : fileName_(fileName),
          namedExport_(std::move(namedExport)),
          importDeclaration_(std::move(importDeclaration)),
          completionName_(std::move(completionName)),
          status_(status)
    {
    }

    const char *GetFileName()
    {
        return fileName_;
    }

    const std::string &GetNamedExport()
    {
        return namedExport_;
    }

    const std::string &GetImportDeclaration()
    {
        return importDeclaration_;
    }

    ResolutionStatus GetStatus()
    {
        return status_;
    }
};
struct CompletionEntry {
private:
    std::string name_;
    CompletionEntryKind kind_;
    std::string sortText_;
    // This is what the Client uses
    std::string insertText_;
    std::optional<CompletionEntryData> data_;
    std::string typeSig_;

public:
    explicit CompletionEntry(std::string name = "", CompletionEntryKind kind = CompletionEntryKind::TEXT,
                             std::string sortText = "", std::string insertText = "",
                             std::optional<CompletionEntryData> data = std::nullopt, std::string typeSig = "")
        : name_(std::move(name)),
          kind_(kind),
          sortText_(std::move(sortText)),
          insertText_(std::move(insertText)),
          data_(std::move(data)),
          typeSig_(std::move(typeSig))
    {
    }
    std::string GetTypeSig() const
    {
        return typeSig_;
    }
    std::string GetSortText() const
    {
        return sortText_;
    }
    std::string GetInsertText() const
    {
        return insertText_;
    }
    CompletionEntryKind GetCompletionKind() const
    {
        return kind_;
    }
    std::string GetName() const
    {
        return name_;
    }
    bool operator==(const CompletionEntry &other) const
    {
        return name_ == other.name_ && kind_ == other.kind_ && sortText_ == other.sortText_ &&
               insertText_ == other.insertText_;
    }
    bool operator!=(const CompletionEntry &other) const
    {
        return !(*this == other);
    }
    std::string GetSortText()
    {
        return sortText_;
    }
    std::optional<CompletionEntryData> GetCompletionEntryData()
    {
        return data_;
    }
};

enum class CompletionDataKind { DATA, KEYWORDS };

struct Request {
    CompletionDataKind kind;
    std::vector<CompletionEntry> keywordCompletions;
};

struct LabelDetails {
private:
    std::optional<std::string> detail_;
    std::optional<std::string> description_;
};

struct CompletionInfo {
private:
    std::vector<CompletionEntry> entries_;

public:
    explicit CompletionInfo(std::vector<CompletionEntry> entries = {}) : entries_(std::move(entries)) {}

    std::vector<CompletionEntry> &GetEntries()
    {
        return entries_;
    }
};

std::vector<CompletionEntry> AllKeywordsCompletions();
std::vector<CompletionEntry> GetKeywordCompletions(const std::string &input);
std::vector<CompletionEntry> GetMemberCompletions(es2panda_Context *context, size_t position);
Request KeywordCompletionData(const std::string &input);
std::string ToLowerCase(const std::string &str);
std::vector<CompletionEntry> GetCompletionsAtPositionImpl(es2panda_Context *context, size_t position);
ArenaVector<varbinder::Scope *> BuildScopePath(varbinder::Scope *startScope, ArenaAllocator *allocator);
CompletionEntry ProcessAutoImportForEntry(CompletionEntry &entry);

std::optional<CompletionEntryData> GetAutoImportCompletionEntry(ark::es2panda::lsp::CompletionEntryData *data,
                                                                const ArkTsConfig *config, const std::string &name);
std::optional<CompletionEntryData> CompletionEntryDataToOriginInfo(ark::es2panda::lsp::CompletionEntryData *data,
                                                                   const ArkTsConfig *config, const std::string &name);
std::optional<bool> IsCompletionEntryDataResolved(ark::es2panda::lsp::CompletionEntryData *data,
                                                  const ArkTsConfig *config);
std::vector<ir::AstNode *> FilterFromBody(const ArenaVector<ir::AstNode *> &bodyNodes, const std::string &triggerWord,
                                          bool isStatic = false);

bool StartsWith(const std::string &str, const std::string &prefix);
bool IsDefinedClassOrStruct(ir::AstNode *preNode);

std::vector<CompletionEntry> GetPropertyCompletions(ir::AstNode *preNode, const std::string &triggerWord);
std::string GetClassPropertyName(ir::AstNode *node);
ir::AstNode *GetIdentifierFromTSInterfaceHeritage(ir::AstNode *node);

}  // namespace ark::es2panda::lsp
#endif
