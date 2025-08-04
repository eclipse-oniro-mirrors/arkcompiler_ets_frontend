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

#include <cstring>
#include <algorithm>
#include "util.h"
#include "util/diagnosticEngine.h"

// NOLINTBEGIN

static es2panda_Impl *g_impl = nullptr;
static es2panda_Context *g_ctx = nullptr;
static std::string g_source = R"(
function main() {}
)";

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return 1;
    }

    if (GetImpl() == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }

    g_impl = GetImpl();
    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = g_impl->CreateConfig(argc - 1, args);
    g_ctx = g_impl->CreateContextFromString(config, g_source.data(), argv[argc - 1]);
    if (g_ctx == nullptr) {
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    g_impl->ProceedToState(g_ctx, ES2PANDA_STATE_PARSED);
    if (g_impl->ContextState(g_ctx) == ES2PANDA_STATE_ERROR) {
        return PROCEED_ERROR_CODE;
    }

    auto suggestionkind = g_impl->CreateDiagnosticKind(g_ctx, "origin {}", ES2PANDA_PLUGIN_SUGGESTION);
    const char *params[] = {
        "a",
    };
    auto diagnostikind = g_impl->CreateDiagnosticKind(g_ctx, "error", ES2PANDA_PLUGIN_ERROR);

    es2panda_SourcePosition *left = g_impl->CreateSourcePosition(g_ctx, 0, 0);
    es2panda_SourcePosition *right = g_impl->CreateSourcePosition(g_ctx, 7, 0);
    es2panda_SourceRange *range = g_impl->CreateSourceRange(g_ctx, left, right);

    auto suggestionInfo = g_impl->CreateSuggestionInfo(g_ctx, suggestionkind, params, 1, "replace b", "Title:", range);
    auto diagnosticInfo = g_impl->CreateDiagnosticInfo(g_ctx, diagnostikind, nullptr, 0, left);
    g_impl->LogDiagnosticWithSuggestion(g_ctx, diagnosticInfo, suggestionInfo);

    auto errors = g_impl->GetPluginErrors(g_ctx);
    auto diagnosticStorage = reinterpret_cast<const ark::es2panda::util::DiagnosticStorage *>(errors);
    if (diagnosticStorage->size() != 1 || strcmp((*diagnosticStorage)[0]->Message().data(), "error") != 0) {
        return TEST_ERROR_CODE;
    }

    const size_t startLine = 0;
    const size_t endLine = 0;
    const size_t startIndex = 0;
    const size_t endIndex = 7;

    auto diagnostic = reinterpret_cast<const ark::es2panda::util::Diagnostic *>(&(*(*diagnosticStorage)[0]));
    auto suggestion = (diagnostic->Suggestion())[0];
    if (strcmp(suggestion->SubstitutionCode().data(), "replace b") != 0 ||
        strcmp(suggestion->Title().data(), "Title:") != 0 || strcmp(suggestion->Message().data(), "origin a") != 0 ||
        suggestion->SourceRange()->start.line != startLine || suggestion->SourceRange()->end.line != endLine ||
        suggestion->SourceRange()->start.index != startIndex || suggestion->SourceRange()->end.index != endIndex) {
        return TEST_ERROR_CODE;
    }

    g_impl->DestroyContext(g_ctx);
    g_impl->DestroyConfig(config);

    return 0;
}

// NOLINTEND
