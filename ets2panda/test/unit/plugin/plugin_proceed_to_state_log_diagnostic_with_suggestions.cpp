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

#include <cstring>
#include <algorithm>
#include "util.h"
#include "util/diagnosticEngine.h"

// NOLINTBEGIN
constexpr size_t EXPECTED_SUGGESTION_COUNT = 2;
constexpr size_t SOURCE_START_LINE = 0;
constexpr size_t SOURCE_END_LINE = 0;
constexpr size_t SOURCE_START_INDEX = 0;
constexpr size_t SOURCE_END_INDEX = 7;
static es2panda_Impl *g_impl = nullptr;
static std::string g_source = R"(
function main() {}
)";

static bool CreateSuggestionsAndLog(es2panda_Context *g_ctx)
{
    auto suggestionKind1 = g_impl->CreateDiagnosticKind(g_ctx, "origin {}", ES2PANDA_PLUGIN_SUGGESTION);
    auto suggestionKind2 = g_impl->CreateDiagnosticKind(g_ctx, "another {}", ES2PANDA_PLUGIN_SUGGESTION);
    auto diagnosticKind = g_impl->CreateDiagnosticKind(g_ctx, "error", ES2PANDA_PLUGIN_ERROR);

    auto *left = g_impl->CreateSourcePosition(g_ctx, SOURCE_START_INDEX, SOURCE_START_LINE);
    auto *right = g_impl->CreateSourcePosition(g_ctx, SOURCE_END_INDEX, SOURCE_END_LINE);
    auto *range = g_impl->CreateSourceRange(g_ctx, left, right);

    const char *params1[] = {"a"};
    const char *params2[] = {"b"};

    auto *suggestionInfo1 =
        g_impl->CreateSuggestionInfo(g_ctx, suggestionKind1, params1, 1, "replace a", "Title A:", range);
    auto *suggestionInfo2 =
        g_impl->CreateSuggestionInfo(g_ctx, suggestionKind2, params2, 1, "replace b", "Title B:", range);

    es2panda_SuggestionInfo *suggestions[] = {suggestionInfo1, suggestionInfo2};

    auto *diagnosticInfo = g_impl->CreateDiagnosticInfo(g_ctx, diagnosticKind, nullptr, 0, left);

    g_impl->LogDiagnosticWithSuggestions(g_ctx, diagnosticInfo, suggestions, EXPECTED_SUGGESTION_COUNT);

    return true;
}

static bool ValidateDiagnostic(es2panda_Context *g_ctx)
{
    auto errors = g_impl->GetPluginErrors(g_ctx);
    auto storage = reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(errors);

    if (storage->size() != 1 || strcmp((*storage)[0]->Message().data(), "error") != 0) {
        return false;
    }

    auto diagnostic = reinterpret_cast<const ark::es2panda::util::Diagnostic *>(&(*(*storage)[0]));

    auto suggestions = diagnostic->Suggestion();
    if (suggestions.size() != EXPECTED_SUGGESTION_COUNT) {
        return false;
    }

    for (const auto *s : suggestions) {
        if (s->SourceRange()->start.line != SOURCE_START_LINE || s->SourceRange()->end.line != SOURCE_END_LINE ||
            s->SourceRange()->start.index != SOURCE_START_INDEX || s->SourceRange()->end.index != SOURCE_END_INDEX) {
            return false;
        }
    }

    return true;
}

static bool TestFunction(es2panda_Context *g_ctx)
{
    return CreateSuggestionsAndLog(g_ctx) && ValidateDiagnostic(g_ctx);
}

int main(int argc, char **argv)
{
    std::map<es2panda_ContextState, std::vector<std::function<bool(es2panda_Context *)>>> testFunctions;
    testFunctions[ES2PANDA_STATE_LOWERED] = {TestFunction};
    ProccedToStatePluginTestData data = {argc, argv, &g_impl, testFunctions, true, g_source, ES2PANDA_STATE_LOWERED};
    return RunAllStagesWithTestFunction(data);
}
// NOLINTEND
