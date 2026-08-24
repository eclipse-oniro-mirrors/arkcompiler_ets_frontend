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

#include "lsp_api_test.h"

#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

#include <algorithm>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <vector>

namespace {

using ark::es2panda::lsp::Initializer;

class LspIncrementalPrepareTest : public LSPAPITests {};

constexpr int INCREMENTAL_PREPARE_REPARSE = 0;
constexpr int INCREMENTAL_PREPARE_REUSE_AST = 1;
constexpr int LSP_API_SUCCESS = 0;
constexpr size_t SOURCE_FILE_INDEX = 0;
constexpr size_t UNRELATED_FILE_INDEX = 1;
constexpr size_t CONSUMER_FILE_INDEX = 2;
constexpr bool FILE_CHANGED = true;
constexpr bool FILE_UNCHANGED = false;

bool HasDiagnosticContaining(const DiagnosticReferences &diagnostics, std::string_view text)
{
    return std::any_of(diagnostics.diagnostic.begin(), diagnostics.diagnostic.end(),
                       [text](const auto &diagnostic) { return diagnostic.message_.find(text) != std::string::npos; });
}

void WriteFile(const std::string &fileName, const std::string &text)
{
    std::ofstream out {fileName, std::ios::trunc};
    ASSERT_TRUE(out.is_open());
    out << text;
}

TEST_F(LspIncrementalPrepareTest, IncrementalPrepareInvalidatesChangedExportWithoutClearingUnrelatedProgram)
{
    std::vector<std::string> files = {
        "source.ets",
        "unrelated.ets",
        "consumer.ets",
    };
    std::vector<std::string> texts = {
        R"ETS(export class Source {})ETS",
        R"ETS(export class Unrelated {})ETS",
        R"ETS(
            import { Source } from "./source.ets";
            import { Unrelated } from "./unrelated.ets";
            let value: Source | null = null;
            let unrelated: Unrelated | null = null;
            console.log(value, unrelated);
        )ETS",
    };
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    const auto &sourceFile = filePaths[SOURCE_FILE_INDEX];
    const auto &unrelatedFile = filePaths[UNRELATED_FILE_INDEX];
    const auto &consumerFile = filePaths[CONSUMER_FILE_INDEX];
    const auto &unrelatedText = texts[UNRELATED_FILE_INDEX];
    const auto &consumerText = texts[CONSUMER_FILE_INDEX];

    auto *publicApi = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    auto *lspApi = GetImpl();
    Initializer initializer;
    auto *ctx = initializer.CreateContext(consumerFile.c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    ASSERT_EQ(publicApi->ContextState(ctx), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(lspApi->getSemanticDiagnostics(ctx).diagnostic.size(), 0U);

    const std::string changedSource = R"ETS(export class RenamedSource {})ETS";
    ASSERT_NO_FATAL_FAILURE(WriteFile(sourceFile, changedSource));

    ASSERT_EQ(publicApi->IncrementalPrepareProgram(ctx, sourceFile.c_str(), changedSource.c_str(), FILE_CHANGED),
              INCREMENTAL_PREPARE_REPARSE);

    ASSERT_EQ(lspApi->DeleteDependantProgramsForFiles(ctx, sourceFile.c_str()), LSP_API_SUCCESS);
    ASSERT_EQ(publicApi->IncrementalPrepareProgram(ctx, unrelatedFile.c_str(), unrelatedText.c_str(), FILE_UNCHANGED),
              INCREMENTAL_PREPARE_REUSE_AST);
    ASSERT_EQ(publicApi->IncrementalPrepareProgram(ctx, consumerFile.c_str(), consumerText.c_str(), FILE_UNCHANGED),
              INCREMENTAL_PREPARE_REPARSE);
    initializer.DestroyContext(ctx);

    auto *verifyCtx = initializer.CreateContext(consumerFile.c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(verifyCtx, nullptr);
    const auto diagnostics = lspApi->getSemanticDiagnostics(verifyCtx);
    EXPECT_GT(diagnostics.diagnostic.size(), 0U);
    EXPECT_TRUE(HasDiagnosticContaining(diagnostics, "Source"));
    EXPECT_FALSE(HasDiagnosticContaining(diagnostics, "Unrelated"));

    initializer.DestroyContext(verifyCtx);
}

TEST_F(LspIncrementalPrepareTest, IncrementalPrepareChangedFileCanProceedToChecked)
{
    std::vector<std::string> files = {"changed.ets"};
    std::vector<std::string> texts = {
        R"ETS(
            export function value(): int {
                return 1;
            }
        )ETS",
    };
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    const auto &changedFile = filePaths[SOURCE_FILE_INDEX];
    const auto &initialText = texts[SOURCE_FILE_INDEX];

    auto *publicApi = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    auto *lspApi = GetImpl();
    ark::es2panda::EHeap::Scope eheapScope;
    Initializer initializer;
    std::vector<std::string> activeTexts {initialText};
    auto *ctx = initializer.CreateContextWithCache(changedFile.c_str(), ES2PANDA_STATE_CHECKED, activeTexts);
    ASSERT_NE(ctx, nullptr);
    ASSERT_EQ(publicApi->ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const std::string changedText = R"ETS(
        export function value(): int {
            return 2;
        }
    )ETS";
    ASSERT_NO_FATAL_FAILURE(WriteFile(changedFile, changedText));

    ASSERT_EQ(publicApi->IncrementalPrepareProgram(ctx, changedFile.c_str(), changedText.c_str(), FILE_CHANGED),
              INCREMENTAL_PREPARE_REPARSE);
    ctx = publicApi->ProceedToState(ctx, ES2PANDA_STATE_CHECKED);
    const auto diagnostics = lspApi->getSemanticDiagnostics(ctx);
    ASSERT_EQ(publicApi->ContextState(ctx), ES2PANDA_STATE_CHECKED)
        << publicApi->ContextErrorMessage(ctx) << " diagnostic count=" << diagnostics.diagnostic.size();

    initializer.DestroyContext(ctx);
}

}  // namespace
