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

#include "lsp/include/symbol_reference_index.h"
#include "compiler/lowering/util.h"
#include "ir/astNode.h"
#include "lsp/include/internal_api.h"
#include "parser/program/program.h"
#include "public/public.h"

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace ark::es2panda::lsp {
namespace {

struct SymbolOccurrence {
    size_t start = 0;
    size_t end = 0;
    SymbolId symbolId = 0;
};

struct FileIndexData {
    std::string source;
    std::vector<SymbolOccurrence> symbolOccurrences;
    std::vector<std::pair<SymbolId, ReferenceInfo>> references;
};

std::unordered_map<std::string, FileIndexData> g_fileIndices {};
std::unordered_map<SymbolId, std::vector<ReferenceInfo>> g_symbolReferences {};
std::unordered_map<SymbolId, ReferenceInfo> g_symbolDefinitions {};

std::string GetNodeFileName(const ir::AstNode *node, const public_lib::Context *ctx)
{
    if (node == nullptr) {
        return ctx != nullptr ? ctx->sourceFileName : "";
    }
    if (node->Range().start.Program() != nullptr) {
        return std::string(node->Range().start.Program()->SourceFilePath());
    }
    if (ctx != nullptr) {
        return ctx->sourceFileName;
    }
    return "";
}

SymbolId BuildSymbolId(const ir::AstNode *declNode, const ir::Identifier *identifier, const public_lib::Context *ctx)
{
    if (declNode == nullptr || identifier == nullptr) {
        return 0;
    }

    const auto declFileName = GetNodeFileName(declNode, ctx);
    const auto key = declFileName + ":" + std::to_string(declNode->Start().index) + ":" +
                     std::to_string(declNode->End().index) + ":" + std::string(identifier->Name()) + ":" +
                     std::to_string(static_cast<int>(declNode->Type()));
    return static_cast<SymbolId>(std::hash<std::string> {}(key));
}

bool IsDefinitionNode(const ir::AstNode *node, const ir::AstNode *owner)
{
    if (node == nullptr || owner == nullptr) {
        return false;
    }
    return node->Start().index == owner->Start().index && node->End().index == owner->End().index;
}

void RemoveFileContributions(const std::string &fileName)
{
    auto it = g_fileIndices.find(fileName);
    if (it == g_fileIndices.end()) {
        return;
    }

    for (const auto &entry : it->second.references) {
        const auto symbolId = entry.first;
        const auto refInfo = entry.second;
        auto refsIt = g_symbolReferences.find(symbolId);
        if (refsIt == g_symbolReferences.end()) {
            continue;
        }
        auto &refs = refsIt->second;
        refs.erase(std::remove_if(refs.begin(), refs.end(),
                                  // CC-OFFNXT(G.FMT.06-CPP) project code style
                                  [refInfo](const ReferenceInfo &current) {
                                      return current.fileName == refInfo.fileName && current.start == refInfo.start &&
                                             current.length == refInfo.length;
                                  }),  // CC-OFF(G.FMT.02-CPP) project code style
                   refs.end());
        if (refs.empty()) {
            g_symbolReferences.erase(refsIt);
            g_symbolDefinitions.erase(symbolId);
        }
    }

    g_fileIndices.erase(it);
}

void DeduplicateAndSortReferences(std::vector<ReferenceInfo> &refs)
{
    std::sort(refs.begin(), refs.end(), [](const ReferenceInfo &lhs, const ReferenceInfo &rhs) {
        if (lhs.fileName != rhs.fileName) {
            return lhs.fileName < rhs.fileName;
        }
        if (lhs.start != rhs.start) {
            return lhs.start < rhs.start;
        }
        return lhs.length < rhs.length;
    });
    refs.erase(std::unique(refs.begin(), refs.end(),
                           // CC-OFFNXT(G.FMT.06-CPP) project code style
                           [](const ReferenceInfo &lhs, const ReferenceInfo &rhs) {
                               return lhs.fileName == rhs.fileName && lhs.start == rhs.start &&
                                      lhs.length == rhs.length;
                           }),  // CC-OFF(G.FMT.02-CPP) project code style
               refs.end());
}

void ProcessNodeForSymbolIndex(const public_lib::Context *ctx, ir::AstNode *node,
                               std::unordered_set<std::string> &localDedup, FileIndexData &fileIndex)
{
    auto *targetNode = node->OriginalNode() != nullptr ? node->OriginalNode() : node;
    if (targetNode == nullptr || !targetNode->IsIdentifier()) {
        return;
    }

    auto *identifier = targetNode->AsIdentifier();
    auto *owner = ark::es2panda::lsp::GetOwner(targetNode);
    if (identifier == nullptr || owner == nullptr) {
        return;
    }

    const SymbolId symbolId = BuildSymbolId(owner, identifier, ctx);
    if (symbolId == 0) {
        return;
    }

    const size_t start = targetNode->Start().index;
    const size_t end = targetNode->End().index;
    if (end <= start) {
        return;
    }

    const std::string refFileName = GetNodeFileName(targetNode, ctx);
    const ReferenceInfo ref(refFileName, start, end - start);
    const std::string dedupKey = std::to_string(symbolId) + ":" + ref.fileName + ":" + std::to_string(ref.start) + ":" +
                                 std::to_string(ref.length);
    if (!localDedup.insert(dedupKey).second) {
        return;
    }

    fileIndex.symbolOccurrences.push_back(SymbolOccurrence {start, end, symbolId});
    fileIndex.references.emplace_back(symbolId, ref);
    g_symbolReferences[symbolId].push_back(ref);

    if (IsDefinitionNode(targetNode, owner)) {
        g_symbolDefinitions[symbolId] =
            ReferenceInfo(GetNodeFileName(owner, ctx), owner->Start().index, owner->End().index - owner->Start().index);
    }
}

bool BuildSymbolReferenceIndexForProgram(const public_lib::Context *ctx, parser::Program *program)
{
    const std::string fileName = std::string(program->SourceFilePath());
    RemoveFileContributions(fileName);

    FileIndexData fileIndex {};
    fileIndex.source = std::string(program->SourceCode());

    std::unordered_set<std::string> localDedup {};
    auto *astRoot = reinterpret_cast<ir::AstNode *>(program->Ast());
    astRoot->IterateRecursively([ctx, &localDedup, &fileIndex](ir::AstNode *node) {
        ProcessNodeForSymbolIndex(ctx, node, localDedup, fileIndex);
    });

    std::sort(fileIndex.symbolOccurrences.begin(), fileIndex.symbolOccurrences.end(),
              // CC-OFFNXT(G.FMT.06-CPP) project code style
              [](const SymbolOccurrence &lhs, const SymbolOccurrence &rhs) {
                  if (lhs.start != rhs.start) {
                      return lhs.start < rhs.start;
                  }
                  return lhs.end < rhs.end;  // CC-OFF(G.FMT.02-CPP) project code style
              });

    for (auto &[symbolId, refs] : g_symbolReferences) {
        DeduplicateAndSortReferences(refs);
    }

    g_fileIndices[fileName] = std::move(fileIndex);
    return true;
}

}  // namespace

void InitSymbolReferenceIndex()
{
    g_fileIndices.clear();
    g_symbolReferences.clear();
    g_symbolDefinitions.clear();
}

void ClearSymbolReferenceIndex()
{
    InitSymbolReferenceIndex();
}

bool BuildSymbolReferenceIndexForContext(es2panda_Context *context)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    if (ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return false;
    }

    return BuildSymbolReferenceIndexForProgram(ctx, ctx->parserProgram);
}

