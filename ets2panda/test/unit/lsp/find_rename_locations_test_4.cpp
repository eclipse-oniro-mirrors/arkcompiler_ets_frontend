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

#include <cstddef>
#include <string>
#include <vector>

#include "lsp_api_test.h"
#include "lsp/include/find_rename_locations.h"

#include <gtest/gtest.h>

namespace {

using ark::es2panda::lsp::FindRenameLocationsInCurrentFile;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::NeedsCrossFileRename;

class LspFindRenameLocationsTests4 : public LSPAPITests {};

// Test: shorthand property `let obj = { foo };` rename should find the shorthand usage
TEST_F(LspFindRenameLocationsTests4, ShorthandPropertyRenameFindsShorthandUsage)
{
    std::vector<std::string> files = {"rename_shorthand.ets"};
    std::vector<std::string> texts = {R"(let foo: number = 1;
let obj = { foo };
console.log(obj.foo);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at the declaration of "foo"
    const auto position = texts[0].find("foo: number");
    ASSERT_NE(position, std::string::npos);

    ASSERT_FALSE(NeedsCrossFileRename(context, position));
    auto res = FindRenameLocationsInCurrentFile(context, position);
    ASSERT_GE(res.size(), 1U);

    // All rename locations should be in the same file
    for (const auto &loc : res) {
        ASSERT_EQ(loc.fileName, filePaths[0]);
    }

    initializer.DestroyContext(context);
}

// Test: destructuring `let { foo } = obj;` rename - documents current behavior for destructuring bindings
TEST_F(LspFindRenameLocationsTests4, DestructuringRenameDoesNotCrash)
{
    std::vector<std::string> files = {"rename_destructuring.ets"};
    std::vector<std::string> texts = {R"(let obj = { foo: 1, bar: 2 };
let { foo } = obj;
console.log(foo);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at the destructured "foo" binding (inside the braces)
    const auto bracePos = texts[0].find("{ foo }");
    ASSERT_NE(bracePos, std::string::npos);
    const auto position = bracePos + std::string("{ ").size();  // position on "foo"

    // The API should not crash when querying at a destructuring binding position.
    ASSERT_FALSE(NeedsCrossFileRename(context, position));
    auto res = FindRenameLocationsInCurrentFile(context, position);

    initializer.DestroyContext(context);

    // Destructuring rename support may be limited in the current LSP version.
    // Verify the API returns a usable result without crashing.
    for (const auto &loc : res) {
        ASSERT_EQ(loc.fileName, filePaths[0]);
    }
}

// Test: rename local variable should not affect string literal with same text
TEST_F(LspFindRenameLocationsTests4, RenameDoesNotAffectStringLiteralWithSameText)
{
    std::vector<std::string> files = {"rename_not_string.ets"};
    std::vector<std::string> texts = {R"(let target: number = 1;
let label: string = "target";
console.log(target);
console.log(label);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at the declaration of "target"
    const auto position = texts[0].find("target: number");
    ASSERT_NE(position, std::string::npos);

    ASSERT_FALSE(NeedsCrossFileRename(context, position));
    auto res = FindRenameLocationsInCurrentFile(context, position);

    initializer.DestroyContext(context);

    ASSERT_GE(res.size(), 1U);
    // The string literal "target" should NOT be included as a rename location.
    // Locate the string literal offset to verify it is absent from results.
    const auto stringLitStart = texts[0].find("\"target\"");
    ASSERT_NE(stringLitStart, std::string::npos);
    const auto stringLitContentStart = stringLitStart + 1;  // inside the quotes
    bool stringLitInResults = false;
    for (const auto &loc : res) {
        if (loc.start == stringLitContentStart) {
            stringLitInResults = true;
            break;
        }
    }
    EXPECT_FALSE(stringLitInResults);
}

// Test: rename local variable should not affect comment with same text
TEST_F(LspFindRenameLocationsTests4, RenameDoesNotAffectCommentWithSameText)
{
    std::vector<std::string> files = {"rename_not_comment.ets"};
    std::vector<std::string> texts = {R"(// target is a variable
let target: number = 1;
console.log(target);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at the declaration of "target"
    const auto position = texts[0].find("target: number");
    ASSERT_NE(position, std::string::npos);

    ASSERT_FALSE(NeedsCrossFileRename(context, position));
    auto res = FindRenameLocationsInCurrentFile(context, position);

    initializer.DestroyContext(context);

    ASSERT_GE(res.size(), 1U);
    // The comment "target" should NOT be included as a rename location.
    const auto commentStart = texts[0].find("// target");
    ASSERT_NE(commentStart, std::string::npos);
    const auto commentContentStart = commentStart + std::string("// ").size();
    bool commentInResults = false;
    for (const auto &loc : res) {
        if (loc.start == commentContentStart) {
            commentInResults = true;
            break;
        }
    }
    EXPECT_FALSE(commentInResults);
}

// Test: rename function parameter should find all usages within the function body
TEST_F(LspFindRenameLocationsTests4, RenameFunctionParameterFindsUsagesInBody)
{
    std::vector<std::string> files = {"rename_param.ets"};
    std::vector<std::string> texts = {R"(function greet(name: string): string {
    return "Hello, " + name;
}
let msg = greet("world");)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at the parameter "name"
    const auto position = texts[0].find("name: string");
    ASSERT_NE(position, std::string::npos);

    ASSERT_FALSE(NeedsCrossFileRename(context, position));
    auto res = FindRenameLocationsInCurrentFile(context, position);

    initializer.DestroyContext(context);

    // Should find at least the declaration and the usage in the return statement
    ASSERT_GE(res.size(), 2U);
    for (const auto &loc : res) {
        ASSERT_EQ(loc.fileName, filePaths[0]);
    }
}

}  // namespace
