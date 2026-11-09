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

#ifndef ES2PANDA_LSP_INCLUDE_INTERNAL_API_H
#define ES2PANDA_LSP_INCLUDE_INTERNAL_API_H

#include <optional>
#include <string_view>
#include <vector>
#include "api.h"
#include "checker/types/type.h"
#include "ir/astNode.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
#include "util/eheap.h"

namespace ark::es2panda::util {
class ImportInfo;
}  // namespace ark::es2panda::util

namespace ark::es2panda {
class ArkTsConfig;
}  // namespace ark::es2panda

namespace ark::es2panda::checker {
class ExportClosureResolver;
}  // namespace ark::es2panda::checker

namespace ark::es2panda::lsp {

class Initializer {
public:
    explicit Initializer(bool isLogAwaible = false);

    ~Initializer();

    es2panda_Context *CreateContext(char const *fileName, es2panda_ContextState state, char const *fileSource = nullptr)
    {
        es2panda_Context *ctx = nullptr;
        if (fileSource != nullptr) {
            ctx = impl_->CreateContextFromString(cfg_, fileSource, fileName);
        } else {
            ctx = impl_->CreateContextFromFile(cfg_, fileName);
        }
        impl_->ProceedToState(ctx, state);
        return ctx;
    }

    es2panda_Context *CreateContextWithCache(char const *fileName, es2panda_ContextState state,
                                             std::vector<std::string> &texts)
    {
        es2panda_Context *ctx = nullptr;
        auto *globalContext = new ark::es2panda::public_lib::GlobalContext();
        auto globalAllocator = ark::es2panda::EHeap::NewAllocator().release();
        globalContext->cachedExternalPrograms.emplace(fileName, nullptr);
        globalContext->externalProgramAllocators.emplace(fileName, globalAllocator);
        globalContext->stdLibAllocator = globalAllocator;
        ctx = impl_->CreateCacheContextFromString(
            cfg_, texts[0].c_str(), fileName, reinterpret_cast<es2panda_GlobalContext *>(globalContext), true, true);
        impl_->ProceedToState(ctx, state);
        return ctx;
    }

    void DestroyContext(es2panda_Context *context)
    {
        impl_->DestroyContext(context);
    }

    const es2panda_DiagnosticKind *CreateDiagnosticKind(es2panda_Context *context, const char *dmessage,
                                                        es2panda_PluginDiagnosticType etype)
    {
        return impl_->CreateDiagnosticKind(context, dmessage, etype);
    }

    es2panda_SuggestionInfo *CreateSuggestionInfo(es2panda_Context *context, const es2panda_DiagnosticKind *kind,
                                                  const char **args, size_t argc, const char *substitutionCode,
                                                  const char *title, es2panda_SourceRange *range)
    {
        return impl_->CreateSuggestionInfo(context, kind, args, argc, substitutionCode, title, range);
    }

    es2panda_DiagnosticInfo *CreateDiagnosticInfo(es2panda_Context *context, const es2panda_DiagnosticKind *kind,
                                                  const char **args, size_t argc, es2panda_SourcePosition *pos)
    {
        return impl_->CreateDiagnosticInfo(context, kind, args, argc, pos);
    }

    es2panda_SourcePosition *CreateSourcePosition(es2panda_Context *context, size_t index, size_t line)
    {
        return impl_->CreateSourcePosition(context, index, line);
    }

    es2panda_SourceRange *CreateSourceRange(es2panda_Context *context, es2panda_SourcePosition *start,
                                            es2panda_SourcePosition *end)
    {
        return impl_->CreateSourceRange(context, start, end);
    }

    void LogDiagnosticWithSuggestion(es2panda_Context *context, const es2panda_DiagnosticInfo *diagnosticInfo,
                                     const es2panda_SuggestionInfo *suggestionInfo)
    {
        return impl_->LogDiagnosticWithSuggestion(context, diagnosticInfo, suggestionInfo);
    }

    void LogDiagnostic(es2panda_Context *context, const es2panda_DiagnosticKind *ekind, const char **args, size_t argc,
                       es2panda_SourcePosition *pos)
    {
        return impl_->LogDiagnostic(context, ekind, args, argc, pos);
    }

    void ClassDefinitionSetFromStructModifier(es2panda_Context *context, es2panda_AstNode *classInstance)
    {
        return impl_->ClassDefinitionSetFromStructModifier(context, classInstance);
    }

