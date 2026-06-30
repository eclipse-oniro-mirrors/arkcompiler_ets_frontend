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

#ifndef ES2PANDA_LSP_INCLUDE_SERVICES_IMPORT_UTILS_H
#define ES2PANDA_LSP_INCLUDE_SERVICES_IMPORT_UTILS_H

#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ark::es2panda::ir {
class AstNode;
class ETSImportDeclaration;
class Identifier;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::parser {
class Program;
}  // namespace ark::es2panda::parser

namespace ark::es2panda::lsp {

bool IsLineBreak(char ch);

bool IsIdentifierNameEqual(const ir::Identifier *identifier, const std::string &symbolName);

bool DoesImportSpecifierMatchSymbol(const ir::AstNode *specifier, const std::string &symbolName);

bool IsSymbolAlreadyImported(const parser::Program *program, const std::string &symbolName);

std::string NormalizeImportModulePath(const std::string &rawPath);

bool IsImportDeclarationFromModule(const ir::ETSImportDeclaration *importDecl, const std::string &moduleName);

struct ImportDeclarationShape {
    bool hasNamespace = false;
    std::optional<std::string> defaultImport;
    std::vector<std::pair<std::string, std::string>> namedImports;
};

ImportDeclarationShape GetImportDeclarationShape(const ir::ETSImportDeclaration *importDecl);

std::string FormatImportSpecifier(const std::pair<std::string, std::string> &specifier);

bool TryApplySymbolToImportShape(ImportDeclarationShape *shape, const std::string &symbolName, bool isDefaultImport);

std::vector<std::pair<std::string, std::string>> DeduplicateNamedImports(
    const std::vector<std::pair<std::string, std::string>> &namedImports);

std::string BuildImportDeclarationText(const std::optional<std::string> &defaultImport,
                                       const std::vector<std::pair<std::string, std::string>> &namedImports,
                                       const std::string &sourceModule);

std::string BuildImportDeclarationTextWithAddedSymbol(const ir::ETSImportDeclaration *importDecl,
                                                      const std::string &symbolName, bool isDefaultImport);

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

ImportMergePriority GetImportMergePriority(const ImportDeclarationShape &shape, bool isDefaultImport);

ir::ETSImportDeclaration *FindMergeableImportDeclarationForModule(parser::Program *program,
                                                                  const std::string &moduleName, bool isDefaultImport);

size_t GetImportInsertPosition(const parser::Program *program);

size_t AdjustInsertPositionForUseStaticDirective(size_t insertPos, std::string_view source);

std::string BuildImportInsertText(std::string_view source, size_t insertPos, const std::string &symbolName,
                                  const std::string &moduleName, bool isDefaultImport);

}  // namespace ark::es2panda::lsp

#endif  // ES2PANDA_LSP_INCLUDE_SERVICES_IMPORT_UTILS_H
