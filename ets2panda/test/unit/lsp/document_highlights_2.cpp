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

#include <gtest/gtest.h>

#include "lsp/include/api.h"
#include "lsp/include/internal_api.h"

namespace {

using ark::es2panda::lsp::Initializer;

class LspDocumentHighlights2 : public LSPAPITests {};

struct SpanExpectation {
    size_t start;
    size_t length;
    HighlightSpanKind kind;
};

void AssertHighlightSpans(const DocumentHighlightsReferences &result, const std::string &fileName,
                          const std::vector<SpanExpectation> &expected)
{
    ASSERT_EQ(result.documentHighlights_.size(), 1U);
    EXPECT_EQ(result.documentHighlights_[0].fileName_, fileName);
    const auto &spans = result.documentHighlights_[0].highlightSpans_;
    ASSERT_EQ(spans.size(), expected.size());
    for (size_t i = 0; i < expected.size(); i++) {
        EXPECT_EQ(spans[i].textSpan_.start, expected[i].start);
        EXPECT_EQ(spans[i].textSpan_.length, expected[i].length);
        EXPECT_EQ(spans[i].kind_, expected[i].kind);
    }
}

// getDocumentHighlights on an imported symbol must keep only the spans of the queried file:
// the definition and the occurrences living in the exporting file are filtered out by
// MakeHighlightSpansFromIndexedReferences (spanFileName != fileName). A locally declared symbol
// still keeps its WRITTEN_REFERENCE definition span, proving the filter is driven by the span's
// owning file and not by the highlight kind.
TEST_F(LspDocumentHighlights2, CrossFileHighlightsAreFilteredToQueriedFile)
{
    std::vector<std::string> files = {"CrossFileHighlightsExport.ets", "CrossFileHighlightsImport.ets"};
    std::vector<std::string> texts = {"export class Foo {\n    v: number = 0;\n}\n",
                                      R"(import { Foo } from './CrossFileHighlightsExport';

let f: Foo = new Foo();
f.v = 1;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContextWithExternal(context));

    const std::string &source = texts[1];

    // Querying the imported class keeps exactly the importer-local occurrences (import specifier,
    // type annotation, constructor call); its definition span lives in the exporter file and is
    // filtered away.
    const auto classPos = source.find("Foo");
    ASSERT_NE(classPos, std::string::npos);
    auto classResult = lspApi->getDocumentHighlights(context, classPos);

    const std::vector<SpanExpectation> expectedClassSpans {
        {source.find("Foo"), 3U, HighlightSpanKind::REFERENCE},
        {source.find(": Foo") + 2U, 3U, HighlightSpanKind::REFERENCE},
        {source.find("new Foo") + 4U, 3U, HighlightSpanKind::REFERENCE}};
    AssertHighlightSpans(classResult, filePaths[1], expectedClassSpans);
    for (const auto &span : classResult.documentHighlights_[0].highlightSpans_) {
        EXPECT_NE(span.kind_, HighlightSpanKind::WRITTEN_REFERENCE);
    }

    // The declaration `let f` is in this file, so it stays a WRITTEN_REFERENCE next to its read
    // reference in `f.v`.
    const auto varPos = source.find("f.v");
    ASSERT_NE(varPos, std::string::npos);
    auto varResult = lspApi->getDocumentHighlights(context, varPos);
    initializer.DestroyContext(context);

    const std::vector<SpanExpectation> expectedVarSpans {
        {source.find("let f") + 4U, 1U, HighlightSpanKind::WRITTEN_REFERENCE},
        {varPos, 1U, HighlightSpanKind::REFERENCE}};
    AssertHighlightSpans(varResult, filePaths[1], expectedVarSpans);
}

// Positions that touch no identifier or primitive type produce empty highlight results instead of
// crashing: punctuation tokens, tokens inside string literals, and offsets beyond EOF.
TEST_F(LspDocumentHighlights2, NonIdentifierPositionsReturnEmptyHighlights)
{
    std::vector<std::string> files = {"NonIdentifierPositionsReturnEmptyHighlights.ets"};
    std::vector<std::string> texts = {"let value = \"text\";\nlet other = 123;\n"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    LSPAPI const *lspApi = GetImpl();

    const std::string &source = texts[0];
    const size_t assignPos = source.find('=');
    const size_t stringContentPos = source.find("text") + 1U;
    const size_t beyondEofPos = source.size() + 10U;

    auto assignResult = lspApi->getDocumentHighlights(context, assignPos);
    auto stringResult = lspApi->getDocumentHighlights(context, stringContentPos);
    auto eofResult = lspApi->getDocumentHighlights(context, beyondEofPos);
    initializer.DestroyContext(context);

    for (const auto *result : {&assignResult, &stringResult, &eofResult}) {
        ASSERT_EQ(result->documentHighlights_.size(), 1U);
        EXPECT_EQ(result->documentHighlights_[0].fileName_, filePaths[0]);
        EXPECT_TRUE(result->documentHighlights_[0].highlightSpans_.empty());
    }
}

}  // namespace
