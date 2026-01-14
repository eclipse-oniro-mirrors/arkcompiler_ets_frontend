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

#include "completions.h"
#include "internal_api.h"
#include "compiler/lowering/util.h"
#include "ir/astNode.h"
#include "ir/ets/etsUnionType.h"
#include "public/public.h"
#include <algorithm>
#include <regex>
#include <cstddef>
#include <optional>
#include <string>
#include "generated/tokenType.h"
#include <cstdio>
#include "public/es2panda_lib.h"
#include "lexer/token/letters.h"
#include "ir/base/classProperty.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/ts/tsModuleDeclaration.h"
#include "ir/ts/tsModuleBlock.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/statements/classDeclaration.h"
#include "ir/statements/functionDeclaration.h"
#include "ir/statements/variableDeclaration.h"
#include "ir/statements/variableDeclarator.h"
#include "ir/ets/etsParameterExpression.h"
#include <sstream>
namespace ark::es2panda::lsp {

static std::vector<CompletionEntry> GetCompletionsForDeclaration(ir::AstNode *decl, const std::string &triggerWord,
                                                                 bool isStatic = false);

inline fs::path NormalizePath(const fs::path &p)
{
#if defined(__cpp_lib_filesystem)
    return p.lexically_normal();
#else
    try {
        return fs::canonical(p);
    } catch (...) {
        return fs::absolute(p);
    }
#endif
}

inline bool IsDirectory(const fs::directory_entry &e)
{
#if defined(__cpp_lib_filesystem)
    return e.is_directory();
#else
    return fs::is_directory(e.path());
#endif
}

inline bool IsRegularFile(const fs::directory_entry &e)
{
#if defined(__cpp_lib_filesystem)
    return e.is_regular_file();
#else
    return fs::is_regular_file(e.path());
#endif
}

std::string ToLowerCase(const std::string &str)
{
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}

std::vector<CompletionEntry> AllKeywordsCompletions()
{
    std::vector<CompletionEntry> keywords;
    std::string name;
    for (int i = static_cast<int>(lexer::TokenType::FIRST_KEYW); i <= static_cast<int>(lexer::TokenType::KEYW_YIELD);
         i++) {
        name = TokenToString(static_cast<lexer::TokenType>(i));
        keywords.emplace_back(lsp::CompletionEntry(name, CompletionEntryKind::KEYWORD,
                                                   std::string(sort_text::GLOBALS_OR_KEYWORDS), name));
    }
    return keywords;
}

std::vector<CompletionEntry> GetKeywordCompletions(const std::string &input)
{
    std::vector<CompletionEntry> allKeywords = AllKeywordsCompletions();
    std::vector<CompletionEntry> completions;

    for (const auto &entry : allKeywords) {
        if (ToLowerCase(entry.GetName()).find(ToLowerCase(input)) == 0) {
            completions.push_back(entry);
        }
    }
    return completions;
}

std::string GetTypeSig(ir::AstNode *node)
{
    if (!node->IsMethodDefinition() && !node->IsClassProperty()) {
        return "";
    }

    if (node->IsMethodDefinition()) {
        auto func = node->AsMethodDefinition()->Function();
        std::string sig;
        sig.append("(");
        bool first = true;
        for (auto *param : func->Params()) {
            if (!first) {
                sig.append(",");
            }
            first = false;
            std::string typeStr;
            if (param->IsETSParameterExpression() && param->AsETSParameterExpression()->TypeAnnotation()) {
                typeStr = param->AsETSParameterExpression()->TypeAnnotation()->DumpEtsSrc();
            } else if (param->IsIdentifier() && param->AsIdentifier()->TypeAnnotation()) {
                typeStr = param->AsIdentifier()->TypeAnnotation()->DumpEtsSrc();
            } else {
                typeStr = "any";
            }
            typeStr.erase(
                std::remove_if(typeStr.begin(), typeStr.end(), [](unsigned char c) { return std::isspace(c); }),
                typeStr.end());
            sig.append(typeStr);
        }
        sig.append(")");
        if (func->ReturnTypeAnnotation()) {
            std::string retStr = func->ReturnTypeAnnotation()->DumpEtsSrc();
            retStr.erase(std::remove_if(retStr.begin(), retStr.end(), [](unsigned char c) { return std::isspace(c); }),
                         retStr.end());
            sig.append(":");
            sig.append(retStr);
        }
        return sig;
    }
    if (node->IsClassProperty()) {
        auto type = node->AsClassProperty()->TypeAnnotation();
        if (type) {
            std::string typeStr = type->DumpEtsSrc();
            typeStr.erase(
                std::remove_if(typeStr.begin(), typeStr.end(), [](unsigned char c) { return std::isspace(c); }),
                typeStr.end());
            return typeStr;
        }
    }
    return "";
}

CompletionEntry GetDeclarationEntry(ir::AstNode *node)
{
    if (node == nullptr) {
        return CompletionEntry();
    }
    std::string name;
    // GetClassPropertyName function could get name of ClassDeclaration
    if (node->IsClassDeclaration()) {
        name = GetClassPropertyName(node);
        return CompletionEntry(name, CompletionEntryKind::CLASS, std::string(sort_text::GLOBALS_OR_KEYWORDS), name);
    }
    if (node->IsTSInterfaceDeclaration()) {
        if (node->AsTSInterfaceDeclaration()->Id() == nullptr) {
            return CompletionEntry();
        }
        name = std::string(node->AsTSInterfaceDeclaration()->Id()->Name());
        return CompletionEntry(name, CompletionEntryKind::INTERFACE, std::string(sort_text::GLOBALS_OR_KEYWORDS), name);
    }
    if (node->IsMethodDefinition()) {
        name = std::string(node->AsMethodDefinition()->Key()->AsIdentifier()->Name());
        return CompletionEntry(name, CompletionEntryKind::METHOD, std::string(sort_text::GLOBALS_OR_KEYWORDS),
                               name + "()", std::nullopt, GetTypeSig(node));
    }
    if (node->IsClassProperty()) {
        name = GetClassPropertyName(node);
        return CompletionEntry(name, CompletionEntryKind::PROPERTY, std::string(sort_text::GLOBALS_OR_KEYWORDS), name,
                               std::nullopt, GetTypeSig(node));
    }
    if (node->IsETSStructDeclaration()) {
        if (node->AsETSStructDeclaration()->Definition() == nullptr) {
            return CompletionEntry();
        }
        if (node->AsETSStructDeclaration()->Definition()->Ident() == nullptr) {
            return CompletionEntry();
        }
        name = std::string(node->AsETSStructDeclaration()->Definition()->Ident()->Name());
        return CompletionEntry(name, CompletionEntryKind::STRUCT, std::string(sort_text::GLOBALS_OR_KEYWORDS), name);
    }
    return CompletionEntry();
}

static void GetExportFromClass(ir::ClassDefinition *classDef, std::vector<CompletionEntry> &exportEntries,
                               const std::string &fileName = "")
{
    for (auto &prop : classDef->Body()) {
        if (prop->IsClassDeclaration() && prop->AsClassDeclaration()->Definition()->IsNamespaceTransformed()) {
            GetExportFromClass(prop->AsClassDeclaration()->Definition(), exportEntries, fileName);
        }
        if (prop->IsExported()) {
            auto entry = GetDeclarationEntry(prop);
            if (!entry.GetName().empty() &&
                (fileName.empty() || entry.GetName().compare(0, fileName.length(), fileName) == 0)) {
                exportEntries.emplace_back(entry);
            }
        }
    }
}

std::vector<CompletionEntry> GetExportsFromProgram(parser::Program *program, const std::string &fileName = "")
{
    std::vector<CompletionEntry> exportEntries;
    for (auto &stmt : program->Ast()->Statements()) {
        if (stmt->IsClassDeclaration()) {
            auto classDef = stmt->AsClassDeclaration()->Definition();
            if (classDef->IsGlobal() || classDef->IsNamespaceTransformed()) {
                GetExportFromClass(classDef, exportEntries, fileName);
            }
        }
        if (stmt->IsExported()) {
            auto entry = GetDeclarationEntry(stmt);
            if (!entry.GetName().empty() &&
                (fileName.empty() || entry.GetName().compare(0, fileName.length(), fileName) == 0)) {
                exportEntries.emplace_back(entry);
            }
        }
    }

    return exportEntries;
}

std::vector<CompletionEntry> GetSystemInterfaceCompletions(const std::string &input, parser::Program *program)
{
    std::vector<CompletionEntry> allExternalSourceExports;
    std::vector<CompletionEntry> completions;
    for (auto [_, programList] : program->ExternalSources()) {
        for (auto prog : programList) {
            auto exports = GetExportsFromProgram(prog);
            if (!exports.empty()) {
                allExternalSourceExports.insert(allExternalSourceExports.end(), exports.begin(), exports.end());
            }
        }
    }

    for (const auto &entry : allExternalSourceExports) {
        if (ToLowerCase(entry.GetName()).find(ToLowerCase(input)) == 0) {
            completions.emplace_back(entry);
        }
    }
    return completions;
}

bool IsPointValid(const std::string &str)
{
    std::regex pattern(R"(^[a-zA-Z_$][a-zA-Z0-9_$().\-]*(\?)?\.$)");
    return std::regex_match(str, pattern);
}

bool IsEndWithValidPoint(std::string str)
{
    return !str.empty() && str.back() == '.' && IsPointValid(str);
}

bool IsEndWithToken(ir::AstNode *preNode, std::string str)
{
    if (str.empty()) {
        return preNode->IsIdentifier();
    }
    return str.back() != '.' && preNode->IsIdentifier();
}

size_t GetPrecedingTokenPosition(std::string sourceCode, size_t pos)
{
    while (pos > 0) {
        char c = sourceCode[pos];
        if (std::isalnum(c) != 0 || c == '_') {
            return pos;
        }
        pos--;
    }
    return pos;
}

bool IsIgnoredName(const std::string &name)
{
    static const std::unordered_set<std::string> IGNORED_NAMES = {"constructor", "_$init$_",
                                                                  "_$initializerBlockInit$_"};
    return IGNORED_NAMES.find(name) != IGNORED_NAMES.end();
}

bool IsWordPartOfIdentifierName(ir::AstNode *node, std::string triggerWord)
{
    if (node == nullptr || !node->IsIdentifier()) {
        return false;
    }
    std::string name(node->AsIdentifier()->Name());
    std::string lowerTrigger = std::move(triggerWord);
    std::string lowerName = name;
    std::transform(lowerTrigger.begin(), lowerTrigger.end(), lowerTrigger.begin(), ::tolower);
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    return lowerName.find(lowerTrigger) != std::string::npos && !IsIgnoredName(name);
}

std::vector<ir::AstNode *> FilterFromBody(const ArenaVector<ir::AstNode *> &bodyNodes, const std::string &triggerWord,
                                          bool isStatic)
{
    std::vector<ir::AstNode *> res;
    if (bodyNodes.empty()) {
        return res;
    }
    for (auto node : bodyNodes) {
        if (node->IsClassProperty()) {
            if (isStatic && !node->AsClassProperty()->IsStatic()) {
                continue;
            }
            if (IsWordPartOfIdentifierName(node->AsClassProperty()->Key(), triggerWord)) {
                res.emplace_back(node);
            }
        }
        if (node->IsClassDeclaration()) {
            auto def = node->AsClassDeclaration()->Definition()->AsClassDefinition();
            if (def != nullptr && def->Ident() != nullptr && IsWordPartOfIdentifierName(def->Ident(), triggerWord)) {
                res.emplace_back(node);
            }
        }
        if (node->IsMethodDefinition()) {
            if (isStatic && !node->AsMethodDefinition()->IsStatic()) {
                continue;
            }
            if (IsWordPartOfIdentifierName(node->AsMethodDefinition()->Key(), triggerWord)) {
                res.emplace_back(node);
            }
        }
        // add new node to find interface in namespace
        if (node->IsTSInterfaceDeclaration()) {
            res.emplace_back(node);
        }
    }
    return res;
}

std::vector<ir::AstNode *> FilterFromEnumMember(const ArenaVector<ir::AstNode *> &members,
                                                const std::string &triggerWord)
{
    std::vector<ir::AstNode *> res;
    if (members.empty()) {
        return res;
    }
    for (auto member : members) {
        if (member->IsTSEnumMember() && IsWordPartOfIdentifierName(member->AsTSEnumMember()->Key(), triggerWord)) {
            res.emplace_back(member);
        }
    }
    return res;
}

std::vector<ir::AstNode *> FilterFromInterfaceBody(const ArenaVector<ir::AstNode *> &members,
                                                   const std::string &triggerWord)
{
    std::vector<ir::AstNode *> res;
    if (members.empty()) {
        return res;
    }
    for (auto member : members) {
        if (member->IsMethodDefinition() &&
            IsWordPartOfIdentifierName(member->AsMethodDefinition()->Key(), triggerWord)) {
            res.emplace_back(member);
        }
    }
    return res;
}

std::string GetClassPropertyName(ir::AstNode *node)
{
    // property in class
    if (node->IsClassProperty()) {
        auto key = node->AsClassProperty()->Key();
        if (key != nullptr && key->IsIdentifier()) {
            return std::string(key->AsIdentifier()->Name());
        }
    }
    // class in namespace
    if (node->IsClassDeclaration()) {
        auto def = node->AsClassDeclaration()->Definition();
        if (def != nullptr && def->IsClassDefinition() && def->AsClassDefinition()->Ident() != nullptr &&
            def->AsClassDefinition()->Ident()->IsIdentifier()) {
            return std::string(def->AsClassDefinition()->Ident()->AsIdentifier()->Name());
        }
    }
    if (node->IsTSInterfaceDeclaration()) {
        auto def = node->AsTSInterfaceDeclaration()->Id();
        if (def != nullptr) {
            return std::string(def->Name());
        }
    }
    return "";
}

std::string GetEnumMemberName(ir::AstNode *node)
{
    if (!node->IsTSEnumMember()) {
        return "";
    }
    auto id = node->AsTSEnumMember()->Key();
    if (id == nullptr || !id->IsIdentifier()) {
        return "";
    }
    return std::string(id->AsIdentifier()->Name());
}

std::string GetMethodDefinitionName(ir::AstNode *node)
{
    if (!node->IsMethodDefinition()) {
        return "";
    }
    auto key = node->AsMethodDefinition()->Key();
    if (key == nullptr || !key->IsIdentifier()) {
        return "";
    }
    return std::string(key->AsIdentifier()->Name());
}

ir::AstNode *GetDefinitionFromTypeAnnotation(ir::TypeNode *type)
{
    if (type == nullptr || !type->IsETSTypeReference()) {
        return nullptr;
    }
    auto typeRefPart = type->AsETSTypeReference()->Part();
    if (typeRefPart == nullptr) {
        return nullptr;
    }
    auto id = typeRefPart->Name();
    if (id == nullptr || !id->IsIdentifier()) {
        return nullptr;
    }
    return compiler::DeclarationFromIdentifier(id->AsIdentifier());
}

static ir::AstNode *FindRelevantParentDeclaration(ir::AstNode *decl)
{
    while (decl != nullptr && (decl->IsIdentifier() || decl->IsMemberExpression())) {
        decl = decl->Parent();
        if (decl != nullptr && (decl->IsTSModuleDeclaration() || decl->IsClassDefinition() ||
                                decl->IsClassDeclaration() || decl->IsTSInterfaceDeclaration())) {
            break;
        }
    }
    return decl;
}

static ir::TypeNode *CollectDefinitionsFromDecl(ir::AstNode *decl, std::vector<ir::AstNode *> &definitions)
{
    if (decl->IsETSParameterExpression()) {
        return decl->AsETSParameterExpression()->TypeAnnotation();
    }
    if (decl->IsClassProperty()) {
        return decl->AsClassProperty()->TypeAnnotation();
    }
    if (decl->IsClassDeclaration()) {
        definitions.push_back(decl->AsClassDeclaration()->Definition());
    } else if (decl->IsClassDefinition() || decl->IsMethodDefinition() || decl->IsTSInterfaceDeclaration() ||
               decl->IsTSModuleDeclaration()) {
        definitions.push_back(decl);
    }
    return nullptr;
}

static void CollectDefinitionsFromType(ir::TypeNode *type, std::vector<ir::AstNode *> &definitions)
{
    if (type->IsETSTypeReference()) {
        definitions.push_back(GetDefinitionFromTypeAnnotation(type));
    } else if (type->IsETSUnionType()) {
        for (auto *combine : type->AsETSUnionType()->Types()) {
            definitions.push_back(GetDefinitionFromTypeAnnotation(combine));
        }
    }
}

std::vector<ir::AstNode *> GetDefinitionFromDeclType(ir::AstNode *node)
{
    std::vector<ir::AstNode *> definitions;
    if (node == nullptr || !node->IsIdentifier()) {
        return definitions;
    }
    auto decl = compiler::DeclarationFromIdentifier(node->AsIdentifier());
    if (decl == nullptr) {
        return definitions;
    }

    decl = FindRelevantParentDeclaration(decl);
    if (decl == nullptr) {
        return definitions;
    }

    ir::TypeNode *type = CollectDefinitionsFromDecl(decl, definitions);
    if (type != nullptr) {
        CollectDefinitionsFromType(type, definitions);
    }
    return definitions;
}

static ir::AstNode *GetNodeFromType(checker::Type *type)
{
    if (type && type->Variable() && type->Variable()->Declaration()) {
        return type->Variable()->Declaration()->Node();
    }
    return nullptr;
}

std::vector<ir::AstNode *> GetDefinitionFromIdentifier(ir::AstNode *node)
{
    std::vector<ir::AstNode *> definitions;
    if (node == nullptr || !node->IsIdentifier()) {
        return definitions;
    }

    auto *tsType = node->AsIdentifier()->TsType();
    if (tsType == nullptr) {
        return GetDefinitionFromDeclType(node);
    }

    if (node->AsIdentifier()->TsType()->IsETSUnionType()) {
        auto *unionType = tsType->AsETSUnionType();
        for (auto *assemblerType : unionType->ConstituentTypes()) {
            auto *defNode = GetNodeFromType(assemblerType);
            if (defNode != nullptr && std::find(definitions.begin(), definitions.end(), defNode) == definitions.end()) {
                definitions.push_back(defNode);
            }
        }
    } else if (tsType->IsETSObjectType() || tsType->IsETSFunctionType()) {
        if (auto *defNode = GetNodeFromType(tsType)) {
            definitions.push_back(defNode);
        }
    }

    if (definitions.empty()) {
        return GetDefinitionFromDeclType(node);
    }

    return definitions;
}

std::vector<CompletionEntry> GetEntriesForClassDeclaration(
    const std::vector<ark::es2panda::ir::AstNode *> &propertyNodes)
{
    if (propertyNodes.empty()) {
        return {};
    }
    std::vector<CompletionEntry> completions;
    completions.reserve(propertyNodes.size());
    std::string name;
    for (auto node : propertyNodes) {
        if (node->IsClassProperty()) {
            name = GetClassPropertyName(node);
            completions.emplace_back(lsp::CompletionEntry(name, CompletionEntryKind::PROPERTY,
                                                          std::string(sort_text::SUGGESTED_CLASS_MEMBERS), name,
                                                          std::nullopt, GetTypeSig(node)));
        }
        if (node->IsClassDeclaration()) {
            name = GetClassPropertyName(node);
            completions.emplace_back(lsp::CompletionEntry(
                name, CompletionEntryKind::CLASS, std::string(sort_text::MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), name));
        }
        if (node->IsMethodDefinition()) {
            name = GetMethodDefinitionName(node);
            completions.emplace_back(lsp::CompletionEntry(name, CompletionEntryKind::METHOD,
                                                          std::string(sort_text::CLASS_MEMBER_SNIPPETS), name + "()",
                                                          std::nullopt, GetTypeSig(node)));
        }
        if (node->IsTSInterfaceDeclaration()) {
            completions.emplace_back(lsp::CompletionEntry(GetClassPropertyName(node), CompletionEntryKind::INTERFACE,
                                                          std::string(sort_text::SUGGESTED_CLASS_MEMBERS)));
        }
    }
    return completions;
}

std::vector<CompletionEntry> GetEntriesForTSInterfaceDeclaration(
    const std::vector<ark::es2panda::ir::AstNode *> &propertyNodes)
{
    if (propertyNodes.empty()) {
        return {};
    }
    std::vector<CompletionEntry> completions;
    completions.reserve(propertyNodes.size());
    std::string name;
    for (auto node : propertyNodes) {
        if (!node->IsMethodDefinition()) {
            continue;
        }
        name = GetMethodDefinitionName(node);
        if (node->AsMethodDefinition()->IsGetter() || node->AsMethodDefinition()->IsSetter()) {
            // Each of properties in interface no need to add '()'
            completions.emplace_back(lsp::CompletionEntry(name, CompletionEntryKind::METHOD,
                                                          std::string(sort_text::CLASS_MEMBER_SNIPPETS), name,
                                                          std::nullopt, GetTypeSig(node)));
        } else {
            completions.emplace_back(lsp::CompletionEntry(name, CompletionEntryKind::METHOD,
                                                          std::string(sort_text::CLASS_MEMBER_SNIPPETS), name + "()",
                                                          std::nullopt, GetTypeSig(node)));
        }
    }
    return completions;
}

std::vector<CompletionEntry> GetEntriesForEnumDeclaration(
    const std::vector<ark::es2panda::ir::AstNode *> &qualifiedMembers)
{
    if (qualifiedMembers.empty()) {
        return {};
    }
    std::vector<CompletionEntry> completions;
    completions.reserve(qualifiedMembers.size());
    std::string name;
    for (auto member : qualifiedMembers) {
        name = GetEnumMemberName(member);
        completions.emplace_back(lsp::CompletionEntry(name, CompletionEntryKind::ENUM_MEMBER,
                                                      std::string(sort_text::MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT),
                                                      name));
    }

    return completions;
}

static void AddClassCompletion(ir::Statement *stmt, const std::string &triggerWord,
                               std::vector<CompletionEntry> &completions)
{
    std::string name = stmt->AsClassDeclaration()->Definition()->Ident()->Name().Mutf8();
    if (name.find(triggerWord) == 0) {
        completions.emplace_back(name, CompletionEntryKind::CLASS,
                                 std::string(sort_text::MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), name);
    }
}

static void AddInterfaceCompletion(ir::Statement *stmt, const std::string &triggerWord,
                                   std::vector<CompletionEntry> &completions)
{
    std::string name = stmt->AsTSInterfaceDeclaration()->Id()->Name().Mutf8();
    if (name.find(triggerWord) == 0) {
        completions.emplace_back(name, CompletionEntryKind::INTERFACE,
                                 std::string(sort_text::MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), name);
    }
}

static void AddModuleCompletion(ir::Statement *stmt, const std::string &triggerWord,
                                std::vector<CompletionEntry> &completions)
{
    if (!stmt->AsTSModuleDeclaration()->Name()->IsIdentifier()) {
        return;
    }
    std::string name = stmt->AsTSModuleDeclaration()->Name()->AsIdentifier()->Name().Mutf8();
    if (name.find(triggerWord) == 0) {
        completions.emplace_back(name, CompletionEntryKind::MODULE,
                                 std::string(sort_text::MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), name);
    }
}

static void AddVariableCompletion(ir::Statement *stmt, const std::string &triggerWord,
                                  std::vector<CompletionEntry> &completions)
{
    for (auto *declarator : stmt->AsVariableDeclaration()->Declarators()) {
        if (!declarator->Id()->IsIdentifier()) {
            continue;
        }
        std::string name = declarator->Id()->AsIdentifier()->Name().Mutf8();
        if (name.find(triggerWord) != 0) {
            continue;
        }
        auto kind = stmt->AsVariableDeclaration()->Kind() == ir::VariableDeclaration::VariableDeclarationKind::CONST
                        ? CompletionEntryKind::CONSTANT
                        : CompletionEntryKind::VARIABLE;
        completions.emplace_back(name, kind, std::string(sort_text::MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), name);
    }
}

static void AddFunctionCompletion(ir::Statement *stmt, const std::string &triggerWord,
                                  std::vector<CompletionEntry> &completions)
{
    auto *func = stmt->AsFunctionDeclaration()->Function();
    if (func == nullptr || func->Id() == nullptr) {
        return;
    }
    std::string name = func->Id()->Name().Mutf8();
    if (name.find(triggerWord) == 0) {
        completions.emplace_back(name, CompletionEntryKind::FUNCTION,
                                 std::string(sort_text::MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), name + "()");
    }
}

static void AddCompletionFromStatement(ir::Statement *stmt, const std::string &triggerWord,
                                       std::vector<CompletionEntry> &completions)
{
    if (!stmt->IsExported()) {
        return;
    }

    if (stmt->IsClassDeclaration()) {
        AddClassCompletion(stmt, triggerWord, completions);
    } else if (stmt->IsTSInterfaceDeclaration()) {
        AddInterfaceCompletion(stmt, triggerWord, completions);
    } else if (stmt->IsTSModuleDeclaration()) {
        AddModuleCompletion(stmt, triggerWord, completions);
    } else if (stmt->IsVariableDeclaration()) {
        AddVariableCompletion(stmt, triggerWord, completions);
    } else if (stmt->IsFunctionDeclaration()) {
        AddFunctionCompletion(stmt, triggerWord, completions);
    }
}

std::vector<CompletionEntry> GetCompletionFromTSModuleDeclaration(ir::TSModuleDeclaration *decl,
                                                                  const std::string &triggerWord)
{
    std::vector<CompletionEntry> completions;
    auto *body = decl->Body();
    if (body == nullptr) {
        return completions;
    }

    if (!body->IsTSModuleBlock()) {
        return completions;
    }

    for (auto *stmt : body->AsTSModuleBlock()->Statements()) {
        AddCompletionFromStatement(stmt, triggerWord, completions);
    }
    return completions;
}

ir::AstNode *GetIdentifierFromSuper(ir::AstNode *super)
{
    if (super == nullptr || !super->IsETSTypeReference()) {
        return nullptr;
    }
    auto part = super->AsETSTypeReference()->Part();
    if (part == nullptr || !part->IsETSTypeReferencePart()) {
        return nullptr;
    }
    return part->AsETSTypeReferencePart()->Name();
}

std::vector<CompletionEntry> GetCompletionFromClassDefinition(ir::ClassDefinition *decl, const std::string &triggerWord,
                                                              bool isStatic)
{
    // After enum refactoring, enum declaration is transformed to a class declaration
    if (compiler::ClassDefinitionIsEnumTransformed(decl)) {
        if (decl->AsClassDefinition()->OrigEnumDecl() == nullptr) {
            return {};
        }
        auto members = decl->AsClassDefinition()->OrigEnumDecl()->AsTSEnumDeclaration()->Members();
        auto qualifiedMembers = FilterFromEnumMember(members, triggerWord);
        return GetEntriesForEnumDeclaration(qualifiedMembers);
    }
    auto bodyNodes = decl->Body();
    std::vector<CompletionEntry> extendCompletions;
    auto super = decl->Super();
    if (super != nullptr) {
        auto ident = GetIdentifierFromSuper(super);
        if (ident != nullptr) {
            auto decls = GetDefinitionFromIdentifier(ident);
            for (auto superDecl : decls) {
                auto superItems = GetCompletionsForDeclaration(superDecl, triggerWord, isStatic);
                extendCompletions.insert(extendCompletions.end(), superItems.begin(), superItems.end());
            }
        }
    }
    auto propertyNodes = FilterFromBody(bodyNodes, triggerWord, isStatic);
    auto res = GetEntriesForClassDeclaration(propertyNodes);
    res.insert(res.end(), extendCompletions.begin(), extendCompletions.end());
    return res;
}

ir::AstNode *GetIdentifierFromTSInterfaceHeritage(ir::AstNode *node)
{
    if (node == nullptr) {
        return nullptr;
    }
    ir::AstNode *expr = nullptr;
    if (node->IsTSInterfaceHeritage()) {
        expr = node->AsTSInterfaceHeritage()->Expr();
    } else if (node->IsTSClassImplements()) {
        expr = node->AsTSClassImplements()->Expr();
    } else {
        return nullptr;
    }
    if (expr == nullptr) {
        return nullptr;
    }
    auto part = expr->AsETSTypeReference()->Part();
    if (part == nullptr) {
        return nullptr;
    }
    return part->AsETSTypeReferencePart()->Name();
}

std::vector<CompletionEntry> GetCompletionFromTSInterfaceDeclaration(ir::TSInterfaceDeclaration *decl,
                                                                     const std::string &triggerWord)
{
    std::vector<CompletionEntry> completions;
    auto body = decl->AsTSInterfaceDeclaration()->Body();
    if (body == nullptr) {
        return {};
    }
    auto bodies = body->Body();
    std::vector<CompletionEntry> extendCompletions;
    auto extends = decl->AsTSInterfaceDeclaration()->Extends();
    for (auto extend : extends) {
        auto ident = GetIdentifierFromTSInterfaceHeritage(extend);
        if (ident != nullptr && ident->IsIdentifier()) {
            auto extendInterf = compiler::DeclarationFromIdentifier(ident->AsIdentifier());
            if (extendInterf == nullptr) {
                continue;
            }
            auto extendCom =
                extendInterf->IsTSInterfaceDeclaration()
                    ? GetCompletionFromTSInterfaceDeclaration(extendInterf->AsTSInterfaceDeclaration(), triggerWord)
                    : completions;
            extendCompletions.insert(extendCompletions.end(), extendCom.begin(), extendCom.end());
        }
    }
    auto qualifiedBodies = FilterFromInterfaceBody(bodies, triggerWord);
    auto res = GetEntriesForTSInterfaceDeclaration(qualifiedBodies);
    res.insert(res.end(), extendCompletions.begin(), extendCompletions.end());
    return res;
}

ir::AstNode *GetDefinitionOfThisExpression(ir::AstNode *preNode)
{
    if (preNode == nullptr || (!preNode->IsThisExpression() && !preNode->IsTSThisType())) {
        return nullptr;
    }
    while (preNode->Parent() != nullptr) {
        preNode = preNode->Parent();
        // class and namespace
        if (preNode->IsClassDeclaration() && IsDefinedClassOrStruct(preNode)) {
            return preNode->AsClassDeclaration()->Definition();
        }
        if (preNode->IsETSStructDeclaration() && IsDefinedClassOrStruct(preNode)) {
            return preNode->AsETSStructDeclaration()->Definition();
        }
        if (preNode->IsTSInterfaceDeclaration()) {
            return preNode;
        }
    }
    return nullptr;
}

ir::AstNode *GetIdentifierOfThisExpression(ir::AstNode *preNode)
{
    if (preNode == nullptr || (!preNode->IsThisExpression() && !preNode->IsTSThisType())) {
        return nullptr;
    }
    auto def = GetDefinitionOfThisExpression(preNode);
    if (def->IsClassDefinition()) {
        return def->AsClassDefinition()->Ident();
    }
    if (def->IsTSInterfaceDeclaration()) {
        return def->AsTSInterfaceDeclaration()->Id();
    }
    return nullptr;
}

std::vector<CompletionEntry> GetCompletionFromMethodDefinition(ir::MethodDefinition *decl,
                                                               const std::string &triggerWord)
{
    auto value = decl->AsMethodDefinition()->Value();
    if (value == nullptr || !value->IsFunctionExpression()) {
        return {};
    }
    auto func = value->AsFunctionExpression()->Function();
    if (func == nullptr || func->ReturnTypeAnnotation() == nullptr) {
        return {};
    }
    auto returnType = func->ReturnTypeAnnotation();
    if (returnType->IsTSThisType()) {
        auto ident = GetIdentifierOfThisExpression(returnType);
        if (ident == nullptr || !ident->IsIdentifier()) {
            return {};
        }
        return GetPropertyCompletions(reinterpret_cast<ir::AstNode *>(ident), triggerWord);
    }
    if (returnType->IsETSTypeReference()) {
        auto expr = returnType->AsETSTypeReference()->Part()->Name();
        if (expr == nullptr || !expr->IsIdentifier()) {
            return {};
        }
        return GetPropertyCompletions(reinterpret_cast<ir::AstNode *>(expr), triggerWord);
    }
    return {};
}

ir::AstNode *GetIndentifierFromCallExpression(ir::AstNode *node)
{
    if (node == nullptr) {
        return nullptr;
    }
    if (!node->IsCallExpression() && !node->IsMemberExpression()) {
        return nullptr;
    }
    auto callee = node;
    if (node->IsCallExpression()) {
        callee = node->AsCallExpression()->Callee();
    }
    if (callee->IsIdentifier()) {
        return callee;
    }
    if (callee == nullptr || !callee->IsMemberExpression()) {
        return nullptr;
    }
    return callee->AsMemberExpression()->Property();
}

util::StringView GetNameFromDefinition(ir::AstNode *preNode)
{
    if (preNode == nullptr || !preNode->IsClassDefinition()) {
        return "";
    }
    auto ident = preNode->AsClassDefinition()->Ident();
    if (ident == nullptr) {
        return "";
    }
    return ident->Name();
}

bool IsDeclarationNameDefined(util::StringView name)
{
    static const std::unordered_set<util::StringView> IGNORED_NAMES = {"ETSGLOBAL"};
    return IGNORED_NAMES.find(name) == IGNORED_NAMES.end();
}

bool IsDefinedClassOrStruct(ir::AstNode *preNode)
{
    if (preNode == nullptr) {
        return false;
    }
    if (!preNode->IsClassDeclaration() && !preNode->IsETSStructDeclaration()) {
        return false;
    }
    if (preNode->IsClassDeclaration()) {
        return IsDeclarationNameDefined(GetNameFromDefinition(preNode->AsClassDeclaration()->Definition()));
    }
    if (preNode->IsETSStructDeclaration()) {
        return IsDeclarationNameDefined(GetNameFromDefinition(preNode->AsETSStructDeclaration()->Definition()));
    }
    return false;
}

std::vector<CompletionEntry> GetCompletionFromThisExpression(ir::AstNode *preNode, const std::string &triggerWord)
{
    if (preNode == nullptr || !preNode->IsThisExpression()) {
        return {};
    }
    auto def = GetDefinitionOfThisExpression(preNode);
    if (def == nullptr) {
        return {};
    }
    if (def->IsClassDefinition()) {
        return GetCompletionFromClassDefinition(def->AsClassDefinition(), triggerWord, false);
    }
    if (def->IsTSInterfaceDeclaration()) {
        return GetCompletionFromTSInterfaceDeclaration(def->AsTSInterfaceDeclaration(), triggerWord);
    }
    return {};
}

std::vector<CompletionEntry> GetCompletionsForDeclaration(ir::AstNode *decl, const std::string &triggerWord,
                                                          bool isStatic)
{
    if (decl->IsMethodDefinition()) {
        return GetCompletionFromMethodDefinition(decl->AsMethodDefinition(), triggerWord);
    }
    if (decl->IsTSInterfaceDeclaration()) {
        return GetCompletionFromTSInterfaceDeclaration(decl->AsTSInterfaceDeclaration(), triggerWord);
    }
    if (decl->IsClassDefinition()) {
        return GetCompletionFromClassDefinition(decl->AsClassDefinition(), triggerWord, isStatic);
    }
    if (decl->IsTSModuleDeclaration()) {
        return GetCompletionFromTSModuleDeclaration(decl->AsTSModuleDeclaration(), triggerWord);
    }
    return {};
}

void IntersectCompletions(std::vector<CompletionEntry> &completions, const std::vector<CompletionEntry> &currentItems)
{
    auto it = std::remove_if(completions.begin(), completions.end(), [&currentItems](const CompletionEntry &existing) {
        auto compare = [&existing](const CompletionEntry &current) {
            return existing.GetName() == current.GetName() &&
                   existing.GetCompletionKind() == current.GetCompletionKind() &&
                   existing.GetTypeSig() == current.GetTypeSig();
        };
        return std::find_if(currentItems.begin(), currentItems.end(), compare) == currentItems.end();
    });
    completions.erase(it, completions.end());
}

static bool IsStaticContext(ir::AstNode *node)
{
    if (node == nullptr || !node->IsIdentifier()) {
        return false;
    }
    auto var = node->AsIdentifier()->Variable();
    if (var == nullptr) {
        return false;
    }
    auto decl = var->Declaration();
    if (decl == nullptr) {
        return false;
    }
    auto defNode = decl->Node();
    if (defNode == nullptr) {
        return false;
    }
    return defNode->IsClassDeclaration() || defNode->IsClassDefinition() || defNode->IsTSModuleDeclaration() ||
           defNode->IsTSEnumDeclaration() || defNode->IsTSInterfaceDeclaration();
}

std::vector<CompletionEntry> GetPropertyCompletions(ir::AstNode *preNode, const std::string &triggerWord)
{
    std::vector<CompletionEntry> completions;
    if (preNode == nullptr) {
        return completions;
    }
    if (preNode->IsCallExpression() || preNode->IsMemberExpression()) {
        preNode = GetIndentifierFromCallExpression(preNode);
    }
    if (preNode == nullptr) {
        return completions;
    }
    if (preNode->IsThisExpression()) {
        return GetCompletionFromThisExpression(preNode, triggerWord);
    }
    if (!preNode->IsIdentifier()) {
        return completions;
    }

    auto decls = GetDefinitionFromIdentifier(preNode);
    if (decls.empty()) {
        return completions;
    }

    bool isStatic = IsStaticContext(preNode);

    bool isFirst = true;
    for (auto decl : decls) {
        auto currentItems = GetCompletionsForDeclaration(decl, triggerWord, isStatic);

        if (isFirst) {
            completions = std::move(currentItems);
            isFirst = false;
        } else {
            IntersectCompletions(completions, currentItems);
        }
        if (completions.empty()) {
            return {};
        }
    }
    return completions;
}

Request KeywordCompletionData(const std::string &input)
{
    return {
        CompletionDataKind::KEYWORDS,
        GetKeywordCompletions(input),
    };
}

std::string GetDeclName(const ir::AstNode *decl)
{
    switch (decl->Type()) {
        case ir::AstNodeType::IDENTIFIER:
            return decl->AsIdentifier()->ToString();
        case ir::AstNodeType::METHOD_DEFINITION:
            return decl->AsMethodDefinition()->Key()->AsIdentifier()->ToString();
        case ir::AstNodeType::CLASS_PROPERTY:
            return decl->AsClassProperty()->Key()->AsIdentifier()->ToString();
        case ir::AstNodeType::CLASS_DEFINITION:
            return std::string(decl->AsClassDefinition()->Ident()->Name());
        case ir::AstNodeType::TS_INTERFACE_DECLARATION:
            return std::string(decl->AsTSInterfaceDeclaration()->Id()->Name());
        default:
            return "";
    }
}

bool IsGlobalVar(const ir::AstNode *node)
{
    return node->IsClassProperty() && node->Parent()->IsClassDefinition() &&
           node->Parent()->AsClassDefinition()->IsGlobal();
}

bool IsVariableOfKind(const ir::Identifier *node, ir::VariableDeclaration::VariableDeclarationKind kind)
{
    /** A VariableDeclaration statement:
     *  - type: VariableDeclaration
     *      - type: VariableDeclarator
     *      - id
     *          - type: Identifier
     *  - Declaration kind
     */
    return node->Parent() != nullptr &&  // try to get the VariableDeclarator
           node->Parent()->IsVariableDeclarator() &&
           node->Parent()->Parent() != nullptr &&  // try to get the VariableDeclaration
           node->Parent()->Parent()->IsVariableDeclaration() &&
           node->Parent()->Parent()->AsVariableDeclaration()->Kind() == kind;
}

bool IsConstVar(const ir::AstNode *node)
{
    if (!node->IsIdentifier()) {
        return false;
    }
    return IsVariableOfKind(node->AsIdentifier(), ir::VariableDeclaration::VariableDeclarationKind::CONST);
}

bool IsLetVar(const ir::AstNode *node)
{
    if (!node->IsIdentifier()) {
        return false;
    }
    return IsVariableOfKind(node->AsIdentifier(), ir::VariableDeclaration::VariableDeclarationKind::LET);
}

bool IsValidDecl(const ir::AstNode *decl)
{
    return decl != nullptr && NodeHasTokens(decl) &&
           (decl->IsMethodDefinition() || IsLetVar(decl) || IsConstVar(decl) || IsGlobalVar(decl) ||
            decl->IsClassDefinition() || decl->IsTSInterfaceDeclaration());
}

CompletionEntry InitEntry(const ir::AstNode *decl)
{
    auto name = GetDeclName(decl);
    auto sortText = sort_text::GLOBALS_OR_KEYWORDS;
    auto kind = CompletionEntryKind::KEYWORD;
    auto insertText = name;
    if (IsLetVar(decl)) {
        kind = CompletionEntryKind::VARIABLE;
    } else if (IsConstVar(decl)) {
        kind = CompletionEntryKind::CONSTANT;
    } else if (IsGlobalVar(decl)) {
        auto globalDefinition = decl->Parent()->AsClassDefinition();
        auto cctor = globalDefinition->FindChild([&globalDefinition](ir::AstNode *child) {
            return child->IsClassStaticBlock() && child->Parent()->IsClassDefinition() &&
                   child->Parent()->AsClassDefinition() == globalDefinition;
        });
        if (cctor == nullptr) {
            return CompletionEntry(name, CompletionEntryKind::CONSTANT, std::string(sortText), name);
        }
        auto found = cctor->FindChild([&name](ir::AstNode *child) {
            return child->IsAssignmentExpression() && child->AsAssignmentExpression()->Left()->IsIdentifier() &&
                   child->AsAssignmentExpression()->Left()->AsIdentifier()->ToString() == name;
        });
        if (found != nullptr && !decl->AsClassProperty()->IsConst()) {
            // let variable in global definition need to be assigned in _$init$_ method
            kind = CompletionEntryKind::VARIABLE;
        } else {
            kind = CompletionEntryKind::CONSTANT;
        }
    } else if (decl->IsMethodDefinition()) {
        kind = CompletionEntryKind::FUNCTION;
        insertText = name + "()";
    } else if (decl->IsClassDefinition()) {
        kind = CompletionEntryKind::MODULE;
    }
    return CompletionEntry(name, kind, std::string(sortText), insertText);
}

bool IsAnnotationBeginning(std::string sourceCode, size_t pos)
{
    return sourceCode.at(pos - 1) == '@';
}

std::vector<CompletionEntry> GetCompletionFromPath(es2panda_Context *context, std::vector<CompletionEntry> &completions,
                                                   ir::ETSImportDeclaration *importDecl, ir::AstNode *node = nullptr)
{
    if (importDecl == nullptr || !importDecl->IsValid()) {
        return completions;
    }
    auto importText = importDecl->Source()->Str().Utf8();
    size_t lastSlashPos = importText.find_last_of("/\\");
    auto fileName = util::StringView(importText.substr(lastSlashPos + 1));
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    if (!ctx->parserProgram->DirectExternalSources().count(fileName)) {
        return completions;
    }
    auto specifiers = importDecl->Specifiers();
    std::unordered_set<std::string> hasImported;
    for (auto &specifier : specifiers) {
        if (specifier->IsImportSpecifier()) {
            auto name = specifier->AsImportSpecifier()->Imported()->AsIdentifier()->Name();
            hasImported.emplace(name.Utf8());
        }
    }
    auto &programs = ctx->parserProgram->DirectExternalSources().at(fileName);
    std::string specStr = "";
    if (node != nullptr && !node->AsIdentifier()->Name().Is(ERROR_LITERAL)) {
        specStr = node->AsIdentifier()->Name().Utf8();
    }
    for (auto &program : programs) {
        auto ans = GetExportsFromProgram(program, specStr);
        for (auto &entry : ans) {
            if (!hasImported.count(entry.GetName())) {
                completions.emplace_back(std::move(entry));
            }
        }
    }
    return completions;
}

std::vector<CompletionEntry> GetImportStatementPathCompletions(std::vector<CompletionEntry> &completions,
                                                               fs::path &searchDir, std::string &prefix)
{
    if (!fs::exists(searchDir) || !fs::is_directory(searchDir)) {
        return completions;
    }
    for (const auto &entry : fs::directory_iterator(searchDir)) {
        std::string name = entry.path().stem().string();
        bool isPrefix = prefix.empty() || (name.rfind(prefix, 0) == 0);
        if (IsDirectory(entry) && isPrefix) {
            completions.emplace_back(name, CompletionEntryKind::FOLDER, std::string(sort_text::GLOBALS_OR_KEYWORDS),
                                     name);
        } else if (IsRegularFile(entry) && entry.path().extension() == ".ets" && isPrefix) {
            completions.emplace_back(name, CompletionEntryKind::FILE, std::string(sort_text::GLOBALS_OR_KEYWORDS),
                                     name);
        }
    }
    return completions;
}

bool TryCompleteMissingFromKeyword(std::vector<CompletionEntry> &completions, ir::AstNode *node)
{
    auto parent = node->Parent();
    if (parent == nullptr || !parent->IsETSImportDeclaration()) {
        return false;
    }
    auto importDecl = parent->AsETSImportDeclaration();
    // When Source has a real range, the import path is already present
    // (e.g. `import {x} from 'f'`), so don't suggest `from` again.
    if (importDecl->Source()->Start().index != importDecl->Source()->End().index) {
        return false;
    }
    auto sourceStr = node->AsStringLiteral()->Str();
    std::string name = lexer::TokenToString(lexer::TokenType::KEYW_FROM);
    if (name.compare(0, sourceStr.Length(), sourceStr.Utf8()) == 0) {
        completions.emplace_back(name, CompletionEntryKind::KEYWORD, std::string(sort_text::GLOBALS_OR_KEYWORDS), name);
    }
    return true;
}

std::vector<CompletionEntry> GetImportStatementCompletions(es2panda_Context *context, ir::AstNode *node, size_t pos)
{
    std::vector<CompletionEntry> completions;
    if (node->IsETSImportDeclaration()) {
        auto importDecl = node->AsETSImportDeclaration();
        auto source = GetCurrentTokenValueImpl(context, pos, importDecl);
        if (source.find('}') == std::string::npos) {
            // When the cursor is positioned within the {} brackets, like "import {a,} from './xxx'".
            // Get completion of export var from import path
            return GetCompletionFromPath(context, completions, importDecl);
        }
        auto sourceStr = importDecl->Source()->Str();
        // Handle the input code "import {x} from " again reminder "from" keyword.
        if (sourceStr.Is(ERROR_LITERAL)) {
            return completions;
        }
        std::string name = lexer::TokenToString(lexer::TokenType::KEYW_FROM);
        // 1.If input code "import {x} ", the sourceStr is not "ERROR_LITERAL", it need to complete the "from" keyword.
        // 2.If input code "import {x} A" case, the "from" keyword should not be completed
        // because the prefix does not match.
        if (name.compare(0, sourceStr.Length(), sourceStr.Utf8()) != 0) {
            return completions;
        }
        completions.emplace_back(name, CompletionEntryKind::KEYWORD, std::string(sort_text::GLOBALS_OR_KEYWORDS), name);
    } else if (node->IsStringLiteral()) {
        if (TryCompleteMissingFromKeyword(completions, node)) {
            return completions;
        }
        auto ctx = reinterpret_cast<public_lib::Context *>(context);
        auto importText = node->AsStringLiteral()->ToString();
        auto filePath = ctx->sourceFile->filePath;
        fs::path baseDir = fs::path(filePath).parent_path();
        fs::path importPath(importText);
        fs::path normalized = NormalizePath(baseDir / importPath);
        std::string prefix = importPath.filename().string();
        fs::path searchDir = NormalizePath(baseDir / importPath.parent_path());
        // Determine whether the scene ends with '/' or './xx'
        if (!importText.empty() && importText.back() == '/') {
            searchDir = normalized;
            prefix.clear();
        }
        return GetImportStatementPathCompletions(completions, searchDir, prefix);
    } else if (node->IsIdentifier()) {
        // Get completion of export var from import path
        auto parent = node->Parent();
        ir::ETSImportDeclaration *importDecl = nullptr;
        if (parent->IsETSImportDeclaration()) {
            importDecl = parent->AsETSImportDeclaration();
        } else if (parent->IsImportSpecifier() || parent->IsImportDefaultSpecifier() ||
                   parent->IsImportNamespaceSpecifier()) {
            importDecl = parent->Parent()->AsETSImportDeclaration();
        }
        return GetCompletionFromPath(context, completions, importDecl, node);
    }
    return completions;
}

std::vector<CompletionEntry> GetAnnotationCompletions(es2panda_Context *context, size_t pos,
                                                      ir::AstNode *node = nullptr)
{
    std::vector<CompletionEntry> completions;
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    auto ast = ctx->parserProgram->Ast();
    auto importStatements = std::vector<ir::AstNode *>();
    auto annotationDeclarations = std::vector<ir::AstNode *>();
    for (auto &statement : ast->Statements()) {
        if (statement != nullptr && statement->IsETSImportDeclaration()) {
            importStatements.push_back(statement);
        }
        if (statement != nullptr && statement->Range().end.index < pos && statement->IsAnnotationDeclaration()) {
            annotationDeclarations.push_back(statement);
        }
    }
    auto addAnnotationCompletion = [&completions, &node](const std::string &name) {
        if (node == nullptr ||
            (node->IsIdentifier() && name.find(node->AsIdentifier()->Name().Utf8()) != std::string::npos)) {
            completions.emplace_back(name, CompletionEntryKind::ANNOTATION, std::string(sort_text::GLOBALS_OR_KEYWORDS),
                                     name);
        }
    };
    for (auto &import : importStatements) {
        auto specifiers = import->AsETSImportDeclaration()->Specifiers();
        for (auto &specifier : specifiers) {
            std::string localName;
            ir::AstNode *decl = nullptr;
            if (specifier->IsImportSpecifier()) {
                localName = std::string(specifier->AsImportSpecifier()->Local()->AsIdentifier()->Name());
                decl = compiler::DeclarationFromIdentifier(specifier->AsImportSpecifier()->Imported());
            } else if (specifier->IsImportDefaultSpecifier()) {
                localName = std::string(specifier->AsImportDefaultSpecifier()->Local()->AsIdentifier()->Name());
                decl = compiler::DeclarationFromIdentifier(specifier->AsImportDefaultSpecifier()->Local());
            }
            if (decl != nullptr && decl->IsAnnotationDeclaration()) {
                addAnnotationCompletion(localName);
            }
        }
    }
    for (auto &annotation : annotationDeclarations) {
        auto annotationName = std::string(annotation->AsAnnotationDeclaration()->GetBaseName()->Name());
        addAnnotationCompletion(annotationName);
    }
    return completions;
}

varbinder::Scope *NearestScope(const ir::AstNode *ast)
{
    // A same function in ets2panda/compiler/lowering/util.cpp is not suitable here
    // We modify it to let it find the right scope of class and interface
    while (ast != nullptr && !ast->IsScopeBearer()) {
        ast = ast->Parent();
        if (ast->IsClassDefinition() || ast->IsTSInterfaceDeclaration()) {
            ast = ast->Parent();
        }
    }

    return ast == nullptr ? nullptr : ast->Scope();
}

void GetIdentifiersInScope(const varbinder::Scope *scope, size_t position, ArenaVector<ir::AstNode *> &results)
{
    if (scope->Node() == nullptr) {
        return;
    }
    auto checkFunc = [scope, position](ir::AstNode *child) -> bool {
        return child->End().index < position && NodeHasTokens(child) && NearestScope(child) == scope &&
               child->IsIdentifier();
    };
    FindAllChild(scope->Node(), checkFunc, results);
}

auto GetDeclByScopePath(ArenaVector<varbinder::Scope *> &scopePath, size_t position, ArenaAllocator *allocator)
{
    auto hashFunc = [](const ir::AstNode *node) {
        static std::hash<std::string> strHasher;
        return strHasher(GetDeclName(node));
    };
    auto equalFunc = [](const ir::AstNode *lhs, const ir::AstNode *rhs) {
        return GetDeclName(lhs) == GetDeclName(rhs);
    };
    auto decls = ArenaUnorderedSet<ir::AstNode *, decltype(hashFunc), decltype(equalFunc)>(0, hashFunc, equalFunc,
                                                                                           allocator->Adapter());
    for (auto scope : scopePath) {
        auto nodes = ArenaVector<ir::AstNode *>(allocator->Adapter());
        GetIdentifiersInScope(scope, position, nodes);
        for (auto node : nodes) {
            auto decl = compiler::DeclarationFromIdentifier(node->AsIdentifier());
            if (IsValidDecl(decl)) {
                decls.insert(decl);
            }
        }
    }
    return decls;
}

// Support: global variables, local variables, functions, keywords
std::vector<CompletionEntry> GetGlobalCompletions(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    auto allocator = ctx->allocator;
    if (allocator == nullptr) {
        return {};
    }
    auto precedingToken = FindPrecedingToken(position, ctx->parserProgram->Ast(), allocator);
    if (precedingToken == nullptr) {
        return {};
    }
    auto scopePath = BuildScopePath(compiler::NearestScope(precedingToken), allocator);
    auto prefix = GetCurrentTokenValueImpl(context, position, precedingToken);
    auto decls = GetDeclByScopePath(scopePath, position, allocator);
    std::vector<CompletionEntry> completions;

    for (auto decl : decls) {
        auto entry = InitEntry(decl);
        if (entry.GetName().find(prefix) != 0) {
            continue;
        }
        entry = ProcessAutoImportForEntry(entry);
        completions.push_back(entry);
    }

    auto keywordCompletions = GetKeywordCompletions(prefix);
    completions.insert(completions.end(), keywordCompletions.begin(), keywordCompletions.end());
    auto systemInterfaceCompletions = GetSystemInterfaceCompletions(prefix, ctx->parserProgram);
    completions.insert(completions.end(), systemInterfaceCompletions.begin(), systemInterfaceCompletions.end());

    return completions;
}

ArenaVector<varbinder::Scope *> BuildScopePath(varbinder::Scope *startScope, ArenaAllocator *allocator)
{
    ArenaVector<varbinder::Scope *> scopePath(allocator->Adapter());
    for (auto scope = startScope; scope != nullptr; scope = scope->Parent()) {
        scopePath.push_back(scope);
    }
    return scopePath;
}

CompletionEntry ProcessAutoImportForEntry(CompletionEntry &entry)
{
    auto dataOpt = entry.GetCompletionEntryData();
    if (!dataOpt.has_value()) {
        return entry;
    }

    auto config = GetArkTsConfigFromFile(dataOpt->GetFileName());
    if (config == nullptr) {
        return entry;
    }

    auto autoImportData = GetAutoImportCompletionEntry(&dataOpt.value(), config, entry.GetName());
    if (!autoImportData.has_value()) {
        return entry;
    }

    return CompletionEntry(entry.GetName(), entry.GetCompletionKind(), entry.GetSortText(), entry.GetInsertText(),
                           autoImportData);
}

bool IsNameForDelaration(ir::AstNode *node)
{
    return node->Parent() != nullptr &&
           (node->Parent()->IsClassDefinition() || node->Parent()->IsTSInterfaceDeclaration() ||
            node->Parent()->IsMethodDefinition());
}

bool IsTokenAfterPoint(ir::AstNode *precedingToken)
{
    return precedingToken->IsMemberExpression() || precedingToken->IsTSQualifiedName();
}

bool IsStandaloneNode(ir::AstNode *node)
{
    return node->IsBlockStatement() || node->IsCallExpression();
}

ir::AstNode *GetMemberExprOfIdentifier(ir::AstNode *memberExp)
{
    while (memberExp->Parent() != nullptr && !IsTokenAfterPoint(memberExp) && !IsStandaloneNode(memberExp)) {
        memberExp = memberExp->Parent();
    }
    return memberExp;
}

std::vector<CompletionEntry> GetPropertyCompletionsWithValidPoint(ir::AstNode *precedingToken,
                                                                  const std::string &triggerWord)
{
    auto memberExp = precedingToken;
    while (memberExp->Parent() != nullptr && !IsTokenAfterPoint(memberExp)) {
        memberExp = memberExp->Parent();
    }
    if (!IsTokenAfterPoint(memberExp)) {
        return {};
    }
    if (memberExp->IsMemberExpression()) {
        precedingToken = memberExp->AsMemberExpression()->Object();
        return GetPropertyCompletions(precedingToken, triggerWord);
    }
    if (memberExp->IsTSQualifiedName()) {
        precedingToken = memberExp->AsTSQualifiedName()->Left();
        return GetPropertyCompletions(precedingToken, triggerWord);
    }
    return {};
}

bool IsInETSImportStatement(size_t pos, ir::AstNode *node)
{
    auto parent = node->Parent();
    ir::ETSImportDeclaration *importDecl = nullptr;
    if (node->IsETSImportDeclaration()) {
        importDecl = node->AsETSImportDeclaration();
    } else if (parent == nullptr) {
        return false;
    } else if (node->IsStringLiteral() && parent->IsETSImportDeclaration()) {
        importDecl = parent->AsETSImportDeclaration();
    } else if (node->IsIdentifier()) {
        if (parent->IsETSImportDeclaration()) {
            importDecl = parent->AsETSImportDeclaration();
        } else if (parent->IsImportSpecifier() || parent->IsImportDefaultSpecifier() ||
                   parent->IsImportNamespaceSpecifier()) {
            importDecl = parent->Parent()->AsETSImportDeclaration();
        }
    }
    if (importDecl == nullptr) {
        return false;
    }
    size_t end = importDecl->Range().end.index;
    if (pos > end) {
        return false;
    }
    return true;
}

std::vector<CompletionEntry> GetCompletionsAtPositionImpl(es2panda_Context *context, size_t pos)
{
    if (context == nullptr) {
        return {};
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    if (ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return {};
    }
    auto allocator = ctx->allocator;
    std::string sourceCode(ctx->parserProgram->SourceCode());
    if (IsAnnotationBeginning(sourceCode, pos)) {
        return GetAnnotationCompletions(context, pos);  // need to filter annotation
    }
    // Current GetPrecedingPosition cannot get token of "obj." with position.
    auto precedingToken = FindPrecedingToken(pos, ctx->parserProgram->Ast(), allocator);
    if (precedingToken == nullptr) {
        return {};
    }
    if (IsInETSImportStatement(pos, precedingToken)) {
        return GetImportStatementCompletions(context, precedingToken, pos);
    }
    if (IsAnnotationBeginning(sourceCode, precedingToken->Start().index)) {
        return GetAnnotationCompletions(context, pos, precedingToken);  // need to filter annotation
    }
    auto triggerValue = GetCurrentTokenValueImpl(context, pos, precedingToken);
    if (IsEndWithValidPoint(triggerValue)) {
        return GetPropertyCompletionsWithValidPoint(precedingToken, "");
    }
    auto memberExpr = GetMemberExprOfIdentifier(precedingToken);
    if (IsEndWithToken(precedingToken, triggerValue) && IsTokenAfterPoint(memberExpr)) {
        return GetPropertyCompletionsWithValidPoint(precedingToken, triggerValue);
    }
    if (IsNameForDelaration(precedingToken)) {
        return {};
    }
    return GetGlobalCompletions(context, pos);
}

std::optional<CompletionEntryData> GetAutoImportCompletionEntry(ark::es2panda::lsp::CompletionEntryData *data,
                                                                const std::shared_ptr<ArkTsConfig> &config,
                                                                const std::string &name)
{
    const char *fileName = data->GetFileName();
    if (fileName == nullptr || std::strlen(fileName) == 0) {
        return std::nullopt;
    }
    if (config == nullptr) {
        return std::nullopt;
    }
    return CompletionEntryDataToOriginInfo(data, config, name);
}

std::optional<CompletionEntryData> CompletionEntryDataToOriginInfo(ark::es2panda::lsp::CompletionEntryData *data,
                                                                   const std::shared_ptr<ArkTsConfig> &config,
                                                                   const std::string &name)
{
    if (IsCompletionEntryDataResolved(data, config) == true) {
        return CompletionEntryData(data->GetFileName(), data->GetNamedExport(), data->GetImportDeclaration(), name,
                                   ResolutionStatus::RESOLVED);
    }
    if (IsCompletionEntryDataResolved(data, config) == false) {
        return CompletionEntryData(data->GetFileName(), data->GetNamedExport(), data->GetImportDeclaration(), name,
                                   ResolutionStatus::UNRESOLVED);
    }
    return std::nullopt;
}

bool StartsWith(const std::string &str, const std::string &prefix)
{
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

std::optional<bool> IsCompletionEntryDataResolved(ark::es2panda::lsp::CompletionEntryData *data,
                                                  const std::shared_ptr<ArkTsConfig> &config)
{
    auto importDecl = data->GetImportDeclaration();
    if (importDecl.length() == 0) {
        return std::nullopt;
    }
    if (StartsWith(importDecl, "./") || StartsWith(importDecl, "../")) {
        return true;
    }

    const char slash = static_cast<char>(lexer::LEX_CHAR_SLASH);
    auto pos = importDecl.find(slash);
    const std::string importSub = importDecl.substr(0, pos);
    auto configPaths = config->Paths();
    if (configPaths.count(importSub) != 0) {
        return false;
    }

    return std::nullopt;
}

std::shared_ptr<ArkTsConfig> GetArkTsConfigFromFile(const char *fileName)
{
    Initializer initializer = Initializer();

    auto ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED);
    auto config = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->config->options->ArkTSConfig();
    initializer.DestroyContext(ctx);

    return config;
}
}  // namespace ark::es2panda::lsp