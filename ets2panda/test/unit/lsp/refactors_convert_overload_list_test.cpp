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
#include "lsp/include/refactors/convert_overload_list.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/services/text_change/text_change_context.h"

namespace {
using ark::es2panda::lsp::CONVERT_OVERLOAD_LIST_ACTION;
using ark::es2panda::lsp::ConvertOverloadListRefactor;
using ark::es2panda::lsp::Initializer;

class LspConvertOverloadListTests : public LSPAPITests {};

TEST_F(LspConvertOverloadListTests, BasicFunctionOverloads)
{
    std::vector<std::string> files = {"refactorBasicFunction.ets"};
    std::vector<std::string> texts = {R"(
 export function formatInput(input: string): string;
 export function formatInput(input: number): number;
 export function formatInput(input: string | number): string | number {
     return input;
 }
 )"};

    auto filePaths = CreateTempFile(files, texts);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kPositionInFunctionName = 22;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext textChangesContext {{}, formatContext, {}};

    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(CONVERT_OVERLOAD_LIST_ACTION.kind);
    refactorContext.span.pos = kPositionInFunctionName;
    refactorContext.textChangesContext = &textChangesContext;

    auto result = GetApplicableRefactorsImpl(&refactorContext);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kExpectedRefactorCount = 1;
    ASSERT_EQ(kExpectedRefactorCount, result.size());
    ASSERT_EQ(std::string(CONVERT_OVERLOAD_LIST_ACTION.name), result[0].action.name);

    ConvertOverloadListRefactor refactor;
    auto editsPtr = refactor.GetEditsForAction(refactorContext, std::string(CONVERT_OVERLOAD_LIST_ACTION.name));
    ASSERT_NE(editsPtr, nullptr);
    const auto &edits = editsPtr->GetFileTextChanges();
    EXPECT_FALSE(edits.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LspConvertOverloadListTests, ClassMethodOverloads)
{
    std::vector<std::string> files = {"refactorClassMethod.ets"};
    std::vector<std::string> texts = {R"(
 class Calculator {
     add(x: number): number {
         return x * 2;
     }
     
     add(x: number, y: number): number {
         return x + y;
     }
     
     add(x: number, y: number, z: number): number {
         return x + y + z;
     }
 }
 )"};

    auto filePaths = CreateTempFile(files, texts);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kExpectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), kExpectedFileCount);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kPositionInMethodName = 26;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(CONVERT_OVERLOAD_LIST_ACTION.kind);
    refactorContext.span.pos = kPositionInMethodName;

    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);

    // NOLINTNEXTLINE(readability-identifier-naming
    constexpr size_t kExpectedRefactorCount = 1;
    ASSERT_EQ(kExpectedRefactorCount, result.size());
    ASSERT_EQ(std::string(CONVERT_OVERLOAD_LIST_ACTION.name), result[0].action.name);
}

TEST_F(LspConvertOverloadListTests, OptionalParameterOverloads)
{
    std::vector<std::string> files = {"refactorOptionalParams.ets"};
    std::vector<std::string> texts = {R"(
 function format(value: string): string {
     return value.toUpperCase();
 }
 
 function format(value: string, style: string): string {
     if (style === "upper") {
         return value.toUpperCase();
     }
     return value.toLowerCase();
 }
 
 function format(value: string, style: string, prefix: string): string {
     let result = style === "upper" ? value.toUpperCase() : value.toLowerCase();
     return prefix + result;
 }
 )"};

    auto filePaths = CreateTempFile(files, texts);
    // NOLINTNEXTLINE(readability-identifier-naming
    constexpr size_t kExpectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), kExpectedFileCount);

    // NOLINTNEXTLINE(readability-identifier-naming
    constexpr size_t kPositionInFunctionName = 12;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(CONVERT_OVERLOAD_LIST_ACTION.kind);
    refactorContext.span.pos = kPositionInFunctionName;

    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);

    // NOLINTNEXTLINE(readability-identifier-naming
    constexpr size_t kExpectedRefactorCount = 1;
    ASSERT_EQ(kExpectedRefactorCount, result.size());
    ASSERT_EQ(std::string(CONVERT_OVERLOAD_LIST_ACTION.name), result[0].action.name);
}

