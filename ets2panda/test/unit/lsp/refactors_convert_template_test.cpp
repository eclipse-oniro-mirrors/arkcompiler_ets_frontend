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
#include <string_view>
#include "lsp/include/services/text_change/change_tracker.h"
#include "lsp_api_test.h"
#include "lsp/include/applicable_refactors.h"
#include "lsp/include/refactors/convert_template.h"
#include "public/public.h"

const size_t REFACTOR_TEMPLATE_POSITION_OFFSET = 8;

namespace {
using ark::es2panda::lsp::Initializer;

class LspTemplateRefTests : public LSPAPITests {
public:
    static constexpr std::string_view TO_NAMED_TEMPLATE_KIND = "refactor.rewrite.string";
    static constexpr std::string_view TO_NAMED_TEMPLATE_NAME = "Convert to template string";
};

TEST_F(LspTemplateRefTests, ConvertTemplateRefactor1)
{
    std::vector<std::string> files = {"ConvertTemplateRefactor1.ets"};
    std::vector<std::string> texts = {R"(let a = /*1*/"\\0\\b\\f\\t\\r\\n" +text + "\\n"/*2*/;)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    auto pos = texts[0].find("/*1*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = std::string(TO_NAMED_TEMPLATE_KIND);
    refactorContext.span.pos = pos + REFACTOR_TEMPLATE_POSITION_OFFSET;
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ConvertTemplateRefactor().GetEditsForAction(refactorContext,
                                                                                   std::string(TO_NAMED_TEMPLATE_NAME));
    initializer.DestroyContext(ctx);

    EXPECT_EQ(actions->GetFileTextChanges()[0].textChanges[0].newText, R"(`\\0\\b\\f\\t\\r\\n${text}\\n`;)");
    ASSERT_EQ(1, result.size());
    ASSERT_EQ(std::string(TO_NAMED_TEMPLATE_NAME), result[0].action.name);
}

TEST_F(LspTemplateRefTests, ConvertTemplateRefactor2)
{
    std::vector<std::string> files = {"ConvertTemplateRefactor2.ets"};
    std::vector<std::string> texts = {R"(let b = /*3*/"" + text + ""/*4*/;)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    auto pos = texts[0].find("/*3*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = std::string(TO_NAMED_TEMPLATE_KIND);
    refactorContext.span.pos = pos + REFACTOR_TEMPLATE_POSITION_OFFSET;
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ConvertTemplateRefactor().GetEditsForAction(refactorContext,
                                                                                   std::string(TO_NAMED_TEMPLATE_NAME));
    initializer.DestroyContext(ctx);
    EXPECT_EQ(actions->GetFileTextChanges()[0].textChanges[0].newText, R"(`${text}`;)");
    ASSERT_EQ(1, result.size());
    ASSERT_EQ(std::string(TO_NAMED_TEMPLATE_NAME), result[0].action.name);
}

TEST_F(LspTemplateRefTests, ConvertTemplateRefactor3)
{
    std::vector<std::string> files = {"ConvertTemplateRefactor3.ets"};
    std::vector<std::string> texts = {R"(let c = /*5*/'\$' + text + "\\\\/*6*/";)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    auto pos = texts[0].find("/*5*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = std::string(TO_NAMED_TEMPLATE_KIND);
    refactorContext.span.pos = pos + REFACTOR_TEMPLATE_POSITION_OFFSET;
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ConvertTemplateRefactor().GetEditsForAction(refactorContext,
                                                                                   std::string(TO_NAMED_TEMPLATE_NAME));
    initializer.DestroyContext(ctx);
    EXPECT_EQ(actions->GetFileTextChanges()[0].textChanges[0].newText, R"(`\$${text}\\\\/*6*/`;)");
    ASSERT_EQ(1, result.size());
    ASSERT_EQ(std::string(TO_NAMED_TEMPLATE_NAME), result[0].action.name);
}

TEST_F(LspTemplateRefTests, ConvertTemplateRefactor4)
{
    std::vector<std::string> files = {"ConvertTemplateRefactor4.ets"};
    std::vector<std::string> texts = {R"(let d = /*7*/\'\$\' + text + \'\\\\\'/*8*/;)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    auto pos = texts[0].find("/*7*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = std::string(TO_NAMED_TEMPLATE_KIND);
    refactorContext.span.pos = pos + REFACTOR_TEMPLATE_POSITION_OFFSET;
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ConvertTemplateRefactor().GetEditsForAction(refactorContext,
                                                                                   std::string(TO_NAMED_TEMPLATE_NAME));

    initializer.DestroyContext(ctx);
    ASSERT_EQ(0, result.size());
}

TEST_F(LspTemplateRefTests, ConvertTemplateRefactor5)
{
    std::vector<std::string> files = {"ConvertTemplateRefactor5.ets"};
    std::vector<std::string> texts = {R"(let e = /*9*/'\$\{' + text + "\}"/*10*/;)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    auto pos = texts[0].find("/*9*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = std::string(TO_NAMED_TEMPLATE_KIND);
    refactorContext.span.pos = pos + REFACTOR_TEMPLATE_POSITION_OFFSET;
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ConvertTemplateRefactor().GetEditsForAction(refactorContext,
                                                                                   std::string(TO_NAMED_TEMPLATE_NAME));

    initializer.DestroyContext(ctx);
    EXPECT_EQ(actions->GetFileTextChanges()[0].textChanges[0].newText, R"(`\${${text}}`;)");
    ASSERT_EQ(1, result.size());
    ASSERT_EQ(std::string(TO_NAMED_TEMPLATE_NAME), result[0].action.name);
}

TEST_F(LspTemplateRefTests, ConvertTemplateRefactor6)
{
    std::vector<std::string> files = {"ConvertTemplateRefactor6.ets"};
    std::vector<std::string> texts = {R"(let f = /*11*/ \'\\\$\{\' + text + \'\}\'/*12*/;)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    auto pos = texts[0].find("/*11*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = std::string(TO_NAMED_TEMPLATE_KIND);
    refactorContext.span.pos = pos + REFACTOR_TEMPLATE_POSITION_OFFSET;
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ConvertTemplateRefactor().GetEditsForAction(refactorContext,
                                                                                   std::string(TO_NAMED_TEMPLATE_NAME));
    initializer.DestroyContext(ctx);
    ASSERT_EQ(0, result.size());
}

TEST_F(LspTemplateRefTests, ConvertTemplateRefactor7)
{
    std::vector<std::string> files = {"ConvertTemplateRefactor7.ets"};
    std::vector<std::string> texts = {R"(let g = /*13*/'\\\\$ + text + "\\\\"/*14*/;)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    auto pos = texts[0].find("/*13*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = std::string(TO_NAMED_TEMPLATE_KIND);
    refactorContext.span.pos = pos + REFACTOR_TEMPLATE_POSITION_OFFSET;
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ConvertTemplateRefactor().GetEditsForAction(refactorContext,
                                                                                   std::string(TO_NAMED_TEMPLATE_NAME));
    initializer.DestroyContext(ctx);
    ASSERT_EQ(0, result.size());
}

TEST_F(LspTemplateRefTests, ConvertTemplateRefactor8)
{
    std::vector<std::string> files = {"ConvertTemplateRefactor8.ets"};
    std::vector<std::string> texts = {R"(let h = /*15*/"\\u0041\\u0061" + text + "\\0\\u0000"/*16*/;)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    auto pos = texts[0].find("/*15*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = std::string(TO_NAMED_TEMPLATE_KIND);
    refactorContext.span.pos = pos + REFACTOR_TEMPLATE_POSITION_OFFSET;
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ConvertTemplateRefactor().GetEditsForAction(refactorContext,
                                                                                   std::string(TO_NAMED_TEMPLATE_NAME));
    initializer.DestroyContext(ctx);
    EXPECT_EQ(actions->GetFileTextChanges()[0].textChanges[0].newText, R"(`\\u0041\\u0061${text}\\0\\u0000`;)");
    ASSERT_EQ(1, result.size());
    ASSERT_EQ(std::string(TO_NAMED_TEMPLATE_NAME), result[0].action.name);
}

TEST_F(LspTemplateRefTests, ConvertTemplateRefactor9)
{
    std::vector<std::string> files = {"ConvertTemplateRefactor9.ets"};
    std::vector<std::string> texts = {R"(let i = /*17*/'\$\`' + text + "\`\\\\"/*18*/;)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    auto pos = texts[0].find("/*17*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = std::string(TO_NAMED_TEMPLATE_KIND);
    refactorContext.span.pos = pos + REFACTOR_TEMPLATE_POSITION_OFFSET;
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::ConvertTemplateRefactor().GetEditsForAction(refactorContext,
                                                                                   std::string(TO_NAMED_TEMPLATE_NAME));
    initializer.DestroyContext(ctx);
    EXPECT_EQ(actions->GetFileTextChanges()[0].textChanges[0].newText, R"(`\$\`${text}\`\\\\`;)");
    ASSERT_EQ(1, result.size());
    ASSERT_EQ(std::string(TO_NAMED_TEMPLATE_NAME), result[0].action.name);
}

}  // namespace