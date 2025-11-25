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

#include "navigate_to.h"
#include "quick_info.h"
#include "internal_api.h"
#include "lsp/include/get_adjusted_location.h"
#include "public/public.h"

namespace ark::es2panda::lsp {

PatternMatcher::PatternMatcher(const std::string &pattern, bool isCaseSensitive)
    : pattern_(pattern), isCaseSensitive_(isCaseSensitive)
{
    try {
        regexPattern_ = isCaseSensitive ? std::regex(pattern) : std::regex(pattern, std::regex_constants::icase);
    } catch (const std::regex_error &) {
        regexPattern_ = std::nullopt;
    }
}

bool PatternMatcher::IsPatternValid() const
{
    return regexPattern_.has_value();
}

bool PatternMatcher::MatchesExact(const std::string &candidate) const
{
    return regexPattern_ && std::regex_match(candidate, *regexPattern_);
}

bool PatternMatcher::MatchesPrefix(const std::string &candidate) const
{
    if (candidate.size() < pattern_.size()) {
        return false;
    }

    for (size_t i = 0; i < pattern_.size(); ++i) {
        char a = pattern_[i];
        char b = candidate[i];
        if (isCaseSensitive_) {
            if (a != b) {
                return false;
            }
        } else {
            if (std::tolower(a) != std::tolower(b)) {
                return false;
            }
        }
    }

    return true;
}

bool PatternMatcher::MatchesSubstring(const std::string &candidate) const
{
    return regexPattern_ && std::regex_search(candidate, *regexPattern_);
}

MatchKind DetermineMatchKind(const std::string &candidate, const PatternMatcher &matcher)
{
    if (matcher.MatchesExact(candidate)) {
        return MatchKind::EXACT;
    }
    if (matcher.MatchesPrefix(candidate)) {
        return MatchKind::PREFIX;
    }
    if (matcher.MatchesSubstring(candidate)) {
        return MatchKind::SUBSTRING;
    }
    return MatchKind::NONE;
}

std::optional<NavigateToItem> TryMakeItemFromNode(ir::AstNode *node, const PatternMatcher &matcher,
                                                  const std::string &filePath)
{
    if ((node == nullptr) || !node->IsIdentifier()) {
        return std::nullopt;
    }

    const auto name = node->AsIdentifier()->Name();
    const auto matchKind = DetermineMatchKind(std::string(name), matcher);
    if (matchKind == MatchKind::NONE) {
        return std::nullopt;
    }

    auto *container = GetContainerNode(node);
    if (container == nullptr) {
        return std::nullopt;
    }

    const auto containerName = GetIdentifierName(container);
    const auto nodeType = node->Type();
    const auto containerType = container->Type();

    return NavigateToItem {std::string(name), ToString(nodeType),     matchKind, true, filePath,
                           containerName,     ToString(containerType)};
}

// improve get declarations correct and better
std::vector<NavigateToItem> GetItemsFromNamedDeclaration(es2panda_Context *context, const SourceFile &file,
                                                         const PatternMatcher &matcher)
{
    std::vector<NavigateToItem> items;

    const std::string filePath = std::string {file.filePath};
    const std::string fileContent = std::string {file.source};

    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    auto *ast = reinterpret_cast<ir::AstNode *>(ctx->parserProgram->Ast());
    auto children = GetChildren(ast, ctx->allocator);
    items.reserve(children.size());
    for (auto *child : children) {
        if (auto item = TryMakeItemFromNode(child, matcher, filePath)) {
            items.push_back(std::move(*item));
        }
    }
    return items;
}

// Helper: tries to emit a single item, returns false if limit reached
static bool TryEmitItem(const NavigateToItem &item, size_t &remaining,
                        std::set<std::pair<std::string, std::string>> &seenPairs, std::vector<NavigateToItem> &results)
{
    if (remaining == 0) {
        return false;
    }
    auto key = std::make_pair(item.name, item.containerName);
    if (!seenPairs.insert(key).second) {
        return true;  // duplicate, but still under limit
    }
    results.emplace_back(item);
    --remaining;
    return remaining > 0;
}

std::vector<NavigateToItem> GetNavigateToItems(es2panda_Context *context, const std::vector<SourceFile> &srcFiles,
                                               size_t maxResultCount, const std::string &searchValue,
                                               bool isCaseSensitive)
{
    static std::unordered_map<std::string, size_t> totalEmitted;
    size_t &emittedSoFar = totalEmitted[searchValue];

    std::vector<NavigateToItem> results;
    std::set<std::pair<std::string, std::string>> seenPairs;
    PatternMatcher matcher(searchValue, isCaseSensitive);

    if (!matcher.IsPatternValid() || emittedSoFar >= maxResultCount) {
        return results;
    }

    size_t remaining = maxResultCount - emittedSoFar;

    for (const auto &file : srcFiles) {
        auto items = GetItemsFromNamedDeclaration(context, file, matcher);
        for (const auto &item : items) {
            if (!TryEmitItem(item, remaining, seenPairs, results)) {
                emittedSoFar = maxResultCount;
                return results;
            }
        }
    }

    emittedSoFar = maxResultCount - remaining;
    return results;
}

}  // namespace ark::es2panda::lsp