bool BuildSymbolReferenceIndexForContextWithExternal(es2panda_Context *context)
{
    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    bool buildSuccess = true;
    std::unordered_set<std::string> indexedFiles {};

    auto buildIfNeeded = [ctx, &buildSuccess, &indexedFiles](parser::Program *program) {
        if (program == nullptr || program->Ast() == nullptr) {
            return;
        }
        std::string fileName = std::string(program->AbsoluteName());
        if (!indexedFiles.emplace(fileName).second) {
            return;
        }
        buildSuccess = BuildSymbolReferenceIndexForProgram(ctx, program) && buildSuccess;
    };

    buildIfNeeded(ctx->parserProgram);

    auto *externalSources = ctx->parserProgram->GetExternalSources();
    if (externalSources == nullptr) {
        return buildSuccess;
    }

    for (const auto &[_, program] : externalSources->Direct()) {
        buildIfNeeded(program);
    }

    // Traverse transitive externals across all module kinds:
    // MODULE, SOURCE_DECL, PACKAGE, ETSCACHE_DECL.
    externalSources->template Visit<false>([&buildIfNeeded](auto *program) { buildIfNeeded(program); });
    return buildSuccess;
}

bool RemoveSymbolReferenceIndexForFile(const std::string &fileName)
{
    const auto before = g_fileIndices.size();
    RemoveFileContributions(fileName);
    return g_fileIndices.size() < before;
}

References GetReferencesAtPositionFromIndex(es2panda_Context *context, size_t position)
{
    References result {};
    if (context == nullptr) {
        return result;
    }

    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    const std::string fileName = ctx->sourceFileName;
    auto fileIt = g_fileIndices.find(fileName);
    if (fileIt == g_fileIndices.end()) {
        return result;
    }

    SymbolId hitSymbol = 0;
    size_t hitLength = static_cast<size_t>(-1);
    for (const auto &occurrence : fileIt->second.symbolOccurrences) {
        if (position < occurrence.start || position >= occurrence.end) {
            continue;
        }
        const size_t currentLength = occurrence.end - occurrence.start;
        if (currentLength <= hitLength) {
            hitLength = currentLength;
            hitSymbol = occurrence.symbolId;
        }
    }

    if (hitSymbol == 0) {
        return result;
    }

    auto refsIt = g_symbolReferences.find(hitSymbol);
    if (refsIt == g_symbolReferences.end()) {
        return result;
    }

    auto refs = refsIt->second;
    auto defIt = g_symbolDefinitions.find(hitSymbol);
    if (defIt != g_symbolDefinitions.end()) {
        const auto &defRef = defIt->second;
        result.definitionInfo = DefinitionInfo(defRef.fileName, defRef.start, defRef.length);
        refs.erase(std::remove_if(refs.begin(), refs.end(),
                                  // CC-OFFNXT(G.FMT.06-CPP) project code style
                                  [defRef](const ReferenceInfo &ref) {
                                      return ref.fileName == defRef.fileName && ref.start == defRef.start &&
                                             ref.length == defRef.length;
                                  }),  // CC-OFF(G.FMT.02-CPP) project code style
                   refs.end());
    }

    DeduplicateAndSortReferences(refs);
    result.referenceInfos = std::move(refs);
    return result;
}

std::string GetIndexedFileSource(const std::string &fileName)
{
    auto it = g_fileIndices.find(fileName);
    if (it == g_fileIndices.end()) {
        return "";
    }
    return it->second.source;
}

}  // namespace ark::es2panda::lsp
