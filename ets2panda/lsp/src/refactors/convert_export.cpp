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

#include <string_view>
#include "refactors/convert_export.h"
#include "es2panda.h"
#include "parser/program/program.h"
#include "public/es2panda_lib.h"
#include "es2panda.h"
#include "refactors/refactor_types.h"
#include "services/text_change/text_change_context.h"
#include "refactor_provider.h"
#include "internal_api.h"
#include <string>
#include <unordered_set>
#include <vector>
#if defined(__cpp_lib_filesystem)
#include <filesystem>
namespace fs = std::filesystem;
#elif defined(__cpp_lib_experimental_filesystem)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "No filesystem support detected"
#endif

namespace ark::es2panda::lsp {
ConvertExportRefactor::ConvertExportRefactor()
{
    AddKind(std::string(TO_NAMED_EXPORT_ACTION.kind));
    AddKind(std::string(TO_DEFAULT_EXPORT_ACTION.kind));
}

// Helper: remove the last extension from filename
std::string RemoveExtension(const fs::path &p)
{
    std::string name = p.filename().string();
    auto pos = name.rfind('.');
    if (pos != std::string::npos) {
        name = name.substr(0, pos);
    }
    return name;
}

static inline fs::path GetCanonicalPath(const fs::path &path)
{
#if defined(__cpp_lib_filesystem)
    std::error_code ec;
    auto canonical = fs::weakly_canonical(path, ec);
    if (ec) {
        return fs::absolute(path);
    }
    return canonical;
#else
    return fs::absolute(path);
#endif
}

// Check if imported file matches export file (ignoring extension)
bool IsMatchingImport(const fs::path &importedPath, const fs::path &exportPath)
{
    fs::path canonicalImported = GetCanonicalPath(importedPath);
    fs::path canonicalExport = GetCanonicalPath(exportPath);
    return RemoveExtension(canonicalImported) == RemoveExtension(canonicalExport);
}
static bool HandleImportChild(ir::AstNode *childNode, const fs::path &filePath, const std::string &exportPath,
                              public_lib::Context *ctx, std::vector<std::string> &imports)
{
    if (!childNode->IsStringLiteral()) {
        return false;
    }

    std::string importedFileStr = childNode->AsStringLiteral()->ToString();
    if (importedFileStr.empty()) {
        return false;
    }

    fs::path importedPath = filePath.parent_path();
    importedPath.append(importedFileStr);

    if (IsMatchingImport(importedPath, exportPath)) {
        imports.emplace_back(ctx->parserProgram->SourceFilePath().Utf8().data());
    }

    return true;
}
void CollectImportsFromFile(const fs::path &filePath, std::unordered_set<std::string> &visited,
                            std::vector<std::string> &imports, std::string &exportPath)
{
    std::error_code ec;
    if (!fs::exists(filePath, ec) || visited.count(filePath.string()) != 0) {
        return;
    }
    visited.insert(filePath.string());
    Initializer initializer;
#if defined(_WIN32)
    std::string utf8Path = filePath.u8string();  // Windows
#else
    std::string utf8Path = filePath.string();  // POSIX
#endif
    auto context = initializer.CreateContext(utf8Path.c_str(), ES2PANDA_STATE_CHECKED);
    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return;
    }
    ctx->parserProgram->Ast()->Iterate([&](ir::AstNode *node) {
        if (!node->IsETSImportDeclaration()) {
            return;
        }
        (void)node->FindChild(
            [&](ir::AstNode *childNode) { return HandleImportChild(childNode, filePath, exportPath, ctx, imports); });
    });
}

static bool IsValidSourceFile(const fs::directory_entry &entry)
{
#if defined(__cpp_lib_filesystem)
    if (!entry.is_regular_file()) {
        return false;
    }
#else
    if (!fs::is_regular_file(entry.status())) {
        return false;
    }
#endif

    std::string ext = entry.path().extension().string();
    if (ext.empty()) {
        return false;
    }
    if (ext.front() == '.') {
        ext.erase(0, 1);  // ".ets" -> "ets"
    }
    // string -> enum
    auto scriptExt = util::gen::extension::FromString(ext);

    return scriptExt == ScriptExtension::ETS;
}

void CollectImportsFromFolder(util::StringView folder, std::unordered_set<std::string> &visited,
                              std::vector<std::string> &imports, std::string &exportPath)
{
    fs::path folderPath(folder.Utf8());
    std::error_code ec;

    if (!fs::exists(folderPath, ec) || !fs::is_directory(folderPath, ec)) {
        return;
    }
    for (auto const &entry : fs::directory_iterator(folderPath, ec)) {
        if (IsValidSourceFile(entry)) {
            CollectImportsFromFile(entry.path(), visited, imports, exportPath);
        }
    }
}

