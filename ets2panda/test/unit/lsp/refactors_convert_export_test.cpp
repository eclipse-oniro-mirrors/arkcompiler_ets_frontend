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

#include <gtest/gtest.h>
#include <cstddef>
#include <string>
#include "lsp_api_test.h"
#include "lsp/include/applicable_refactors.h"

namespace {
using ark::es2panda::lsp::Initializer;

class LspConvExpTests : public LSPAPITests {
public:
    static constexpr std::string_view TO_NAMED_EXPORT_KIND = "refactor.rewrite.export.named";
    static constexpr std::string_view TO_NAMED_EXPORT_NAME = "Convert default export to named export";
    static constexpr std::string_view TO_DEFAULT_EXPORT_KIND = "refactor.rewrite.export.default";
    static constexpr std::string_view TO_DEFAULT_EXPORT_NAME = "Convert named export to default export";
};

TEST_F(LspConvExpTests, ConvertExportRefactor1)
{
    std::vector<std::string> files = {"convertExportRefactor1.ets"};
    std::vector<std::string> texts = {R"(export default function add(a: number, b: number): number {
  return a + b;
})"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const position = 27;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(TO_NAMED_EXPORT_KIND);
    refactorContext.span.pos = position;
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(1, result.size());
    ASSERT_EQ(std::string(TO_NAMED_EXPORT_NAME), result[0].action.name);
}

TEST_F(LspConvExpTests, ConvertExportRefactor2)
{
    std::vector<std::string> files = {"convertExportRefactor2.ets"};
    std::vector<std::string> texts = {R"(export default class User {
  name: string = "";
})"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const position = 25;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(TO_NAMED_EXPORT_KIND);
    refactorContext.span.pos = position;
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(1, result.size());
    ASSERT_EQ(std::string(TO_NAMED_EXPORT_NAME), result[0].action.name);
}

TEST_F(LspConvExpTests, ConvertExportRefactor5)
{
    std::vector<std::string> files = {"convertExportRefactor5.ets"};
    std::vector<std::string> texts = {R"(export function add(a: number, b: number): number {
  return a + b;
})"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const position = 19;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(TO_DEFAULT_EXPORT_KIND);
    refactorContext.span.pos = position;
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(1, result.size());
    ASSERT_EQ(std::string(TO_DEFAULT_EXPORT_NAME), result[0].action.name);
}
}  // namespace