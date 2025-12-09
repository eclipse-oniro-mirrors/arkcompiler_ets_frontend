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

class LspConvFuncTests : public LSPAPITests {
public:
    static constexpr std::string_view TO_NAMED_FUNCTION_KIND = "refactor.rewrite.function.named";
    static constexpr std::string_view INVALID_KIND = "aaabbbccc";
    static constexpr std::string_view TO_NAMED_FUNCTION_NAME = "Convert to named function";
};

TEST_F(LspConvFuncTests, ConvertFunctionRefactor1)
{
    std::vector<std::string> files = {"convertFunctionRefactor1.ets"};
    std::vector<std::string> texts = {R"(const add = (x: number, y: number): number => {
     return x + y;
 };)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const position = 8;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(TO_NAMED_FUNCTION_KIND);
    refactorContext.span.pos = position;
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(1, result.size());
    ASSERT_EQ(std::string(TO_NAMED_FUNCTION_NAME), result[0].action.name);
}

TEST_F(LspConvFuncTests, ConvertFunctionRefactor2)
{
    std::vector<std::string> files = {"convertFunctionRefactor2.ets"};
    std::vector<std::string> texts = {R"(function sub(a: number, b: number): number{
     return a - b;
 };)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const position = 11;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(TO_NAMED_FUNCTION_KIND);
    refactorContext.span.pos = position;
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(0, result.size());
}

TEST_F(LspConvFuncTests, ConvertFunctionRefactor3)
{
    std::vector<std::string> files = {"convertFunctionRefactor3.ets"};
    std::vector<std::string> texts = {R"(const add = (x: number, y: number): number => {
     return x + y;
 };)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const position = 8;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(INVALID_KIND);
    refactorContext.span.pos = position;
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(0, result.size());
}

TEST_F(LspConvFuncTests, ConvertFunctionRefactor4)
{
    std::vector<std::string> files = {"convertFunctionRefactor4.ets"};
    std::vector<std::string> texts = {R"(const foo = a => {
let b = 1;
return a + b;
};)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    size_t const position = 13;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(TO_NAMED_FUNCTION_KIND);
    refactorContext.span.pos = position;
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(1, result.size());
    ASSERT_EQ(std::string(TO_NAMED_FUNCTION_NAME), result[0].action.name);
}

TEST_F(LspConvFuncTests, ConvertFunctionRefactor5)
{
    std::vector<std::string> files = {"convertFunctionRefactor5.ets"};
    std::vector<std::string> texts = {R"(function doSomething(a){}
doSomething(() => 1 + 1);)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const position = 41;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(TO_NAMED_FUNCTION_KIND);
    refactorContext.span.pos = position;
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(0, result.size());
}

TEST_F(LspConvFuncTests, ConvertFunctionRefactor6)
{
    std::vector<std::string> files = {"convertFunctionRefactor6.ets"};
    std::vector<std::string> texts = {R"(const foo = (a,b,c) => a + 1;)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const position = 14;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(TO_NAMED_FUNCTION_KIND);
    refactorContext.span.pos = position;
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(1, result.size());
    ASSERT_EQ(std::string(TO_NAMED_FUNCTION_NAME), result[0].action.name);
}

TEST_F(LspConvFuncTests, ConvertFunctionRefactor7)
{
    std::vector<std::string> files = {"convertFunctionRefactor7.ets"};
    std::vector<std::string> texts = {R"(const foo = () => //comment
1)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const position = 14;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(TO_NAMED_FUNCTION_KIND);
    refactorContext.span.pos = position;
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(1, result.size());
    ASSERT_EQ(std::string(TO_NAMED_FUNCTION_NAME), result[0].action.name);
}

TEST_F(LspConvFuncTests, ConvertFunctionRefactor8)
{
    std::vector<std::string> files = {"convertFunctionRefactor8.ets"};
    std::vector<std::string> texts = {R"(function func() {
const test = () => {
}
})"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const position = 28;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(TO_NAMED_FUNCTION_KIND);
    refactorContext.span.pos = position;
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(1, result.size());
}

TEST_F(LspConvFuncTests, ConvertFunctionRefactor9)
{
    std::vector<std::string> files = {"convertFunctionRefactor9.ets"};
    std::vector<std::string> texts = {R"(class AA {
func() {
const test = () => {}
}
})"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const position = 31;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(TO_NAMED_FUNCTION_KIND);
    refactorContext.span.pos = position;
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(1, result.size());
}

TEST_F(LspConvFuncTests, ConvertFunctionRefactor10)
{
    std::vector<std::string> files = {"convertFunctionRefactor10.ets"};
    std::vector<std::string> texts = {R"(const func = () => {
const test = () => {}
})"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const position = 31;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(TO_NAMED_FUNCTION_KIND);
    refactorContext.span.pos = position;
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(1, result.size());
}

}  // namespace