    bool ClassDefinitionIsFromStructConst(es2panda_Context *context, es2panda_AstNode *classInstance)
    {
        return impl_->ClassDefinitionIsFromStructConst(context, classInstance);
    }

    NO_COPY_SEMANTIC(Initializer);
    NO_MOVE_SEMANTIC(Initializer);

private:
    bool logFlag_;
    es2panda_Impl const *impl_;
    es2panda_Config *cfg_;
};

ir::AstNode *GetTouchingToken(es2panda_Context *context, size_t pos, bool flagFindFirstMatch);
ir::AstNode *GetTouchingTokenRightMatch(es2panda_Context *context, size_t pos, parser::Program *program = nullptr);
ir::AstNode *GetTouchingTokenByRange(es2panda_Context *context, const TextRange &span, bool flagFindFirstMatch);
ir::AstNode *GetOptimumNodeByRange(const ir::AstNode *node, const TextRange &span);
ir::AstNode *GetTouchingTokenForIdentifier(es2panda_Context *context, size_t pos, bool flagFindFirstMatch);
ir::AstNode *FindPrecedingToken(const size_t pos, const ir::AstNode *startNode);
ir::AstNode *GetIdentifierFromSuper(ir::AstNode *super);
ir::AstNode *GetOriginalNode(ir::AstNode *astNode);
checker::VerifiedType GetTypeOfSymbolAtLocation(checker::ETSChecker *checker, ir::AstNode *astNode);
FileDiagnostic CreateDiagnosticForNode(es2panda_AstNode *node, Diagnostic diagnostic, es2panda_Context *context,
                                       const std::vector<std::string> &args = std::vector<std::string>());
std::string GetCurrentTokenValueImpl(es2panda_Context *context, size_t position, ir::AstNode *preceding);
void GetRangeOfEnclosingComment(es2panda_Context *context, size_t pos, CommentRange *result);
void GetRangeOfCommentFromContext(std::string const &sourceCode, size_t leftPos, size_t rightPos, size_t pos,
                                  CommentRange *result);
Diagnostic CreateDiagnosticForError(es2panda_Context *context, const util::DiagnosticBase &error);
Diagnostic CreateDiagnosticWithoutFile(const util::DiagnosticBase &error);
void GetGlobalDiagnostics(es2panda_Context *context, DiagnosticReferences &compilerOptionsDiagnostics);
void GetOptionDiagnostics(es2panda_Context *context, DiagnosticReferences &compilerOptionsDiagnostics);
size_t GetTokenPosOfNode(const ir::AstNode *astNode);
std::pair<ir::AstNode *, util::StringView> GetDefinitionAtPositionImpl(es2panda_Context *context, size_t pos);
std::pair<ir::AstNode *, util::StringView> GetDefinitionAtPositionImpl(
    es2panda_Context *context, size_t pos, checker::ExportClosureResolver *resolver,
    varbinder::Variable **resolvedVariable = nullptr);
DocumentHighlights GetDocumentHighlightsImpl(es2panda_Context *context, size_t position);

void GetReferenceLocationAtPositionImpl(FileNodeInfo fileNodeInfo, es2panda_Context *referenceFileContext,
                                        ReferenceLocationList *list);
void RemoveFromFiles(std::vector<std::string> &files, const std::vector<std::string> &autoGenerateFolders);
ir::AstNode *GetIdentifier(ir::AstNode *node);
ark::es2panda::varbinder::Variable *ResolveIdentifier(const ark::es2panda::ir::Identifier *ident);
ir::AstNode *FindRightToken(const size_t pos, const std::vector<ir::AstNode *> &nodes);
ir::AstNode *GetOwner(ir::AstNode *node);
std::string GetOwnerId(ir::AstNode *node);
std::string GetIdentifierName(ir::AstNode *node);
bool NodeHasTokens(const ir::AstNode *node);
std::vector<ir::AstNode *> GetChildren(const ir::AstNode *node);
void FindAllChild(const ir::AstNode *ast, const ir::NodePredicate &cb, std::vector<ir::AstNode *> &results);
ir::AstNode *FindAncestor(ir::AstNode *node, const ir::NodePredicate &cb);
std::vector<CodeFixActionInfo> GetCodeFixesAtPositionImpl(es2panda_Context *context, size_t startPosition,
                                                          size_t endPosition, std::vector<int> &errorCodes,
                                                          CodeFixOptions &codeFixOptions);
CombinedCodeActionsInfo GetCombinedCodeFixImpl(es2panda_Context *context, const std::string &fixId,
                                               CodeFixOptions &codeFixOptions);
ir::Identifier *GetIdentFromNewClassExprPart(const ir::Expression *value);
varbinder::Decl *FindDeclInFunctionScope(varbinder::Scope *scope, const util::StringView &name);
varbinder::Decl *FindDeclInGlobalScope(varbinder::Scope *scope, const util::StringView &name);
varbinder::Decl *FindDeclInScopeWithFallback(varbinder::Scope *scope, const util::StringView &name);
std::string GetImportFilePath(es2panda_Context *context, size_t pos);
std::string GetImportFilePath(const util::ImportInfo &importInfo);
void MakeDiagnosticReferences(es2panda_Context *context, const util::DiagnosticStorage &diagnostics,
                              DiagnosticReferences &result);
std::string GetTokenTypes(ir::ModifierFlags flags);
std::string ComputeRelativeImportPath(const std::string &fromFile, const std::string &toFile,
                                      const ArkTsConfig *config = nullptr);
// The module alias import form of a file ('library' for a file under the paths module root,
// 'dynlib' for a dynamic-module file). Used by import fixes in addition to the file-level
// specifier so the entry alias form stays available.
std::string ComputeModuleImportPath(const std::string &toFile, const ArkTsConfig *config = nullptr);
// The module entry file (without extension) for a file: the bare package dependency's
// source/declaration for a dynamic module, or the 'paths' module root's 'Index' (or the
// per-file alias target). Returns an empty string when the file is not inside a module.
std::string FindModuleEntryFile(const std::string &file, const ArkTsConfig &config);
const ArkTsConfig *GetArkTsConfigOrNull(const public_lib::Context *ctx);

// The live-code mask of a source: true where the byte belongs to code, false inside line
// comments, block comments and string literals. Template literals count as strings in full.
// Regex literals are not distinguished from division: '/'-delimited patterns are
// indistinguishable without parsing context (an accepted limitation of a text scan).
std::vector<bool> BuildLiveCodeMask(const std::string &source);

// The dynamic import form ('dynamic@xxx') of a dependency key: build-system-format keys
// are 'dynamic/'-prefixed and carry the 'dynamic@xxx' import form in the item alias; the
// legacy 'dynamic@'-prefixed key format is returned as is. Returns nullopt for non-dynamic
// keys. Keep in sync with GetDependencyImportSpecifier, which adds the module-key fallback.
std::optional<std::string> GetDynamicImportSpecifierOrNull(std::string_view dependencyKey,
                                                           const ArkTsConfig::ExternalModuleData &moduleData);

// GetDynamicImportSpecifierOrNull with the module-key fallback: a non-dynamic dependency
// key resolves to its first module segment ('dynlib/src/main/ets/Index' -> 'dynlib').
std::string GetDependencyImportSpecifier(std::string_view dependencyKey,
                                         const ArkTsConfig::ExternalModuleData &moduleData);

// The dynamic module key ('dynamic@xxx') when the file is the declaration ('path') or source
// file ('sourceFilePath') of a dynamic dependency; nullopt otherwise. The arktsconfig
// 'dependencies' section is the single source of truth for the dynamic classification.
// The key keeps the file-level import form (see CanonicalizeDynamicImportSource): the module
// entry normalizes to the bare package name, sibling files keep their file identity.
std::optional<std::string> FindDynamicDeclModuleKey(const std::string &file, const ArkTsConfig &config);

// The canonical file-level import form of a resolved import source: a dependency key of a
// dynamic module canonicalizes like FindDynamicDeclModuleKey does for declaration files, so
// both sides of a module symbol key match regardless of the dependency key form written (the
// legacy full form resolves to the transformed form, an entry alias to the package name).
// Sources that are no dynamic dependency keys stay as they are.
std::string CanonicalizeDynamicImportSource(const std::string &source, const ArkTsConfig &config);

// Whether the file is the declaration ('path') or source file ('sourceFilePath') of a
// dependency recorded with a dynamic language. Unlike FindDynamicDeclModuleKey this also
// matches module-relative keys ('dynlib/src/main/ets/Index'), so it covers every dynamic
// dependency of a generated arktsconfig, not only the interop SDK entries.
bool IsDynamicDependencyFile(const std::string &file, const ArkTsConfig &config);
}  // namespace ark::es2panda::lsp

#endif
