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

#ifndef ES2PANDA_LSP_INCLUDE_INTERNAL_API_H
#define ES2PANDA_LSP_INCLUDE_INTERNAL_API_H

#include <vector>
#include "api.h"
#include "checker/types/type.h"
#include "ir/astNode.h"
#include "public/es2panda_lib.h"

namespace ark::es2panda::lsp {

class Initializer {
public:
    Initializer();

    ~Initializer();

    ark::ArenaAllocator *Allocator()
    {
        return allocator_;
    }

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

    NO_COPY_SEMANTIC(Initializer);
    NO_MOVE_SEMANTIC(Initializer);

private:
    es2panda_Impl const *impl_;
    es2panda_Config *cfg_;
    ark::ArenaAllocator *allocator_;
};

ir::AstNode *GetTouchingToken(es2panda_Context *context, size_t pos, bool flagFindFirstMatch);
References GetFileReferencesImpl(es2panda_Context *referenceFileContext, char const *searchFileName,
                                 bool isPackageModule);
ir::AstNode *FindPrecedingToken(const size_t pos, const ir::AstNode *startNode, ArenaAllocator *allocator);
ir::AstNode *GetIdentifierFromSuper(ir::AstNode *super);
ir::AstNode *GetOriginalNode(ir::AstNode *astNode);
checker::VerifiedType GetTypeOfSymbolAtLocation(checker::ETSChecker *checker, ir::AstNode *astNode);
FileDiagnostic CreateDiagnosticForNode(es2panda_AstNode *node, Diagnostic diagnostic, es2panda_Context *context,
                                       const std::vector<std::string> &args = std::vector<std::string>());
std::string GetCurrentTokenValueImpl(es2panda_Context *context, size_t position);
void GetRangeOfEnclosingComment(es2panda_Context *context, size_t pos, CommentRange *result);
Diagnostic CreateDiagnosticForError(es2panda_Context *context, const util::DiagnosticBase &error);
Diagnostic CreateDiagnosticWithoutFile(const util::DiagnosticBase &error);
void GetGlobalDiagnostics(es2panda_Context *context, DiagnosticReferences &compilerOptionsDiagnostics);
void GetOptionDiagnostics(es2panda_Context *context, DiagnosticReferences &compilerOptionsDiagnostics);
size_t GetTokenPosOfNode(const ir::AstNode *astNode);
std::pair<ir::AstNode *, util::StringView> GetDefinitionAtPositionImpl(es2panda_Context *context, size_t pos);
DocumentHighlights GetDocumentHighlightsImpl(es2panda_Context *context, size_t position);

void GetReferenceLocationAtPositionImpl(FileNodeInfo fileNodeInfo, es2panda_Context *referenceFileContext,
                                        ReferenceLocationList *list);
void RemoveFromFiles(std::vector<std::string> &files, const std::vector<std::string> &autoGenerateFolders);
ir::AstNode *FindRightToken(const size_t pos, const ArenaVector<ir::AstNode *> &nodes);
std::string GetOwnerId(ir::AstNode *node);
std::string GetIdentifierName(ir::AstNode *node);
bool NodeHasTokens(const ir::AstNode *node);
void FindAllChild(const ir::AstNode *ast, const ir::NodePredicate &cb, ArenaVector<ir::AstNode *> &results);
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

}  // namespace ark::es2panda::lsp

#endif