std::vector<ApplicableRefactorInfo> ConvertExportRefactor::GetAvailableActions(const RefactorContext &refContext) const
{
    ApplicableRefactorInfo applicableRef;
    std::vector<ApplicableRefactorInfo> res;
    es2panda_Context *context = refContext.context;
    size_t position = refContext.span.pos;

    if (!IsKind(refContext.kind)) {
        return res;
    }

    auto node = GetTouchingToken(context, position, false);
    if (node == nullptr) {
        return res;
    }
    auto cb = [](ir::AstNode *ancestorNode) { return ancestorNode->IsDefaultExported() || ancestorNode->IsExported(); };
    auto ancestor = FindAncestor(node, cb);
    if (ancestor == nullptr) {
        return res;
    }
    if (ancestor->IsDefaultExported()) {
        applicableRef.name = refactor_name::CONVERT_EXPORT_REFACTOR_NAME;
        applicableRef.description = std::string(TO_NAMED_EXPORT_ACTION.description);
        applicableRef.action.kind = std::string(TO_NAMED_EXPORT_ACTION.kind);
        applicableRef.action.name = std::string(TO_NAMED_EXPORT_ACTION.name);
        applicableRef.action.description = std::string(TO_NAMED_EXPORT_ACTION.description);
        res.push_back(applicableRef);
    } else if (ancestor->IsExported()) {
        applicableRef.name = refactor_name::CONVERT_EXPORT_REFACTOR_NAME;
        applicableRef.description = std::string(TO_DEFAULT_EXPORT_ACTION.description);
        applicableRef.action.kind = std::string(TO_DEFAULT_EXPORT_ACTION.kind);
        applicableRef.action.name = std::string(TO_DEFAULT_EXPORT_ACTION.name);
        applicableRef.action.description = std::string(TO_DEFAULT_EXPORT_ACTION.description);
        res.push_back(applicableRef);
    }

    return res;
}
void ConvertExportRefactor::HandleToNamedExport(ChangeTracker &tracker, public_lib::Context *ctxContent,
                                                ir::AstNode *exportIdentifier) const
{
    tracker.ReplaceRangeWithText(ctxContent->sourceFile,
                                 {exportIdentifier->Start().index, exportIdentifier->Start().index + 7}, "");
}

void ConvertExportRefactor::HandleToDefaultExport(ChangeTracker &tracker, public_lib::Context *ctxContent,
                                                  ir::AstNode *exportIdentifier) const
{
    size_t insertPos = exportIdentifier->Start().index - 1;
    tracker.ReplaceRangeWithText(ctxContent->sourceFile, {insertPos, insertPos}, " default ");
}

void ConvertExportRefactor::ApplyExportEdits(ChangeTracker &tracker, public_lib::Context *ctxContent,
                                             ir::AstNode *ancestor, ir::AstNode *exportIdentifier,
                                             const std::string &actionName) const
{
    if (ancestor->IsDefaultExported() && actionName == TO_NAMED_EXPORT_ACTION.name) {
        HandleToNamedExport(tracker, ctxContent, exportIdentifier);
    } else if (ancestor->IsExported() && actionName == TO_DEFAULT_EXPORT_ACTION.name) {
        HandleToDefaultExport(tracker, ctxContent, exportIdentifier);
    }
}

void ConvertExportRefactor::ReplaceWithDefaultImport(ChangeTracker &tracker, public_lib::Context *importContent,
                                                     const ReferenceInfo &ref, const std::string &exportName,
                                                     const std::string &exportFile) const
{
    std::string replacedText = "import " + exportName + " from \"" + exportFile + "\";";
    tracker.ReplaceRangeWithText(importContent->sourceFile,
                                 {static_cast<size_t>(ref.start), static_cast<size_t>(ref.start + ref.length)},
                                 replacedText);
}

void ConvertExportRefactor::ReplaceWithNamedImport(ChangeTracker &tracker, public_lib::Context *importContent,
                                                   const ReferenceInfo &ref, const std::string &exportName,
                                                   const std::string &exportFile) const
{
    std::string replacedText = "import { " + exportName + " } from \"" + exportFile + "\";";
    tracker.ReplaceRangeWithText(importContent->sourceFile,
                                 {static_cast<size_t>(ref.start), static_cast<size_t>(ref.start + ref.length)},
                                 replacedText);
}

