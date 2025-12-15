/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>

#include "find_references.h"
#include "ir/astNode.h"
#include "parser/program/program.h"
#include "public/public.h"
#include "public/es2panda_lib.h"
#include "compiler/lowering/util.h"
#include "internal_api.h"

namespace {
// NOTE(muhammet): This may be wrong/inconsistent (slow for sure) for comparison, have to investigate
// The Type of the Node and identifier name are not enough they don't account for edge cases like
// functions with the same name and signature in different namespaces
using LocationId = std::string;
LocationId GetLocationId(ark::es2panda::ir::AstNode *node, ark::es2panda::parser::Program *program)
{
    if (node == nullptr) {
        return "";
    }
    // Find which file the node belongs to
    std::string absPath;
    if (program->Ast() == node->GetTopStatement()) {
        absPath = std::string {program->AbsoluteName()};
    }
    auto externals = program->DirectExternalSources();
    auto top = node->GetTopStatement();
    for (const auto &entry : externals) {
        for (const auto &p : entry.second) {
            auto programAbsPath = std::string {p->AbsoluteName()};
            auto ast = p->Ast();
            if (ast == top) {
                absPath = programAbsPath;
                break;
            }
        }
    }
    // Should uniquely identify a token using it's sourceFile path, start and end positions
    auto id = absPath + ":" + std::to_string(node->Start().index) + ":" + std::to_string(node->Start().line) + ":" +
              std::to_string(node->End().index) + ":" + std::to_string(node->End().line);
    return id;
}

std::set<ark::es2panda::lsp::ReferencedNode> FindReferences(const ark::es2panda::SourceFile &srcFile,
                                                            LocationId tokenLocationId, std::string tokenName)
{
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto filePath = std::string {srcFile.filePath};
    auto fileContent = std::string {srcFile.source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());

    // Clear before searching each file
    std::set<ark::es2panda::lsp::ReferencedNode> res;
    ark::es2panda::parser::Program *pprogram = nullptr;
    auto cb = [&tokenName, &pprogram, &filePath, &tokenLocationId, &res](ark::es2panda::ir::AstNode *node) {
        if (!node->IsIdentifier()) {
            return false;
        }
        auto nodeName = ark::es2panda::lsp::GetIdentifierName(node);
        if (nodeName != tokenName) {
            return false;
        }
        auto owner = ark::es2panda::lsp::GetOwner(node);
        auto nodeOwnerLocationId = GetLocationId(owner, pprogram);
        auto nodeLocationId = GetLocationId(node, pprogram);
        bool isDefinition = nodeLocationId == nodeOwnerLocationId;
        if (nodeOwnerLocationId == tokenLocationId) {
            res.insert(ark::es2panda::lsp::ReferencedNode {filePath, node->Start().index, node->End().index,
                                                           node->Start().line, isDefinition});
        }
        return false;
    };

    // Search an ast
    auto search = [&cb](ark::es2panda::parser::Program *program) -> void {
        if (program == nullptr) {
            return;
        }
        auto ast = program->Ast();
        ast->FindChild(cb);
    };

    // Search the file
    auto pctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    pprogram = pctx->parserProgram;
    search(pprogram);

    initializer.DestroyContext(context);
    return res;
}

std::set<ark::es2panda::lsp::ReferencedNode> FindReferences(es2panda_Context *context, LocationId tokenLocationId,
                                                            std::string tokenName)
{
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto filePath = std::string {ctx->sourceFile->filePath};

    // Clear before searching each file
    std::set<ark::es2panda::lsp::ReferencedNode> res;
    ark::es2panda::parser::Program *pprogram = ctx->parserProgram;

    if (ctx->parserProgram->Ast() == nullptr) {
        return res;
    }
    ctx->parserProgram->Ast()->IterateRecursively([&tokenName, &pprogram, &filePath, &tokenLocationId,
                                                   &res](ark::es2panda::ir::AstNode *node) {
        auto targetNode = node;
        if (targetNode->OriginalNode()) {
            targetNode = targetNode->OriginalNode();
        }
        if (!targetNode->IsIdentifier()) {
            return;
        }
        auto nodeName = ark::es2panda::lsp::GetIdentifierName(targetNode);
        if (nodeName != tokenName) {
            return;
        }
        auto owner = ark::es2panda::lsp::GetOwner(targetNode);
        auto nodeOwnerLocationId = GetLocationId(owner, pprogram);
        auto nodeLocationId = GetLocationId(targetNode, pprogram);
        bool isDefinition = nodeLocationId == nodeOwnerLocationId;
        if (nodeOwnerLocationId == tokenLocationId) {
            res.insert(ark::es2panda::lsp::ReferencedNode {filePath, targetNode->Start().index, targetNode->End().index,
                                                           targetNode->Start().line, isDefinition});
        }
    });
    return res;
}
}  // namespace

namespace ark::es2panda::lsp {
std::set<ReferencedNode> FindReferences(CancellationToken *tkn, const std::vector<ark::es2panda::SourceFile> &srcFiles,
                                        const ark::es2panda::SourceFile &srcFile, size_t position)
{
    std::string tokenName;
    LocationId tokenLocationId;
    // Part 1: Determine the type of token function/variable
    {
        ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
        auto filePath = std::string {srcFile.filePath};
        auto fileContent = std::string {srcFile.source};
        auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());

        auto touchingToken = GetTouchingToken(context, position, false);
        tokenName = GetIdentifierName(touchingToken);
        auto owner = GetOwner(touchingToken);
        tokenLocationId =
            ::GetLocationId(owner, reinterpret_cast<ark::es2panda::public_lib::Context *>(context)->parserProgram);
        initializer.DestroyContext(context);
    }

    if (tokenLocationId.empty()) {
        return {};
    }

    std::set<ReferencedNode> res;
    // NOTE(muhammet): The process is very wasteful, it creates a new context for each file even if they're dependent on
    // one another
    for (auto fl : srcFiles) {
        // NOTE(muhammet): Need for more fine grained cancellation check but for now doing it before context creations
        // should be good enough, thats where it's slowest
        if (tkn->IsCancellationRequested()) {
            return res;
        }
        auto refList = ::FindReferences(fl, tokenLocationId, tokenName);
        for (const auto &entry : refList) {
            res.insert(entry);
        }
    }

    return res;
}

std::set<ReferencedNode> FindReferences(CancellationToken *tkn, const std::vector<es2panda_Context *> &fileContexts,
                                        es2panda_Context *context, size_t position)
{
    std::string tokenName;
    LocationId tokenLocationId;
    // Part 1: Determine the type of token function/variable
    {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);

        auto touchingToken = GetTouchingTokenForIdentifier(context, position, false);
        tokenName = GetIdentifierName(touchingToken);
        auto owner = GetOwner(touchingToken);
        tokenLocationId = ::GetLocationId(owner, ctx->parserProgram);
    }

    if (tokenLocationId.empty()) {
        return {};
    }

    std::set<ReferencedNode> res;
    for (auto fc : fileContexts) {
        // NOTE(muhammet): Need for more fine grained cancellation check but for now doing it before context
        // creations
        // should be good enough, thats where it's slowest
        if (tkn->IsCancellationRequested()) {
            return res;
        }
        res.merge(::FindReferences(fc, tokenLocationId, tokenName));
    }

    return res;
}
}  // namespace ark::es2panda::lsp
