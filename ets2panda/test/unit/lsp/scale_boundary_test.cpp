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
#include <string>
#include <vector>

#include "lsp_api_test.h"
#include "lsp/include/get_definition_and_bound_span.h"
#include "lsp/include/quick_info.h"
#include "lsp/include/api.h"

namespace {

using ark::es2panda::lsp::DefinitionInfoAndBoundSpan;
using ark::es2panda::lsp::GetDefinitionAndBoundSpan;
using ark::es2panda::lsp::Initializer;

class LspScaleBoundaryTests : public LSPAPITests {};

std::string BuildDeepGenericSource(size_t typeParameterCount)
{
    std::string source = "class DeepBox<";
    for (size_t index = 0; index < typeParameterCount; ++index) {
        if (index > 0U) {
            source += ", ";
        }
        source += "T" + std::to_string(index);
    }
    source += "> {\n";
    source += "  value: T0;\n";
    source += "  last: T19;\n";
    source += "}\n\n";
    source +=
        "let box: DeepBox<number, string, boolean, number, string, boolean, number, string, boolean, number, string, "
        "boolean, number, string, boolean, number, string, boolean, number, string> = ";
    source +=
        "new DeepBox<number, string, boolean, number, string, boolean, number, string, boolean, number, string, "
        "boolean, number, string, boolean, number, string, boolean, number, string>();\n";
    return source;
}

void VerifyDeepGenericQuickInfo(const QuickInfo &quickInfo, size_t offset, const std::string &fileName)
{
    EXPECT_NE(quickInfo, QuickInfo());
    EXPECT_FALSE(quickInfo.GetKind().empty());
    EXPECT_TRUE(quickInfo.GetKind() == "class" || quickInfo.GetKind() == "type alias" ||
                quickInfo.GetKind() == "interface" || quickInfo.GetKind() == "type");

    TextSpan span = quickInfo.GetTextSpan();
    EXPECT_GT(span.length, 0U);
    EXPECT_GE(span.start, offset);

    auto displayParts = quickInfo.GetDisplayParts();
    EXPECT_FALSE(displayParts.empty());
    bool foundDeepBox = false;
    for (const auto &part : displayParts) {
        if (part.GetText().find("DeepBox") != std::string::npos) {
            foundDeepBox = true;
            break;
        }
    }
    EXPECT_TRUE(foundDeepBox) << "Display parts should contain 'DeepBox'";
    EXPECT_EQ(quickInfo.GetFileName(), fileName);
}

/* P1D1-SCALE-03: GetDefinition_DeepNestedAST_50Levels
   Generate syntactically valid ArkTS with 50 nested levels and a resolvable definition.
   Uses nested call expressions: id(id(...id(target)...)) with 50 levels.
   Assert exact declaration file/span and bound span. */
TEST_F(LspScaleBoundaryTests, GetDefinition_DeepNestedAST_50Levels)
{
    const size_t nestLevels = 50U;

    // Build source: let target = 42; function id(x) { return x; } let result = id(id(...id(target)...));
    std::string source = "let target: number = 42;\n";
    source += "function id(x: number): number { return x; }\n";
    source += "let result: number = ";

    // Build 50 nested id calls
    for (size_t i = 0; i < nestLevels; ++i) {
        source += "id(";
    }
    source += "target";
    for (size_t i = 0; i < nestLevels; ++i) {
        source += ")";
    }
    source += ";\n";

    const std::string fileName = "deep_nest_50.ets";
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {source};

    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer init;
    auto ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    // Position at the innermost 'target' (the last occurrence of "target" in source)
    const std::string targetName = "target";
    size_t offset = source.rfind(targetName);
    ASSERT_NE(offset, std::string::npos);

    auto result = GetDefinitionAndBoundSpan(ctx, offset);

    // Should resolve to 'let target' declaration
    EXPECT_FALSE(result.definitionInfo.fileName.empty());
    EXPECT_EQ(result.definitionInfo.fileName, filePaths[0]);

    // Definition of 'target' is at "let target" -> find "target" after "let "
    size_t expectedDefStart = source.find(targetName);
    ASSERT_NE(expectedDefStart, std::string::npos);
    EXPECT_EQ(result.definitionInfo.start, expectedDefStart);
    EXPECT_EQ(result.definitionInfo.length, targetName.size());

    // Bound span should cover the innermost usage
    EXPECT_EQ(result.boundSpan.start, offset);
    EXPECT_EQ(result.boundSpan.length, targetName.size());

    init.DestroyContext(ctx);
}

/* P1D1-SCALE-07: QuickInfo_DeepGenericType_20Params
   Generate a valid 20-parameter generic type and assert exact non-empty
   span/kind/display content based on current QuickInfo types. */
TEST_F(LspScaleBoundaryTests, QuickInfo_DeepGenericType_20Params)
{
    const size_t typeParameterCount = 20U;
    std::string source = BuildDeepGenericSource(typeParameterCount);

    const std::string fileName = "deep_generic_20.ets";
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {source};

    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer init;
    auto ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    // Position at 'DeepBox' in the variable declaration type annotation
    // Find "DeepBox<" in the variable declaration (the second occurrence)
    size_t firstDeepBox = source.find("DeepBox<");
    ASSERT_NE(firstDeepBox, std::string::npos);
    size_t offset = source.find("DeepBox<", firstDeepBox + 1);
    ASSERT_NE(offset, std::string::npos);

    const LSPAPI *lspApi = GetImpl();
    auto quickInfo = lspApi->getQuickInfoAtPosition(fileName.c_str(), ctx, offset);

    VerifyDeepGenericQuickInfo(quickInfo, offset, fileName);

    // Document and tags may be empty for this construct (no JSDoc)
    // Just verify they are accessible
    auto document = quickInfo.GetDocument();
    auto tags = quickInfo.GetTags();
    (void)document;
    (void)tags;

    init.DestroyContext(ctx);
}

}  // namespace
