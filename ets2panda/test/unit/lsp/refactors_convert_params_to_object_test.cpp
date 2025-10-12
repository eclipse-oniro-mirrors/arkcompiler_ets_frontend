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
#include "lsp/include/refactors/convert_params_to_object.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/services/text_change/text_change_context.h"

namespace {
using ark::es2panda::lsp::CONVERT_PARAMS_TO_OBJECT_ACTION;
using ark::es2panda::lsp::ConvertParamsToObjectRefactor;
using ark::es2panda::lsp::Initializer;

class LspConvertParamsToObjectTests : public LSPAPITests {};

TEST_F(LspConvertParamsToObjectTests, BasicFunctionWithThreeParameters)
{
    std::vector<std::string> files = {"convert_params_to_object_refactorParams.ets"};
    std::vector<std::string> texts = {R"(
function createUser(name: string, age: number, email: string): void {
    console.log(name, age, email);
}

function main(): void {
    createUser("John", 25, "john@example.com");
}
)"};

    auto filePaths = CreateTempFile(files, texts);
    constexpr size_t kExpectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), kExpectedFileCount);

    constexpr size_t kPositionInFunctionName = 22;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext textChangesContext {{}, formatContext, {}};

    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(CONVERT_PARAMS_TO_OBJECT_ACTION.kind);
    refactorContext.span.pos = kPositionInFunctionName;
    refactorContext.textChangesContext = &textChangesContext;

    auto result = GetApplicableRefactorsImpl(&refactorContext);
    constexpr size_t kExpectedRefactorCount = 1;
    ASSERT_EQ(kExpectedRefactorCount, result.size());
    ASSERT_EQ(std::string(CONVERT_PARAMS_TO_OBJECT_ACTION.name), result[0].action.name);

    ConvertParamsToObjectRefactor refactor;
    auto editsPtr = refactor.GetEditsForAction(refactorContext, std::string(CONVERT_PARAMS_TO_OBJECT_ACTION.name));
    ASSERT_NE(editsPtr, nullptr);
    const auto &edits = editsPtr->GetFileTextChanges();
    EXPECT_FALSE(edits.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LspConvertParamsToObjectTests, ClassMethodWithMultipleParameters)
{
    std::vector<std::string> files = {"convert_params_to_object_refactorMethod.ets"};
    std::vector<std::string> texts = {R"(
class UserService {
    createUser(name: string, age: number, email: string): void {
        console.log(name, age, email);
    }
}

function main(): void {
    let service = new UserService();
    service.createUser("Jane", 30, "jane@example.com");
}
)"};

    auto filePaths = CreateTempFile(files, texts);
    constexpr size_t kExpectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), kExpectedFileCount);

    constexpr size_t kPositionInMethodName = 35;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext textChangesContext {{}, formatContext, {}};

    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(CONVERT_PARAMS_TO_OBJECT_ACTION.kind);
    refactorContext.span.pos = kPositionInMethodName;
    refactorContext.textChangesContext = &textChangesContext;

    auto result = GetApplicableRefactorsImpl(&refactorContext);
    constexpr size_t kExpectedRefactorCount = 1;
    ASSERT_EQ(kExpectedRefactorCount, result.size());

    ConvertParamsToObjectRefactor refactor;
    auto editsPtr = refactor.GetEditsForAction(refactorContext, std::string(CONVERT_PARAMS_TO_OBJECT_ACTION.name));
    ASSERT_NE(editsPtr, nullptr);

    initializer.DestroyContext(ctx);
}