TEST_F(LspConvertOverloadListTests, InterfaceMethodOverloads)
{
    std::vector<std::string> files = {"refactorInterfaceMethod.ets"};
    std::vector<std::string> texts = {R"(
 interface IDataService {
     fetch(id: string): Promise<Data>;
     fetch(id: number): Promise<Data>;
     fetch(filter: FilterOptions): Promise<Data>;
 }
 
 class Data {
     value: string = "";
 }
 
 interface FilterOptions {
     type: string;
     limit: number;
 }
 )"};

    auto filePaths = CreateTempFile(files, texts);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kExpectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), kExpectedFileCount);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kPositionInInterfaceMethod = 60;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(CONVERT_OVERLOAD_LIST_ACTION.kind);
    refactorContext.span.pos = kPositionInInterfaceMethod;

    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kExpectedRefactorCount = 2;
    ASSERT_EQ(kExpectedRefactorCount, result.size());
    EXPECT_FALSE(result.empty());
    if (!result.empty()) {
        ASSERT_EQ(std::string(CONVERT_OVERLOAD_LIST_ACTION.name), result[0].action.name);
    }
}

TEST_F(LspConvertOverloadListTests, NoOverloadsAvailable)
{
    std::vector<std::string> files = {"refactorNoOverloads.ets"};
    std::vector<std::string> texts = {R"(
 function singleFunction(x: number): number {
     return x * 2;
 }
 )"};

    auto filePaths = CreateTempFile(files, texts);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kExpectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), kExpectedFileCount);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kPositionInFunctionName = 30;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(CONVERT_OVERLOAD_LIST_ACTION.kind);
    refactorContext.span.pos = kPositionInFunctionName;

    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kExpectedRefactorCount = 1;
    ASSERT_EQ(kExpectedRefactorCount, result.size());
}

TEST_F(LspConvertOverloadListTests, InvalidRefactorKind)
{
    std::vector<std::string> files = {"refactorInvalidKind.ets"};
    std::vector<std::string> texts = {R"(
 function test(a: string): string {
     return a;
 }
 
 function test(a: number): number {
     return a;
 }
 )"};

    auto filePaths = CreateTempFile(files, texts);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kExpectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), kExpectedFileCount);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kPositionInFunctionName = 20;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = "invalid.refactor.kind";
    refactorContext.span.pos = kPositionInFunctionName;

    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kExpectedRefactorCount = 1;
    ASSERT_EQ(kExpectedRefactorCount, result.size());
}

TEST_F(LspConvertOverloadListTests, AbstractClassMethodOverloads)
{
    std::vector<std::string> files = {"refactorAbstractClass.ets"};
    std::vector<std::string> texts = {R"(
 abstract class BaseProcessor {
     abstract process(data: string): string;
     abstract process(data: number): number;
     
     helper(): string {
         return "helper";
     }
 }
 
 class ConcreteProcessor extends BaseProcessor {
     process(data: string): string {
         return data.toUpperCase();
     }
     
     process(data: number): number {
         return data * 2;
     }
 }
 )"};

    auto filePaths = CreateTempFile(files, texts);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kExpectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), kExpectedFileCount);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kPositionInAbstractMethod = 48;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(CONVERT_OVERLOAD_LIST_ACTION.kind);
    refactorContext.span.pos = kPositionInAbstractMethod;

    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kExpectedRefactorCount = 1;
    ASSERT_EQ(kExpectedRefactorCount, result.size());
    ASSERT_EQ(std::string(CONVERT_OVERLOAD_LIST_ACTION.name), result[0].action.name);
}

TEST_F(LspConvertOverloadListTests, StaticMethodOverloads)
{
    std::vector<std::string> files = {"refactorStaticMethod.ets"};
    std::vector<std::string> texts = {R"(
 class MathUtils {
     static calculate(x: number): number {
         return x * x;
     }
     
     static calculate(x: number, y: number): number {
         return x + y;
     }
     
     static calculate(x: number, y: number, z: number): number {
         return x + y + z;
     }
 }
 )"};

    // NOLINTNEXTLINE(readability-identifier-naming)
    auto filePaths = CreateTempFile(files, texts);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kExpectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), kExpectedFileCount);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kPositionInStaticMethod = 33;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(CONVERT_OVERLOAD_LIST_ACTION.kind);
    refactorContext.span.pos = kPositionInStaticMethod;

    auto result = GetApplicableRefactorsImpl(&refactorContext);
    initializer.DestroyContext(ctx);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kExpectedRefactorCount = 1;
    ASSERT_EQ(kExpectedRefactorCount, result.size());
    ASSERT_EQ(std::string(CONVERT_OVERLOAD_LIST_ACTION.name), result[0].action.name);
}

}  // namespace