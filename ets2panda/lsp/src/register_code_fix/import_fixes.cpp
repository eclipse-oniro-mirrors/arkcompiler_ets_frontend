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

#include <algorithm>
#include <string_view>
#include <string>
#include <unordered_set>
#include <vector>

#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/completions.h"
#include "lsp/include/register_code_fix/import_fixes.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

namespace ark::es2panda::lsp {
using codefixes::ADD_LOCAL_VARIABLE;
using codefixes::ADD_LOCAL_VARIABLE_FOR_CLASS;

namespace {
constexpr int G_IMPORT_FIXES_CODE = 1005;
constexpr const char *G_IMPORT_FIXES_ID = "ImportFixes";

bool EndsWith(const std::string &value, const std::string &suffix)
{
    return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string NormalizeImportModulePath(const std::string &rawPath)
{
    if (rawPath.empty()) {
        return "";
    }

    std::string modulePath = rawPath;
    auto splitPos = modulePath.find_last_of("/\\");
    if (splitPos != std::string::npos) {
        modulePath = modulePath.substr(splitPos + 1);
    }

    if (EndsWith(modulePath, ".d.ets")) {
        modulePath.resize(modulePath.size() - std::string(".d.ets").size());
    } else if (EndsWith(modulePath, ".ets")) {
        modulePath.resize(modulePath.size() - std::string(".ets").size());
    }

    return modulePath;
}

std::string GetUnresolvedIdentifierName(es2panda_Context *context, size_t position)
{
    auto *token = GetTouchingTokenRightMatch(context, position, false);
    if (token == nullptr || !token->IsIdentifier()) {
        return "";
    }

    return std::string(token->AsIdentifier()->Name());
}

bool IsLineBreak(char ch)
{
    return ch == '\n' || ch == '\r';
}

bool IsIdentifierNameEqual(const ir::Identifier *identifier, const std::string &symbolName)
{
    return identifier != nullptr && std::string(identifier->Name()) == symbolName;
}

bool DoesImportSpecifierMatchSymbol(const ir::AstNode *specifier, const std::string &symbolName)
{
    if (specifier == nullptr) {
        return false;
    }

    if (specifier->IsImportSpecifier()) {
        auto *importSpecifier = specifier->AsImportSpecifier();
        return IsIdentifierNameEqual(importSpecifier->Local(), symbolName) ||
               IsIdentifierNameEqual(importSpecifier->Imported(), symbolName);
    }

    if (specifier->IsImportDefaultSpecifier()) {
        return IsIdentifierNameEqual(specifier->AsImportDefaultSpecifier()->Local(), symbolName);
    }

    if (specifier->IsImportNamespaceSpecifier()) {
        return IsIdentifierNameEqual(specifier->AsImportNamespaceSpecifier()->Local(), symbolName);
    }

    return false;
}

bool IsSymbolAlreadyImported(const parser::Program *program, const std::string &symbolName)
{
    if (program == nullptr || program->Ast() == nullptr) {
        return false;
    }

    for (auto *statement : program->Ast()->Statements()) {
        if (statement == nullptr || !statement->IsETSImportDeclaration()) {
            continue;
        }

        for (auto *specifier : statement->AsETSImportDeclaration()->Specifiers()) {
            if (DoesImportSpecifierMatchSymbol(specifier, symbolName)) {
                return true;
            }
        }
    }
    return false;
}

bool IsImportDeclarationFromModule(const ir::ETSImportDeclaration *importDecl, const std::string &moduleName)
{
    if (importDecl == nullptr || importDecl->Source() == nullptr) {
        return false;
    }

    auto importModule = std::string(importDecl->Source()->Str());
    return importModule == moduleName || NormalizeImportModulePath(importModule) == moduleName;
}

struct ImportDeclarationShape {
    bool hasNamespace = false;
    std::optional<std::string> defaultImport;
    std::vector<std::pair<std::string, std::string>> namedImports;
};

ImportDeclarationShape GetImportDeclarationShape(const ir::ETSImportDeclaration *importDecl)
{
    ImportDeclarationShape shape;
    if (importDecl == nullptr) {
        return shape;
    }

    for (auto *specifier : importDecl->Specifiers()) {
        if (specifier == nullptr) {
            continue;
        }

        if (specifier->IsImportNamespaceSpecifier()) {
            shape.hasNamespace = true;
            return shape;
        }

        if (specifier->IsImportDefaultSpecifier()) {
            auto *local = specifier->AsImportDefaultSpecifier()->Local();
            if (local != nullptr) {
                shape.defaultImport = std::string(local->Name());
            }
            continue;
        }

        if (specifier->IsImportSpecifier()) {
            auto *importSpecifier = specifier->AsImportSpecifier();
            if (importSpecifier->Imported() == nullptr || importSpecifier->Local() == nullptr) {
                continue;
            }
            shape.namedImports.emplace_back(std::string(importSpecifier->Imported()->Name()),
                                            std::string(importSpecifier->Local()->Name()));
        }
    }

    return shape;
}

std::string FormatImportSpecifier(const std::pair<std::string, std::string> &specifier)
{
    if (specifier.first == specifier.second) {
        return specifier.first;
    }
    return specifier.first + " as " + specifier.second;
}

bool TryApplySymbolToImportShape(ImportDeclarationShape *shape, const std::string &symbolName, bool isDefaultImport)
{
    if (shape == nullptr || shape->hasNamespace) {
        return false;
    }

    if (isDefaultImport) {
        if (shape->defaultImport.has_value()) {
            return false;
        }
        shape->defaultImport = symbolName;
        return true;
    }

    shape->namedImports.emplace_back(symbolName, symbolName);
    return true;
}

std::vector<std::pair<std::string, std::string>> DeduplicateNamedImports(
    const std::vector<std::pair<std::string, std::string>> &namedImports)
{
    std::unordered_set<std::string> dedupNamed;
    std::vector<std::pair<std::string, std::string>> normalizedNamedImports;
    normalizedNamedImports.reserve(namedImports.size());
    for (const auto &named : namedImports) {
        auto key = named.first + "#" + named.second;
        if (!dedupNamed.insert(key).second) {
            continue;
        }
        normalizedNamedImports.push_back(named);
    }
    return normalizedNamedImports;
}

std::string BuildImportDeclarationText(const std::optional<std::string> &defaultImport,
                                       const std::vector<std::pair<std::string, std::string>> &namedImports,
                                       const std::string &sourceModule)
{
    if (!defaultImport.has_value() && namedImports.empty()) {
        return "";
    }

    std::string text = "import ";
    if (defaultImport.has_value()) {
        text.append(defaultImport.value());
        if (!namedImports.empty()) {
            text.append(", ");
        }
    }
    if (!namedImports.empty()) {
        text.append("{ ");
        for (size_t i = 0; i < namedImports.size(); ++i) {
            if (i != 0) {
                text.append(", ");
            }
            text.append(FormatImportSpecifier(namedImports[i]));
        }
        text.append(" }");
    }

    text.append(" from '");
    text.append(sourceModule);
    text.append("';");
    return text;
}

std::string BuildImportDeclarationTextWithAddedSymbol(const ir::ETSImportDeclaration *importDecl,
                                                      const std::string &symbolName, bool isDefaultImport)
{
    if (importDecl == nullptr || importDecl->Source() == nullptr) {
        return "";
    }

    auto shape = GetImportDeclarationShape(importDecl);
    if (!TryApplySymbolToImportShape(&shape, symbolName, isDefaultImport)) {
        return "";
    }

    auto normalizedNamedImports = DeduplicateNamedImports(shape.namedImports);
    return BuildImportDeclarationText(shape.defaultImport, normalizedNamedImports,
                                      std::string(importDecl->Source()->Str()));
}

enum class ImportMergePriority : int {
    // Cannot merge into this declaration.
    NOT_MERGEABLE = -1,
    // Declaration has no default/named specifier yet, still mergeable but lowest priority.
    EMPTY_IMPORT = 1,
    // Declaration already has named imports, good merge target.
    HAS_NAMED_IMPORT = 3,
    // Declaration already has default import, best target when adding named import:
    // can form `import A, { b } from 'xxx'`.
    HAS_DEFAULT_IMPORT = 4,
};

ImportMergePriority GetImportMergePriority(const ImportDeclarationShape &shape, bool isDefaultImport)
{
    if (shape.hasNamespace) {
        return ImportMergePriority::NOT_MERGEABLE;
    }
    if (isDefaultImport) {
        if (shape.defaultImport.has_value()) {
            return ImportMergePriority::NOT_MERGEABLE;
        }
        return !shape.namedImports.empty() ? ImportMergePriority::HAS_NAMED_IMPORT : ImportMergePriority::EMPTY_IMPORT;
    }

    if (shape.defaultImport.has_value()) {
        return ImportMergePriority::HAS_DEFAULT_IMPORT;
    }
    if (!shape.namedImports.empty()) {
        return ImportMergePriority::HAS_NAMED_IMPORT;
    }
    return ImportMergePriority::EMPTY_IMPORT;
}

ir::ETSImportDeclaration *FindMergeableImportDeclarationForModule(parser::Program *program,
                                                                  const std::string &moduleName, bool isDefaultImport)
{
    if (program == nullptr || program->Ast() == nullptr) {
        return nullptr;
    }

    ir::ETSImportDeclaration *best = nullptr;
    auto bestPriority = ImportMergePriority::NOT_MERGEABLE;
    for (auto *statement : program->Ast()->Statements()) {
        if (statement == nullptr || !statement->IsETSImportDeclaration()) {
            continue;
        }

        auto *importDecl = statement->AsETSImportDeclaration();
        if (!IsImportDeclarationFromModule(importDecl, moduleName)) {
            continue;
        }

        auto shape = GetImportDeclarationShape(importDecl);
        auto priority = GetImportMergePriority(shape, isDefaultImport);
        if (static_cast<int>(priority) > static_cast<int>(bestPriority)) {
            bestPriority = priority;
            best = importDecl;
        }
    }

    return best;
}

size_t GetImportInsertPosition(const parser::Program *program)
{
    if (program->Ast() == nullptr) {
        return 0;
    }

    size_t firstStatementPos = 0;
    bool hasFirstStatement = false;
    size_t lastImportEndPos = 0;
    bool hasImport = false;

    for (auto *statement : program->Ast()->Statements()) {
        if (statement == nullptr) {
            continue;
        }
        if (!hasFirstStatement) {
            firstStatementPos = statement->Start().index;
            hasFirstStatement = true;
        }

        if (statement->IsETSImportDeclaration()) {
            lastImportEndPos = statement->End().index;
            hasImport = true;
            continue;
        }

        break;
    }

    if (hasImport) {
        return lastImportEndPos;
    }

    return hasFirstStatement ? firstStatementPos : 0;
}

bool IsUseStaticDirectiveAtFileTop(std::string_view source, size_t firstNonWhitespace)
{
    constexpr std::string_view SINGLE_QUOTE_USE_STATIC = "'use static'";
    constexpr std::string_view DOUBLE_QUOTE_USE_STATIC = "\"use static\"";

    if (firstNonWhitespace >= source.size()) {
        return false;
    }

    auto remains = source.substr(firstNonWhitespace);
    if (remains.rfind(SINGLE_QUOTE_USE_STATIC, 0) != 0 && remains.rfind(DOUBLE_QUOTE_USE_STATIC, 0) != 0) {
        return false;
    }

    size_t directiveLen = remains.rfind(SINGLE_QUOTE_USE_STATIC, 0) == 0 ? SINGLE_QUOTE_USE_STATIC.size()
                                                                         : DOUBLE_QUOTE_USE_STATIC.size();
    size_t i = firstNonWhitespace + directiveLen;
    while (i < source.size() && (source[i] == ' ' || source[i] == '\t')) {
        ++i;
    }
    return i == source.size() || source[i] == ';' || source[i] == '\r' || source[i] == '\n';
}

size_t GetInsertPositionAfterFirstLine(std::string_view source, size_t lineStart)
{
    size_t lineBreakPos = source.find_first_of("\r\n", lineStart);
    if (lineBreakPos == std::string_view::npos) {
        return source.size();
    }

    if (source[lineBreakPos] == '\r' && (lineBreakPos + 1U) < source.size() && source[lineBreakPos + 1U] == '\n') {
        return lineBreakPos + 2U;
    }
    return lineBreakPos + 1U;
}

size_t AdjustInsertPositionForUseStaticDirective(size_t originalPos, std::string_view source)
{
    size_t firstNonWhitespace = source.find_first_not_of(" \t\r\n");
    if (firstNonWhitespace == std::string_view::npos) {
        return originalPos;
    }
    if (originalPos != firstNonWhitespace) {
        return originalPos;
    }

    if (!IsUseStaticDirectiveAtFileTop(source, firstNonWhitespace)) {
        return originalPos;
    }

    return GetInsertPositionAfterFirstLine(source, firstNonWhitespace);
}

std::string BuildImportInsertText(std::string_view source, size_t insertPos, const std::string &symbolName,
                                  const std::string &moduleName, bool isDefaultImport)
{
    std::string text;

    if (insertPos > 0 && insertPos <= source.size() && !IsLineBreak(source[insertPos - 1U])) {
        text.push_back('\n');
    }

    text.append("import ");
    if (isDefaultImport) {
        text.append(symbolName);
    } else {
        text.append("{ ");
        text.append(symbolName);
        text.append(" }");
    }
    text.append(" from '");
    text.append(moduleName);
    text.push_back('\'');
    text.push_back(';');

    if (insertPos < source.size() && !IsLineBreak(source[insertPos])) {
        text.push_back('\n');
    }

    return text;
}

struct ImportCodeActionBuildContext {
    const std::string &unresolvedName;
    public_lib::Context *ctx;
    TextChangesContext textChangesContext;
    size_t insertPos;
    std::string_view sourceCode;
};

bool TryBuildImportCodeActionForCollectInfo(const ExternalApiCollectInfo &info, const std::string &moduleName,
                                            ImportCodeActionBuildContext &buildContext, CodeFixAction *codeAction)
{
    auto *mergeImportDecl =
        FindMergeableImportDeclarationForModule(buildContext.ctx->parserProgram, moduleName, info.isDefault);
    auto changes = ChangeTracker::With(buildContext.textChangesContext, [&](ChangeTracker &tracker) {
        if (mergeImportDecl != nullptr) {
            auto mergedImportText =
                BuildImportDeclarationTextWithAddedSymbol(mergeImportDecl, buildContext.unresolvedName, info.isDefault);
            if (mergedImportText.empty()) {
                return;
            }
            tracker.ReplaceRangeWithText(buildContext.ctx->sourceFile,
                                         {mergeImportDecl->Start().index, mergeImportDecl->End().index},
                                         mergedImportText);
            return;
        }

        auto insertText = BuildImportInsertText(buildContext.sourceCode, buildContext.insertPos,
                                                buildContext.unresolvedName, moduleName, info.isDefault);
        tracker.InsertText(buildContext.ctx->sourceFile, buildContext.insertPos, insertText);
    });
    if (changes.empty()) {
        return false;
    }

    codeAction->fixName = G_IMPORT_FIXES_ID;
    codeAction->description = info.isDefault
                                  ? ("Add import " + buildContext.unresolvedName + " from '" + moduleName + "'")
                                  : ("Add import {" + buildContext.unresolvedName + "} from '" + moduleName + "'");
    codeAction->additionalMessage = info.importDeclaration;
    codeAction->changes = std::move(changes);
    return true;
}

std::vector<CodeFixAction> BuildImportCodeActionsForCollectedInfos(
    const std::vector<ExternalApiCollectInfo> &collectedInfos, ImportCodeActionBuildContext &buildContext)
{
    std::vector<CodeFixAction> actions;
    std::unordered_set<std::string> importKeys;
    importKeys.reserve(collectedInfos.size());

    for (const auto &info : collectedInfos) {
        auto moduleName = NormalizeImportModulePath(info.importDeclaration);
        if (moduleName.empty()) {
            continue;
        }

        std::string importKey = moduleName + (info.isDefault ? "#default" : "#named");
        if (!importKeys.insert(importKey).second) {
            continue;
        }

        CodeFixAction codeAction;
        if (!TryBuildImportCodeActionForCollectInfo(info, moduleName, buildContext, &codeAction)) {
            continue;
        }

        actions.push_back(std::move(codeAction));
    }

    return actions;
}
}  // namespace

ImportFixes::ImportFixes()
{
    auto unresolvedCodes = ADD_LOCAL_VARIABLE.GetSupportedCodeNumbers();
    auto unresolvedClassCodes = ADD_LOCAL_VARIABLE_FOR_CLASS.GetSupportedCodeNumbers();

    std::vector<int> errorCodes {G_IMPORT_FIXES_CODE};
    errorCodes.insert(errorCodes.end(), unresolvedCodes.begin(), unresolvedCodes.end());
    errorCodes.insert(errorCodes.end(), unresolvedClassCodes.begin(), unresolvedClassCodes.end());
    std::sort(errorCodes.begin(), errorCodes.end());
    errorCodes.erase(std::unique(errorCodes.begin(), errorCodes.end()), errorCodes.end());

    SetErrorCodes(errorCodes);
    SetFixIds({G_IMPORT_FIXES_ID});
}

std::vector<CodeFixAction> ImportFixes::GetCodeActions(const CodeFixContext &context)
{
    if (context.context == nullptr) {
        return {};
    }

    auto unresolvedName = GetUnresolvedIdentifierName(context.context, context.span.start);
    if (unresolvedName.empty()) {
        return {};
    }

    auto collectedInfos = GetExternalApiCollectInfos(unresolvedName);
    if (collectedInfos.empty()) {
        return {};
    }

    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx->parserProgram == nullptr || ctx->sourceFile == nullptr) {
        return {};
    }
    if (IsSymbolAlreadyImported(ctx->parserProgram, unresolvedName)) {
        return {};
    }

    const std::string_view sourceCode = ctx->parserProgram->SourceCode();
    size_t insertPos = GetImportInsertPosition(ctx->parserProgram);
    insertPos = AdjustInsertPositionForUseStaticDirective(insertPos, sourceCode);

    ImportCodeActionBuildContext buildContext {
        unresolvedName, ctx, {context.host, context.formatContext, context.preferences}, insertPos, sourceCode};
    return BuildImportCodeActionsForCollectedInfos(collectedInfos, buildContext);
}

CombinedCodeActions ImportFixes::GetAllCodeActions(const CodeFixAllContext &codeFixAll)
{
    CombinedCodeActions combinedCodeActions;
    if (codeFixAll.fixId == G_IMPORT_FIXES_ID) {
    }

    return combinedCodeActions;
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<ImportFixes> g_importFixes(G_IMPORT_FIXES_ID);
}  // namespace ark::es2panda::lsp