TEST_F(LspConvertParamsToObjectTests, FunctionWithOptionalParameters)
{
    std::vector<std::string> files = {"convert_params_to_object_refactorOptional.ets"};
    std::vector<std::string> texts = {R"(
function sendEmail(to: string, subject: string, body?: string): void {
    console.log(to, subject, body);
}

function main(): void {
    sendEmail("user@example.com", "Hello", "Test body");
}
)"};

    auto filePaths = CreateTempFile(files, texts);
    constexpr size_t kExpectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), kExpectedFileCount);

    constexpr size_t kPositionInFunctionName = 18;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext textChangesContext {{}, formatContext, {}};

    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(CONVERT_PARAMS_TO_OBJECT_ACTION.kind);
    refactorContext.span.pos = kPositionInFunctionName;
    refactorContext.textChangesContext = &textChangesContext;

    auto result = GetApplicableRefactorsImpl(&refactorContext);
    constexpr size_t kExpectedRefactorCount = 1;
    ASSERT_EQ(kExpectedRefactorCount, result.size());

    initializer.DestroyContext(ctx);
}

TEST_F(LspConvertParamsToObjectTests, NotAvailableForSingleParameter)
{
    std::vector<std::string> files = {"convert_params_to_object_refactorSingleParam.ets"};
    std::vector<std::string> texts = {R"(
function greet(name: string): void {
    console.log("Hello, " + name);
}
)"};

    auto filePaths = CreateTempFile(files, texts);
    constexpr size_t kExpectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), kExpectedFileCount);

    constexpr size_t kPositionInFunctionName = 18;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext textChangesContext {{}, formatContext, {}};

    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(CONVERT_PARAMS_TO_OBJECT_ACTION.kind);
    refactorContext.span.pos = kPositionInFunctionName;
    refactorContext.textChangesContext = &textChangesContext;

    auto result = GetApplicableRefactorsImpl(&refactorContext);
    constexpr size_t kExpectedRefactorCount = 1;
    ASSERT_EQ(kExpectedRefactorCount, result.size());

    initializer.DestroyContext(ctx);
}

TEST_F(LspConvertParamsToObjectTests, FunctionWithDefaultValues)
{
    std::vector<std::string> files = {"convert_params_to_object_refactorDefaults.ets"};
    std::vector<std::string> texts = {R"(
function createConfig(host: string, port: number = 3000, ssl: boolean = false): void {
    console.log(host, port, ssl);
}

function main(): void {
    createConfig("localhost", 8080, true);
}
)"};

    auto filePaths = CreateTempFile(files, texts);
    constexpr size_t kExpectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), kExpectedFileCount);

    constexpr size_t kPositionInFunctionName = 22;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext textChangesContext {{}, formatContext, {}};

    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(CONVERT_PARAMS_TO_OBJECT_ACTION.kind);
    refactorContext.span.pos = kPositionInFunctionName;
    refactorContext.textChangesContext = &textChangesContext;

    auto result = GetApplicableRefactorsImpl(&refactorContext);
    constexpr size_t kExpectedRefactorCount = 1;
    ASSERT_EQ(kExpectedRefactorCount, result.size());

    initializer.DestroyContext(ctx);
}

TEST_F(LspConvertParamsToObjectTests, StaticMethodWithMultipleParams)
{
    std::vector<std::string> files = {"convert_params_to_object_refactorStatic.ets"};
    std::vector<std::string> texts = {R"(
class MathUtils {
    static calculate(x: number, y: number, z: number): number {
        return x + y + z;
    }
}

function main(): void {
    let result = MathUtils.calculate(1, 2, 3);
}
)"};

    auto filePaths = CreateTempFile(files, texts);
    constexpr size_t kExpectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), kExpectedFileCount);

    constexpr size_t kPositionInStaticMethod = 38;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext textChangesContext {{}, formatContext, {}};

    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(CONVERT_PARAMS_TO_OBJECT_ACTION.kind);
    refactorContext.span.pos = kPositionInStaticMethod;
    refactorContext.textChangesContext = &textChangesContext;

    auto result = GetApplicableRefactorsImpl(&refactorContext);
    constexpr size_t kExpectedRefactorCount = 1;
    ASSERT_EQ(kExpectedRefactorCount, result.size());

    initializer.DestroyContext(ctx);
}

}  // namespace