void ConvertExportRefactor::HandleToDefaultImport(ChangeTracker &tracker, public_lib::Context *importContent,
                                                  const ReferenceInfo &ref, const std::string &exportName,
                                                  const std::string &exportFile) const
{
    ReplaceWithDefaultImport(tracker, importContent, ref, exportName, exportFile);
}

void ConvertExportRefactor::HandleToNamedImport(ChangeTracker &tracker, public_lib::Context *importContent,
                                                const ReferenceInfo &ref, const std::string &exportName,
                                                const std::string &exportFile) const
{
    ReplaceWithNamedImport(tracker, importContent, ref, exportName, exportFile);
}

void ConvertExportRefactor::ProcessReference(ChangeTracker &tracker, public_lib::Context *importContent,
                                             const ReferenceInfo &ref, ir::AstNode *ancestor,
                                             const std::string &actionName, const std::string &exportName,
                                             const std::string &exportFile) const
{
    if (ancestor->IsExported() && actionName == TO_DEFAULT_EXPORT_ACTION.name) {
        HandleToDefaultImport(tracker, importContent, ref, exportName, exportFile);
    } else if (ancestor->IsDefaultExported() && actionName == TO_NAMED_EXPORT_ACTION.name) {
        HandleToNamedImport(tracker, importContent, ref, exportName, exportFile);
    }
}

void ConvertExportRefactor::ApplyImportEdits(ChangeTracker &tracker, const std::vector<std::string> &importers,
                                             const std::string &exportFile, const std::string &exportName,
                                             ir::AstNode *ancestor, const std::string &actionName) const
{
    for (auto &f : importers) {
        Initializer init;
        auto *importCtx = init.CreateContext(f.c_str(), ES2PANDA_STATE_CHECKED);
        if (importCtx == nullptr) {
            continue;
        }

        auto *importContent = reinterpret_cast<public_lib::Context *>(importCtx);
        auto fileRefs = GetFileReferencesImpl(importCtx, exportFile.c_str(), importContent->parserProgram->IsPackage());

        for (auto &ref : fileRefs.referenceInfos) {
            ProcessReference(tracker, importContent, ref, ancestor, actionName, exportName, exportFile);
        }
    }
}

// Check if node is export-related
bool IsExportNode(ir::AstNode *node)
{
    return node->IsDefaultExported() || node->IsExported();
}

// Try to get export identifier and its name from an ancestor node
std::pair<ir::AstNode *, std::string> GetExportIdentifier(ir::AstNode *ancestor)
{
    ir::AstNode *identifier = nullptr;
    std::string name;
    if (ancestor->IsFunctionDeclaration() || ancestor->IsMethodDefinition() || ancestor->IsClassDeclaration() ||
        ancestor->IsVariableDeclaration()) {
        ancestor->FindChild([&](ir::AstNode *node) {
            if (node->IsIdentifier()) {
                identifier = node;
                name = node->AsIdentifier()->Name().Utf8();
                return true;
            }
            return false;
        });
    }
    return {identifier, name};
}

std::unique_ptr<RefactorEditInfo> ConvertExportRefactor::GetEditsForAction(const RefactorContext &refContext,
                                                                           const std::string &actionName) const
{
    auto refactorEditInfo = std::make_unique<RefactorEditInfo>();
    es2panda_Context *ctx = refContext.context;
    size_t pos = refContext.span.pos;

    auto *token = GetTouchingToken(ctx, pos, false);
    if (token == nullptr) {
        return refactorEditInfo;
    }

    ir::AstNode *ancestor = FindAncestor(token, IsExportNode);
    if (ancestor == nullptr) {
        return refactorEditInfo;
    }

    auto result = GetExportIdentifier(ancestor);
    ir::AstNode *exportIdentifier = result.first;
    std::string exportName = result.second;
    if (exportIdentifier == nullptr) {
        return refactorEditInfo;
    }

    auto ctxContent = reinterpret_cast<public_lib::Context *>(ctx);
    auto prog = ctxContent->parserProgram;
    std::string exportFile = ctxContent->sourceFile->filePath.data();

    std::unordered_set<std::string> visited;
    std::vector<std::string> imports;
    CollectImportsFromFolder(prog->SourceFileFolder(), visited, imports, exportFile);

    TextChangesContext textChangesContext = *refContext.textChangesContext;
    auto changes = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        ApplyExportEdits(tracker, ctxContent, ancestor, exportIdentifier, actionName);
        ApplyImportEdits(tracker, imports, exportFile, exportName, ancestor, actionName);
    });

    refactorEditInfo->SetFileTextChanges(changes);
    return refactorEditInfo;
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<ConvertExportRefactor> g_convertExportRefactorRegister("ConvertExportRefactor");

}  // namespace ark::es2panda::lsp
