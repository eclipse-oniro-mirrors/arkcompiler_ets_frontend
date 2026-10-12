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

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <string>

#include "lsp/include/internal_api.h"
#include "lsp_api_test.h"
#include "util/path.h"

namespace {

using ark::es2panda::lsp::Initializer;

class InternalApiArmsTests : public LSPAPITests {};

// PF-010 — tier-2 arm sweep on internal_api.cpp through real public/internal API calls.
// Every test cites its target internal_api.cpp lines in the body comments.

// ComputeRelativeImportPath (internal_api.cpp:72-116) segment walk-up/walk-down loops
// (100-106), "./" prefix arm (111-113), empty-segment skip arms (84-93) and StripExtension
// (57-68) suffix arms (.ets 62-63, .ts 64-65, .d.ets 60-61, no-extension default). Pure
// path contract exercised on real file-path shapes, including a doubled separator.
TEST_F(InternalApiArmsTests, ComputeRelativeImportPathMatrix)
{
    using ark::es2panda::lsp::ComputeRelativeImportPath;
    // Same directory: equal segment lists, relative name gets the "./" prefix, ".ets" strip.
    EXPECT_EQ(ComputeRelativeImportPath("/prj/feature/A.ets", "/prj/feature/B.ets"), "./B");
    // Target nested deeper: only the walk-down loop (104-106) contributes.
    EXPECT_EQ(ComputeRelativeImportPath("/prj/feature/A.ets", "/prj/feature/sub/B.ets"), "./sub/B");
    // Target one level up: walk-up loop (100-102) emits one "../".
    EXPECT_EQ(ComputeRelativeImportPath("/prj/feature/A.ets", "/prj/B.ets"), "../B");
    // Diverging at the root segment: two "../" plus the sibling directory walk-down.
    EXPECT_EQ(ComputeRelativeImportPath("/prj/feature/A.ets", "/lib/dep/B.ets"), "../../lib/dep/B");
    // Deep divergence: three "../" then one "e/" walk-down segment.
    EXPECT_EQ(ComputeRelativeImportPath("/a/b/c/d/A.ets", "/a/e/B.ets"), "../../../e/B");
    // Doubled separator: the empty-segment skip arms keep the segment lists clean, and the
    // ".ts" suffix arm of StripExtension fires.
    EXPECT_EQ(ComputeRelativeImportPath("/prj//feature/A.ets", "/prj/feature/C.ts"), "./C");
    // ".d.ets" is stripped before the plain ".ets" suffix would match (60-61).
    EXPECT_EQ(ComputeRelativeImportPath("/prj/feature/A.ets", "/prj/feature/B.d.ets"), "./B");
    // Extension-less target: StripExtension takes the implicit default arm.
    EXPECT_EQ(ComputeRelativeImportPath("/prj/feature/A.ets", "/prj/feature/B"), "./B");
}

// Initializer ctor BUILD_FOLDER env override arm (internal_api.cpp:127-130) and the
// ~Initializer DestroyConfig arm (internal_api.cpp:137-138) for Initializer(true); the
// default-constructed initializer always takes DestroyConfigWithoutLog (139-140). The env
// value is the exact string the compile-time macro default produces, so context creation
// behavior is unchanged; the previous env value is restored afterwards.
TEST_F(InternalApiArmsTests, InitializerEnvOverrideAndLoggedDestruction)
{
    const char *previousValue = std::getenv("BUILD_FOLDER");
    const std::string overridden =
        std::string(BUILD_FOLDER) + ark::es2panda::util::PATH_DELIMITER + "bin" + ark::es2panda::util::PATH_DELIMITER;
    ASSERT_EQ(setenv("BUILD_FOLDER", overridden.c_str(), 1), 0);

    {
        Initializer loggedInitializer(true);
        es2panda_Context *ctx =
            loggedInitializer.CreateContext("ArmsInitializerEnv.ets", ES2PANDA_STATE_PARSED, "let a = 1;\n");
        ASSERT_NE(ctx, nullptr);
        ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
        loggedInitializer.DestroyContext(ctx);
    }  // ~Initializer(true) runs the DestroyConfig arm.

    if (previousValue != nullptr) {
        ASSERT_EQ(setenv("BUILD_FOLDER", previousValue, 1), 0);
    } else {
        ASSERT_EQ(unsetenv("BUILD_FOLDER"), 0);
    }
}

// GetCurrentTokenValue (api.cpp:314) reaches GetCurrentTokenValueImpl with a null preceding
// token (internal_api.cpp:695-697) and then the re-lexing fallback
// GetCurrentTokenValueFromLexer (internal_api.cpp:671-689): a position before the first
// token start fails the containing-token test (680) and takes the break arm (683-684), and
// a position inside a comment between tokens fails the containment second condition and
// breaks at the next token. Both negative positions yield the empty string.
TEST_F(InternalApiArmsTests, GetCurrentTokenValueLexerFallbackArms)
{
    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();

    es2panda_Context *leadingSpaceCtx =
        initializer.CreateContext("ArmsTokenLeading.ets", ES2PANDA_STATE_CHECKED, " let s = 1;");
    ASSERT_NE(leadingSpaceCtx, nullptr);
    EXPECT_EQ(lspApi->getCurrentTokenValue(leadingSpaceCtx, 0), "");
    initializer.DestroyContext(leadingSpaceCtx);

    es2panda_Context *commentCtx =
        initializer.CreateContext("ArmsTokenComment.ets", ES2PANDA_STATE_CHECKED, "let v = 1; // note\nlet w = 2;");
    ASSERT_NE(commentCtx, nullptr);
    const size_t commentOffset = 15;  // inside "// note"
    EXPECT_EQ(lspApi->getCurrentTokenValue(commentCtx, commentOffset), "");
    initializer.DestroyContext(commentCtx);

    es2panda_Context *eofCtx = initializer.CreateContext("ArmsTokenEof.ets", ES2PANDA_STATE_CHECKED, "let s = 1;");
    ASSERT_NE(eofCtx, nullptr);
    // Measured (2026-09-11): on the re-lexed CHECKED source the final ";" token end extends
    // past the last real byte, so every position beyond the final token resolves through the
    // containment return (680-681) to Substr(finalToken.Start, pos) = ";" — the EOS-exit ""
    // return (688) is not reachable through this route with this source shape.
    EXPECT_EQ(lspApi->getCurrentTokenValue(eofCtx, 40U), ";");
    initializer.DestroyContext(eofCtx);
}

// GetImportFilePath(context, pos) (internal_api.cpp:1460-1472): the touching token of a
// module specifier is the ETSImportDeclaration source and returns the resolved import path
// (1468-1470); a position beyond EOF has no touching token and returns "" (1463-1465); a
// position on another token of the import or on a non-import statement fails the
// parent/Source test and returns "" (1468 FALSE -> 1471).
TEST_F(InternalApiArmsTests, GetImportFilePathFallbackArms)
{
    std::vector<std::string> files = {"ArmsImportFrank.ets", "ArmsImportMain.ets"};
    std::vector<std::string> texts = {"export function frank(): void {}\n",
                                      "import { frank } from './ArmsImportFrank';\nfrank();"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    const std::string &source = texts[1];
    const size_t specifierPos = source.find("'./ArmsImportFrank'") + 1U;  // inside the module specifier
    const size_t importNamePos = source.find("frank");                    // inside the named specifier
    const size_t callPos = source.rfind("frank");                         // call statement, not an import
    const size_t beyondEofPos = source.size() + 10U;

    const std::string specifierPath = ark::es2panda::lsp::GetImportFilePath(ctx, specifierPos);
    const std::string importNamePath = ark::es2panda::lsp::GetImportFilePath(ctx, importNamePos);
    const std::string callPath = ark::es2panda::lsp::GetImportFilePath(ctx, callPos);
    const std::string eofPath = ark::es2panda::lsp::GetImportFilePath(ctx, beyondEofPos);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(specifierPath, filePaths[0]);
    EXPECT_EQ(importNamePath, "");
    EXPECT_EQ(callPath, "");
    EXPECT_EQ(eofPath, "");
}

// GetRangeOfEnclosingComment (internal_api.cpp:791-816) comment-scan arms in
// GetRangeOfCommentFromContext: a trailing line comment without a newline takes the
// lineCommentEnd == npos arm (775-777); a block comment located past the right search
// boundary takes the early break arm (759-760); and a PARSED-state context has no global
// class declaration yet, so GetChildren (internal_api.cpp:520-527) takes the
// classNode == nullptr arm and still resolves the enclosing comment span.
TEST_F(InternalApiArmsTests, SpanOfEnclosingCommentCommentKindAndModuleFallbackArms)
{
    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();

    // Trailing line comment at EOF without newline.
    std::vector<std::string> noNewlineFiles = {"ArmsSpanNoNewline.ets"};
    std::vector<std::string> noNewlineTexts = {"let a // tail comment"};
    auto noNewlinePaths = CreateTempFile(noNewlineFiles, noNewlineTexts);
    ASSERT_EQ(noNewlinePaths.size(), 1U);
    es2panda_Context *noNewlineCtx = initializer.CreateContext(noNewlinePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(noNewlineCtx, nullptr);
    const size_t tailPos = noNewlineTexts[0].find("tail");
    TextSpan tailSpan = lspApi->getSpanOfEnclosingComment(noNewlineCtx, tailPos, false);
    initializer.DestroyContext(noNewlineCtx);
    EXPECT_EQ(tailSpan.start, noNewlineTexts[0].find("//"));
    EXPECT_EQ(tailSpan.length, noNewlineTexts[0].size() - noNewlineTexts[0].find("//"));

    // Line comment queried while a block comment sits beyond the right search boundary.
    std::vector<std::string> twoCommentFiles = {"ArmsSpanTwoComments.ets"};
    std::vector<std::string> twoCommentTexts = {"let a // c1\nlet b /* c2 */"};
    auto twoCommentPaths = CreateTempFile(twoCommentFiles, twoCommentTexts);
    ASSERT_EQ(twoCommentPaths.size(), 1U);
    es2panda_Context *twoCommentCtx = initializer.CreateContext(twoCommentPaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(twoCommentCtx, nullptr);
    const size_t c1Pos = twoCommentTexts[0].find("c1");
    TextSpan c1Span = lspApi->getSpanOfEnclosingComment(twoCommentCtx, c1Pos, false);
    initializer.DestroyContext(twoCommentCtx);
    EXPECT_EQ(c1Span.start, twoCommentTexts[0].find("//"));
    EXPECT_EQ(c1Span.length, std::string("// c1").size());

    // PARSED state: ETS_MODULE without a global class declaration yet.
    std::vector<std::string> parsedFiles = {"ArmsSpanParsed.ets"};
    std::vector<std::string> parsedTexts = {"function A() { return 1; } // done"};
    auto parsedPaths = CreateTempFile(parsedFiles, parsedTexts);
    ASSERT_EQ(parsedPaths.size(), 1U);
    es2panda_Context *parsedCtx = initializer.CreateContext(parsedPaths[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(parsedCtx, nullptr);
    ASSERT_EQ(ContextState(parsedCtx), ES2PANDA_STATE_PARSED);
    const size_t donePos = parsedTexts[0].find("done");
    TextSpan parsedSpan = lspApi->getSpanOfEnclosingComment(parsedCtx, donePos, false);
    initializer.DestroyContext(parsedCtx);
    EXPECT_EQ(parsedSpan.start, parsedTexts[0].find("//"));
    EXPECT_EQ(parsedSpan.length, std::string("// done").size());
}

}  // namespace
