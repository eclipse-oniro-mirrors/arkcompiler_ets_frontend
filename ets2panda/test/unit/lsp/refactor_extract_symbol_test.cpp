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
#include <iostream>
#include <ostream>
#include <string>
#include "lsp/include/refactors/extract_symbol.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "public/public.h"

namespace {
using ark::es2panda::lsp::Initializer;

class LspExtrSymblTests : public LSPAPITests {};

TEST_F(LspExtrSymblTests, ExtrSymblToFunctionTests)
{
    std::vector<std::string> files = {"ExtractSymbol1.ets"};
    std::vector<std::string> texts = {R"(function main() {
    let x = 10;
    let y = 20;
    console.log("x + y = " + (x + y));
})"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    const size_t spanStart = 66;
    const size_t spanEnd = 87;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION.name;
    refactorContext.span = {spanStart, spanEnd};
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ExtractSymbolRefactor().GetEditsForAction(
        refactorContext, std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION.name));
    std::string_view res = actions->GetFileTextChanges().at(0).textChanges.at(0).newText;
    std::string_view expect = "const EXTRACTED_VAL = \"x + y = \" + (x + y);";
    EXPECT_EQ(res, expect);
    initializer.DestroyContext(ctx);
}

TEST_F(LspExtrSymblTests, ExtrSymblConstValTests)
{
    std::vector<std::string> files = {"ExtractSymbol2.ets"};
    std::vector<std::string> texts = {
        R"(let msg = "Hello, " + user.name + "! Today is " + new Date().toDateString();)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    const size_t spanStart = 10;
    const size_t spanEnd = 76;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION.name;
    refactorContext.span = {spanStart, spanEnd};
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ExtractSymbolRefactor().GetEditsForAction(
        refactorContext, std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION.name));
    std::string_view res = actions->GetFileTextChanges().at(0).textChanges.at(0).newText;
    std::string_view expect =
        R"(const EXTRACTED_VAL = "Hello, " + user.name + "! Today is " + new Date().toDateString();)";
    EXPECT_EQ(res, expect);
    initializer.DestroyContext(ctx);
}

TEST_F(LspExtrSymblTests, ExtrSymblConstVal1Tests)
{
    std::vector<std::string> files = {"ExtractSymbol2.ets"};
    std::vector<std::string> texts = {
        R"(let msg = "Hello, " + user.name + "! Today is " + new Date().toDateString();)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    const size_t spanStart = 22;
    const size_t spanEnd = 32;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION.name;
    refactorContext.span = {spanStart, spanEnd};
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ExtractSymbolRefactor().GetEditsForAction(
        refactorContext, std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION.name));
    std::string_view res = actions->GetFileTextChanges().at(0).textChanges.at(0).newText;
    std::string_view expect = "const EXTRACTED_VAL = user.name;";
    EXPECT_EQ(res, expect);
    initializer.DestroyContext(ctx);
}

TEST_F(LspExtrSymblTests, ExtrSymblConstVal2Tests)
{
    std::vector<std::string> files = {"ExtractSymbol2.ets"};
    std::vector<std::string> texts = {
        R"(let msg = "Hello, " + user.name + "! Today is " + new Date().toDateString();)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    const size_t spanStart = 50;
    const size_t spanEnd = 70;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION.name;
    refactorContext.span = {spanStart, spanEnd};
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ExtractSymbolRefactor().GetEditsForAction(
        refactorContext, std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION.name));
    std::string_view res = actions->GetFileTextChanges().at(0).textChanges.at(0).newText;
    std::string_view expect = "const EXTRACTED_VAL = new Date().toDateString();";
    EXPECT_EQ(res, expect);
    initializer.DestroyContext(ctx);
}

TEST_F(LspExtrSymblTests, ExtrSymblLetVal2Tests)
{
    std::vector<std::string> files = {"ExtractSymbolLet2.ets"};
    std::vector<std::string> texts = {
        R"(let msg = "Hello, " + user.name + "! Today is " + new Date().toDateString();)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    const size_t spanStart = 50;
    const size_t spanEnd = 70;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION.name;
    refactorContext.span = {spanStart, spanEnd};
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ExtractSymbolRefactor().GetEditsForAction(
        refactorContext, std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION.name));
    std::string_view res = actions->GetFileTextChanges().at(0).textChanges.at(0).newText;
    std::string_view expect = "let EXTRACTED_VAL = new Date().toDateString();";
    EXPECT_EQ(res, expect);
    initializer.DestroyContext(ctx);
}

TEST_F(LspExtrSymblTests, ExtrSymblLetVal1Tests)
{
    std::vector<std::string> files = {"ExtractSymbolVal2.ets"};
    std::vector<std::string> texts = {
        R"(let msg = "Hello, " + user.name + "! Today is " + new Date().toDateString();)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    const size_t spanStart = 22;
    const size_t spanEnd = 32;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION.name;
    refactorContext.span = {spanStart, spanEnd};
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ExtractSymbolRefactor().GetEditsForAction(
        refactorContext, std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION.name));
    std::string_view res = actions->GetFileTextChanges().at(0).textChanges.at(0).newText;
    std::string_view expect = "let EXTRACTED_VAL = user.name;";
    EXPECT_EQ(res, expect);
    initializer.DestroyContext(ctx);
}

TEST_F(LspExtrSymblTests, ExtrSymblFunctionTests)
{
    std::vector<std::string> files = {"ExtrSymblFunctionTests.ets"};
    std::vector<std::string> texts = {
        R"(function greet(name : string) {
    console.log("Hello, " + name + "!");
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    const size_t spanStart = 48;
    const size_t spanEnd = 62;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION.name;
    refactorContext.span = {spanStart, spanEnd};
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ExtractSymbolRefactor().GetEditsForAction(
        refactorContext, std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION.name));
    std::string_view res = actions->GetFileTextChanges().at(0).textChanges.at(0).newText;
    std::string_view expect = "function extractedFunction1(name : string) {\n    return \"Hello, \" + name;\n}\n\n";
    EXPECT_EQ(res, expect);

    initializer.DestroyContext(ctx);
}

TEST_F(LspExtrSymblTests, ExtrSymblFunctionTests1)
{
    std::vector<std::string> files = {"ExtrSymblFunctionTests1.ets"};
    std::vector<std::string> texts = {
        R"(function greet1(name : string, surname : string) {
    console.log("name is, " + name + "!" + surname);
}
        
        function greet(name : string, surname : string) {
    console.log("Hello, " + name + "!" + surname);
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    const size_t spanStart = 190;
    const size_t spanEnd = 219;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION.name;
    refactorContext.span = {spanStart, spanEnd};
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ExtractSymbolRefactor().GetEditsForAction(
        refactorContext, std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION.name));
    std::string_view res = actions->GetFileTextChanges().at(0).textChanges.at(0).newText;
    std::string_view expect =
        "function extractedFunction2(name : string, surname : string) {\n    return Hello, \" + name + \"!\" + "
        "surname;\n}\n\n";
    EXPECT_EQ(res, expect);

    initializer.DestroyContext(ctx);
}

}  // namespace