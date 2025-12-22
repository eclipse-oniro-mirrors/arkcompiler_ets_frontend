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

#include "lsp/include/services/text_change/text_change_context.h"
#include "lsp_api_test.h"
#include "lsp/include/refactors/infer_function_return_type.h"

#include <gtest/gtest.h>
#include <cstddef>
#include <iostream>
#include <string>
#include "ir/astNode.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

namespace {

class LSPInferFunctionReturnType : public LSPAPITests {
public:
    static constexpr std::string_view INFER_FUNCTION_RETURN_TYPE = "refactor.rewrite.function.returnType";
};

TEST_F(LSPInferFunctionReturnType, InferBooleanReturnType)
{
    std::vector<std::string> files = {"infer_bool.ets"};
    std::vector<std::string> texts = {R"(function example1() { return true; })"};

    auto tempFiles = CreateTempFile(files, texts);
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};

    ark::es2panda::lsp::RefactorContext refContext;
    refContext.context = ctx;
    refContext.textChangesContext = &changeText;
    const size_t startPos = 12;
    const size_t endPos = 20;
    refContext.span = {startPos, endPos};
    refContext.kind = std::string(LSPInferFunctionReturnType::INFER_FUNCTION_RETURN_TYPE);

    auto info = ark::es2panda::lsp::GetInfoInferRet(refContext);
    ASSERT_NE(info.declaration, nullptr);
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    ark::es2panda::lsp::DoChanges(tracker, ctx, info.declaration, info.returnTypeNode);
    auto changes = tracker.GetChanges();
    ASSERT_EQ(changes.size(), 1);
    ASSERT_EQ(changes.at(0).textChanges.at(0).newText, " : boolean");

    initializer.DestroyContext(ctx);
}

TEST_F(LSPInferFunctionReturnType, InferNumberReturnType)
{
    std::vector<std::string> files = {"infer_number.ets"};
    std::vector<std::string> texts = {R"(const multiply = (a: number, b: number) => { return a * b; })"};

    auto tempFiles = CreateTempFile(files, texts);
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};

    ark::es2panda::lsp::RefactorContext refContext;
    refContext.context = ctx;
    refContext.textChangesContext = &changeText;
    const size_t startPos = 42;
    const size_t endPos = 56;
    refContext.span = {startPos, endPos};
    refContext.kind = std::string(LSPInferFunctionReturnType::INFER_FUNCTION_RETURN_TYPE);

    auto info = ark::es2panda::lsp::GetInfoInferRet(refContext);
    ASSERT_NE(info.declaration, nullptr);
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    ark::es2panda::lsp::DoChanges(tracker, ctx, info.declaration, info.returnTypeNode);
    auto changes = tracker.GetChanges();
    ASSERT_EQ(changes.size(), 1);
    ASSERT_EQ(changes.at(0).textChanges.at(0).newText, " : number");

    initializer.DestroyContext(ctx);
}

TEST_F(LSPInferFunctionReturnType, InferStringReturnType)
{
    std::vector<std::string> files = {"infer_string.ets"};
    std::vector<std::string> texts = {R"(function alreadyTyped() { return "hello"; })"};

    auto tempFiles = CreateTempFile(files, texts);
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};

    ark::es2panda::lsp::RefactorContext refContext;
    refContext.context = ctx;
    refContext.textChangesContext = &changeText;
    const size_t startPos = 12;
    const size_t endPos = 20;
    refContext.span = {startPos, endPos};
    refContext.kind = std::string(LSPInferFunctionReturnType::INFER_FUNCTION_RETURN_TYPE);

    auto info = ark::es2panda::lsp::GetInfoInferRet(refContext);
    ASSERT_NE(info.declaration, nullptr);
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    ark::es2panda::lsp::DoChanges(tracker, ctx, info.declaration, info.returnTypeNode);
    auto changes = tracker.GetChanges();
    ASSERT_EQ(changes.size(), 1);
    ASSERT_EQ(changes.at(0).textChanges.at(0).newText, " : string");

    initializer.DestroyContext(ctx);
}

TEST_F(LSPInferFunctionReturnType, ConvertFunctionArrayTest)
{
    std::vector<std::string> files = {"inferArray.ets"};
    std::vector<std::string> texts = {
        R"(function createNumberArray(count: number) {
    const result: number[] = [];
    for (let i = 0; i < count; i++) {
        result.push(i);
    }
    return result;
})"};

    auto tempFiles = CreateTempFile(files, texts);
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};

    ark::es2panda::lsp::RefactorContext refContext;
    refContext.context = ctx;
    refContext.textChangesContext = &changeText;
    const size_t startPos = 10;
    const size_t endPos = 30;
    refContext.span = {startPos, endPos};
    refContext.kind = std::string(LSPInferFunctionReturnType::INFER_FUNCTION_RETURN_TYPE);

    auto info = ark::es2panda::lsp::GetInfoInferRet(refContext);

    ASSERT_NE(info.declaration, nullptr);
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    ark::es2panda::lsp::DoChanges(tracker, ctx, info.declaration, info.returnTypeNode);
    auto changes = tracker.GetChanges();
    ASSERT_EQ(changes.at(0).textChanges.at(0).newText, " : number[]");
    ASSERT_EQ(changes.size(), 1);

    initializer.DestroyContext(ctx);
}

}  // namespace