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

#include "lsp/include/api.h"
#include "lsp/include/completions.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/formatting/formatting_settings.h"
#include "lsp/include/internal_api.h"
#include "lsp_api_test.h"
#include <gtest/gtest.h>

using ark::es2panda::lsp::CompletionInfo;
using ark::es2panda::lsp::Initializer;

namespace {

class LspErrorInputTests : public LSPAPITests {};

// Existing completions/definition negatives use in-file positions only; these cover the
// CodePointOffsetToByteOffset EOF-clamp branch (SIZE_MAX; the beyond-EOF 99999 variant
// was removed as a same-branch duplicate).
TEST_F(LspErrorInputTests, GetCompletions_NegativePosition_NotCrash)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string source = R"(
class A {
    a: number = 1;
    method(): void {}
}
)";
    auto filePaths = CreateTempFile({"error_neg_pos.ets"}, {source});
    ASSERT_FALSE(filePaths.empty());

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    LSPAPI const *lspApi = GetImpl();
    // Pass static_cast<size_t>(-1) — the largest representable size_t value,
    // which triggers CodePointOffsetToByteOffset EOF clamping.
    size_t negativePosition = static_cast<size_t>(-1);
    CompletionInfo result = lspApi->getCompletionsAtPosition(ctx, negativePosition);

    // The designed behavior: completion at EOF (clamped) returns an empty result
    ASSERT_TRUE(result.GetEntries().empty());

    initializer.DestroyContext(ctx);
}

// Out-of-file positions exercise the lsp_utils EOF clamp; upstream get-definition negatives
// use in-file non-identifier positions.
TEST_F(LspErrorInputTests, GetDefinition_NegativePosition_NotCrash)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string source = R"(
class A {}
let a = new A();
)";
    auto filePaths = CreateTempFile({"error_neg_def_pos.ets"}, {source});
    ASSERT_FALSE(filePaths.empty());

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    LSPAPI const *lspApi = GetImpl();
    // Pass static_cast<size_t>(-1) as position
    size_t negativePosition = static_cast<size_t>(-1);
    DefinitionInfo result = lspApi->getDefinitionAtPosition(ctx, negativePosition);

    // The designed behavior: negative position clamps to EOF, no definition exists there
    ASSERT_TRUE(result.fileName.empty());
    ASSERT_EQ(result.start, 0U);
    ASSERT_EQ(result.length, 0U);

    initializer.DestroyContext(ctx);
}

// Public wrapper guard (api.cpp) vs internal FormatDocument(nullptr) covered by
// formatting_test_4.cpp FormatDocument_NullContext.
TEST_F(LspErrorInputTests, FormatDocument_NullContext_NotCrash)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    LSPAPI const *lspApi = GetImpl();

    // Call the public formatting wrapper with a null context
    ark::es2panda::lsp::FormatCodeSettings settings;
    std::vector<TextChange> result = lspApi->getFormattingEditsForDocument(nullptr, settings);

    // The public wrapper checks for nullptr and returns an empty vector
    ASSERT_TRUE(result.empty());

    // No context to destroy — we passed nullptr
}

}  // namespace