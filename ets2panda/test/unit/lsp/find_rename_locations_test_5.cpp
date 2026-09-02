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
#include "lsp/include/cancellation_token.h"
#include "lsp/include/find_rename_locations.h"

#include <gtest/gtest.h>

namespace {

using ark::es2panda::lsp::CancellationToken;
using ark::es2panda::lsp::FindRenameLocations;
using ark::es2panda::lsp::FindRenameLocationsInCurrentFile;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::RenameLocation;

constexpr time_t DEFAULT_THROTTLE = 0;

class LspFindRenameLocationsTests5 : public LSPAPITests {
public:
    static std::vector<RenameLocation> ToSortedVector(const std::set<RenameLocation> &locations)
    {
        return {locations.begin(), locations.end()};
    }

    static void ExpectLocation(const RenameLocation &actual, const std::string &fileName, size_t start, size_t end)
    {
        EXPECT_EQ(actual.fileName, fileName);
        EXPECT_EQ(actual.start, start);
        EXPECT_EQ(actual.end, end);
    }

    class CancelledHost : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return true;
        }
    };

    class NotCancelledHost : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return false;
        }
    };
};

// Plan: local variable rename - all occurrences in scope are collected exactly.
TEST_F(LspFindRenameLocationsTests5, LocalVariableRenameLocations)
{
    const std::string source = "let box = new Box();\nlet unpack: Box = box;\nbox.value;";
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext("rename5_local_var.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    const auto position = source.find("box =");
    ASSERT_NE(position, std::string::npos);
    auto res = FindRenameLocationsInCurrentFile(context, position);
    initializer.DestroyContext(context);

    auto actual = ToSortedVector(res);
    size_t const expectedCount = 3;
    ASSERT_EQ(actual.size(), expectedCount);
    size_t const declStart = 4;
    size_t const useInUnpack = 39;
    size_t const memberAccess = 44;
    size_t const boxEnd = 7;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t thirdLocation = 2;
    ExpectLocation(actual[0], "rename5_local_var.ets", declStart, boxEnd);
    ExpectLocation(actual[1], "rename5_local_var.ets", useInUnpack, useInUnpack + std::string("box").size());
    ExpectLocation(actual[thirdLocation], "rename5_local_var.ets", memberAccess,
                   memberAccess + std::string("box").size());
}

// Plan: function rename - declaration and every call site are collected.
TEST_F(LspFindRenameLocationsTests5, FunctionRenameLocations)
{
    const std::string source = "function score(): number {\n  return 1;\n}\nscore();\nlet total = score();";
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext("rename5_function.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    const auto position = source.find("score(): number");
    ASSERT_NE(position, std::string::npos);
    auto res = FindRenameLocationsInCurrentFile(context, position);
    initializer.DestroyContext(context);

    auto actual = ToSortedVector(res);
    size_t const expectedCount = 3;
    ASSERT_EQ(actual.size(), expectedCount);
    size_t const declStart = 9;
    size_t const firstCall = 41;
    size_t const secondCall = 62;
    size_t const scoreLen = std::string("score").size();
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t thirdLocation = 2;
    ExpectLocation(actual[0], "rename5_function.ets", declStart, declStart + scoreLen);
    ExpectLocation(actual[1], "rename5_function.ets", firstCall, firstCall + scoreLen);
    ExpectLocation(actual[thirdLocation], "rename5_function.ets", secondCall, secondCall + scoreLen);
}

// Plan: class rename - declaration, type annotation and new expression are collected.
TEST_F(LspFindRenameLocationsTests5, ClassRenameLocations)
{
    const std::string source = "class Car {}\nlet mine: Car = new Car();\nmine;";
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext("rename5_class.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    const auto position = source.find("Car {}");
    ASSERT_NE(position, std::string::npos);
    auto res = FindRenameLocationsInCurrentFile(context, position);
    initializer.DestroyContext(context);

    auto actual = ToSortedVector(res);
    size_t const expectedCount = 3;
    ASSERT_EQ(actual.size(), expectedCount);
    size_t const declStart = 6;
    size_t const typeRef = 23;
    size_t const newExpr = 33;
    size_t const carLen = std::string("Car").size();
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t thirdLocation = 2;
    ExpectLocation(actual[0], "rename5_class.ets", declStart, declStart + carLen);
    ExpectLocation(actual[1], "rename5_class.ets", typeRef, typeRef + carLen);
    ExpectLocation(actual[thirdLocation], "rename5_class.ets", newExpr, newExpr + carLen);
}

// Plan: interface rename - declaration, type annotation and property access are collected.
TEST_F(LspFindRenameLocationsTests5, InterfaceRenameLocations)
{
    const std::string source = "interface Shape {\n  name: string;\n}\nlet s: Shape = { name: \"round\" };\ns.name;";
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext("rename5_interface.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    const auto position = source.find("Shape {");
    ASSERT_NE(position, std::string::npos);
    auto res = FindRenameLocationsInCurrentFile(context, position);
    initializer.DestroyContext(context);

    auto actual = ToSortedVector(res);
    size_t const expectedCount = 2;
    ASSERT_EQ(actual.size(), expectedCount);
    size_t const declStart = 10;
    size_t const typeRef = 43;
    size_t const shapeLen = std::string("Shape").size();
    ExpectLocation(actual[0], "rename5_interface.ets", declStart, declStart + shapeLen);
    ExpectLocation(actual[1], "rename5_interface.ets", typeRef, typeRef + shapeLen);
}

// Plan: type alias rename - declaration and both type annotations are collected.
TEST_F(LspFindRenameLocationsTests5, TypeAliasRenameLocations)
{
    const std::string source = "type Num = number;\nlet first: Num = 1;\nlet second: Num = 2;";
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext("rename5_type_alias.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    const auto position = source.find("Num = number");
    ASSERT_NE(position, std::string::npos);
    auto res = FindRenameLocationsInCurrentFile(context, position);
    initializer.DestroyContext(context);

    auto actual = ToSortedVector(res);
    size_t const expectedCount = 3;
    ASSERT_EQ(actual.size(), expectedCount);
    size_t const declStart = 5;
    size_t const firstUse = 30;
    size_t const secondUse = 51;
    size_t const numLen = std::string("Num").size();
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t thirdLocation = 2;
    ExpectLocation(actual[0], "rename5_type_alias.ets", declStart, declStart + numLen);
    ExpectLocation(actual[1], "rename5_type_alias.ets", firstUse, firstUse + numLen);
    ExpectLocation(actual[thirdLocation], "rename5_type_alias.ets", secondUse, secondUse + numLen);
}

// Plan: import alias rename - alias in the import specifier and every usage are collected.
TEST_F(LspFindRenameLocationsTests5, ImportAliasRenameLocations)
{
    std::vector<std::string> files = {"rename5_import_alias_export.ets", "rename5_import_alias_use.ets"};
    std::vector<std::string> texts = {
        "export class Gadget {\n  level: number = 1;\n}",
        "import { Gadget as Tool } from './rename5_import_alias_export';\nlet dev: Tool = new Tool();\ndev.level;"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto position = texts[1].find("Tool }");
    ASSERT_NE(position, std::string::npos);
    auto res = FindRenameLocationsInCurrentFile(context, position);
    initializer.DestroyContext(context);

    auto actual = ToSortedVector(res);
    size_t const expectedCount = 3;
    ASSERT_EQ(actual.size(), expectedCount);
    size_t const aliasInImport = 19;
    size_t const typeRef = 73;
    size_t const newExpr = 84;
    size_t const toolLen = std::string("Tool").size();
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t thirdLocation = 2;
    ExpectLocation(actual[0], filePaths[1], aliasInImport, aliasInImport + toolLen);
    ExpectLocation(actual[1], filePaths[1], typeRef, typeRef + toolLen);
    ExpectLocation(actual[thirdLocation], filePaths[1], newExpr, newExpr + toolLen);
}

// Plan: property rename - declaration, this.foo and external access obj.foo are collected.
TEST_F(LspFindRenameLocationsTests5, PropertyRenameThroughThisAndExternalAccess)
{
    const std::string source = R"(class Counter {
  count: number = 0;
  bump(): void {
    this.count = 1;
  }
}
let c = new Counter();
c.count = 2;)";
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext("rename5_property.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    const auto position = source.find("count: number");
    ASSERT_NE(position, std::string::npos);
    auto res = FindRenameLocationsInCurrentFile(context, position);
    initializer.DestroyContext(context);

    auto actual = ToSortedVector(res);
    size_t const expectedCount = 3;
    ASSERT_EQ(actual.size(), expectedCount);
    size_t const declStart = 18;
    size_t const thisAccess = 63;
    size_t const externalAccess = 105;
    size_t const countLen = std::string("count").size();
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t thirdLocation = 2;
    ExpectLocation(actual[0], "rename5_property.ets", declStart, declStart + countLen);
    ExpectLocation(actual[1], "rename5_property.ets", thisAccess, thisAccess + countLen);
    ExpectLocation(actual[thirdLocation], "rename5_property.ets", externalAccess, externalAccess + countLen);
}

// Plan: shorthand property `{ foo }` rename - declaration, shorthand usage and member access are collected.
TEST_F(LspFindRenameLocationsTests5, ShorthandPropertyRenameLocations)
{
    const std::string source = "let foo: number = 1;\nlet obj = { foo };\nobj.foo;";
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext("rename5_shorthand.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    const auto position = source.find("foo: number");
    ASSERT_NE(position, std::string::npos);
    auto res = FindRenameLocationsInCurrentFile(context, position);
    initializer.DestroyContext(context);

    auto actual = ToSortedVector(res);
    size_t const expectedCount = 3;
    ASSERT_EQ(actual.size(), expectedCount);
    size_t const declStart = 4;
    size_t const shorthand = 33;
    size_t const memberAccess = 44;
    size_t const fooLen = std::string("foo").size();
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t thirdLocation = 2;
    ExpectLocation(actual[0], "rename5_shorthand.ets", declStart, declStart + fooLen);
    ExpectLocation(actual[1], "rename5_shorthand.ets", shorthand, shorthand + fooLen);
    ExpectLocation(actual[thirdLocation], "rename5_shorthand.ets", memberAccess, memberAccess + fooLen);
}

// ETS currently rejects object-destructuring declarations during parsing. Rename
// must therefore stay empty and, importantly, must not reinterpret the invalid
// pattern text as references to the same-named object-literal property.
TEST_F(LspFindRenameLocationsTests5, UnsupportedDestructuringSyntaxReturnsNoRenameLocations)
{
    const std::string source = "let cfg = { port: 80 };\nlet { port } = cfg;\nport;";
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext("rename5_destructuring.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    const auto bracePos = source.find("{ port }");
    ASSERT_NE(bracePos, std::string::npos);
    const auto position = bracePos + std::string("{ ").size();  // on the destructured "port" binding
    auto res = FindRenameLocationsInCurrentFile(context, position);
    initializer.DestroyContext(context);

    EXPECT_TRUE(res.empty());
    size_t const objectLiteralProp = 12;
    size_t const usage = 44;
    for (const auto &loc : res) {
        EXPECT_NE(loc.start, objectLiteralProp);
        EXPECT_NE(loc.start, usage);
    }
}

// Plan: string literals and comments containing the same text are never rename locations.
TEST_F(LspFindRenameLocationsTests5, RenameSkipsStringLiteralAndComment)
{
    const std::string source = R"(// marker in comment
let marker = 1;
let text = "marker in string";
marker;
)";
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext("rename5_string_comment.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    const auto position = source.find("marker = 1");
    ASSERT_NE(position, std::string::npos);
    auto res = FindRenameLocationsInCurrentFile(context, position);
    initializer.DestroyContext(context);

    auto actual = ToSortedVector(res);
    size_t const expectedCount = 2;
    ASSERT_EQ(actual.size(), expectedCount);
    size_t const declStart = 25;
    size_t const usage = 68;
    size_t const markerLen = std::string("marker").size();
    ExpectLocation(actual[0], "rename5_string_comment.ets", declStart, declStart + markerLen);
    ExpectLocation(actual[1], "rename5_string_comment.ets", usage, usage + markerLen);

    const auto commentPos = source.find("marker in comment");
    const auto stringPos = source.find("marker in string");
    ASSERT_NE(commentPos, std::string::npos);
    ASSERT_NE(stringPos, std::string::npos);
    for (const auto &loc : actual) {
        EXPECT_NE(loc.start, commentPos);
        EXPECT_NE(loc.start, stringPos);
    }
}

// Plan: cancellation token - a cancelled token interrupts the cross-file rename search.
TEST_F(LspFindRenameLocationsTests5, CancellationTokenInterruptsRenameSearch)
{
    std::vector<std::string> files = {"rename5_cancel_export.ets", "rename5_cancel_use.ets"};
    std::vector<std::string> texts = {"export function ping(): number {\n  return 1;\n}",
                                      "import { ping } from './rename5_cancel_export';\nping();"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    std::vector<es2panda_Context *> fileContexts;
    for (const auto &filePath : filePaths) {
        auto *fileContext = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);
        ASSERT_NE(fileContext, nullptr);
        fileContexts.push_back(fileContext);
    }

    const auto position = texts[1].find("ping }");
    ASSERT_NE(position, std::string::npos);

    CancelledHost cancelledHost;
    CancellationToken cancelledToken {DEFAULT_THROTTLE, &cancelledHost};
    auto cancelledRes = FindRenameLocations(&cancelledToken, fileContexts, fileContexts[1], position);
    EXPECT_TRUE(cancelledRes.empty());

    NotCancelledHost notCancelledHost;
    CancellationToken liveToken {DEFAULT_THROTTLE, &notCancelledHost};
    auto liveRes = FindRenameLocations(&liveToken, fileContexts, fileContexts[1], position);
    size_t const expectedCount = 3;
    ASSERT_EQ(liveRes.size(), expectedCount);

    for (auto *fileContext : fileContexts) {
        initializer.DestroyContext(fileContext);
    }
}

// Wrapper-level test: NeedsCrossFileRenameWrapper returns true for a member
// access whose declaration lives in another file (the rename must cross file
// boundaries).
TEST_F(LspFindRenameLocationsTests5, NeedsCrossFileRenameForExportedMemberAccess)
{
    std::vector<std::string> files = {"rename5_need_cross_export.ets", "rename5_need_cross_use.ets"};
    std::vector<std::string> texts = {"export class SharedService {\n  sharedValue: number = 1;\n  serve(): void {}\n}",
                                      "import { SharedService } from './rename5_need_cross_export';\n"
                                      "let svc = new SharedService();\nsvc.sharedValue = 2;"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Query at the member access "sharedValue": the declaration lives in the
    // export file, so the wrapper must report a cross-file rename.
    const auto position = texts[1].find("sharedValue = 2");
    ASSERT_NE(position, std::string::npos);
    LSPAPI const *lspApi = GetImpl();
    bool needsCrossFile = lspApi->needsCrossFileRename(context, position);
    initializer.DestroyContext(context);

    ASSERT_TRUE(needsCrossFile);
}

// Wrapper-level test: NeedsCrossFileRenameWrapper returns false for a local
// symbol confined to a single file.
TEST_F(LspFindRenameLocationsTests5, NeedsCrossFileRenameFalseForLocalSymbol)
{
    const std::string source = "function localOnly(): number {\n  return 1;\n}\nlocalOnly();";
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext("rename5_local_only.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    const auto position = source.find("localOnly();");
    ASSERT_NE(position, std::string::npos);
    LSPAPI const *lspApi = GetImpl();
    bool needsCrossFile = lspApi->needsCrossFileRename(context, position);
    initializer.DestroyContext(context);

    ASSERT_FALSE(needsCrossFile);
}

}  // namespace
