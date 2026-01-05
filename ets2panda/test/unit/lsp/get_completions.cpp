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

#include "lsp_api_test.h"
#include "lsp/include/completions.h"
#include "lsp/include/internal_api.h"

class LSPCompletionsTests : public LSPAPITests {};

using ark::es2panda::lsp::CompletionEntry;
using ark::es2panda::lsp::CompletionEntryKind;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::sort_text::CLASS_MEMBER_SNIPPETS;
using ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS;
using ark::es2panda::lsp::sort_text::MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT;
using ark::es2panda::lsp::sort_text::SUGGESTED_CLASS_MEMBERS;

static void AssertCompletionsContainAndNotContainEntries(const std::vector<CompletionEntry> &entries,
                                                         const std::vector<CompletionEntry> &expectedEntries,
                                                         const std::vector<CompletionEntry> &unexpectedEntries)
{
    auto emptyCheck = expectedEntries.empty() && !entries.empty();
    ASSERT_FALSE(emptyCheck) << "Expected empty but the result is not. Actual account: " << entries.size();

    for (const auto &expectedEntry : expectedEntries) {
        bool found = false;
        for (const auto &entry : entries) {
            if (entry.GetName() == expectedEntry.GetName() &&
                entry.GetCompletionKind() == expectedEntry.GetCompletionKind() &&
                entry.GetInsertText() == expectedEntry.GetInsertText()) {
                found = true;
                break;
            }
        }
        ASSERT_TRUE(found) << "Expected completion '" << expectedEntry.GetName() << "' not found";
    }

    for (const auto &unexpectedEntry : unexpectedEntries) {
        bool found = false;
        for (const auto &entry : entries) {
            if (entry.GetName() == unexpectedEntry.GetName() &&
                entry.GetCompletionKind() == unexpectedEntry.GetCompletionKind() &&
                entry.GetInsertText() == unexpectedEntry.GetInsertText()) {
                found = true;
                break;
            }
        }
        ASSERT_FALSE(found) << "Unexpected completion '" << unexpectedEntry.GetName() << "' found";
    }
}

namespace {

TEST_F(LSPCompletionsTests, getCompletionsAtPosition26)
{
    std::vector<std::string> files = {"getCompletionsAtPosition26.ets"};
    std::vector<std::string> texts = {R"delimiter(
Ab
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 3;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("AbcFile", ark::es2panda::lsp::CompletionEntryKind::CLASS, std::string(GLOBALS_OR_KEYWORDS),
                        "AbcFile"),
        CompletionEntry("AbcFileNotFoundError", ark::es2panda::lsp::CompletionEntryKind::CLASS,
                        std::string(GLOBALS_OR_KEYWORDS), "AbcFileNotFoundError")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition25)
{
    std::vector<std::string> files = {"getCompletionsAtPosition25.ets"};
    std::vector<std::string> texts = {R"delimiter(
bo
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 3;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("Box", ark::es2panda::lsp::CompletionEntryKind::CLASS, std::string(GLOBALS_OR_KEYWORDS), "Box"),
        CompletionEntry("Boolean", ark::es2panda::lsp::CompletionEntryKind::CLASS, std::string(GLOBALS_OR_KEYWORDS),
                        "Boolean"),
        CompletionEntry("BootRuntimeLinker", ark::es2panda::lsp::CompletionEntryKind::CLASS,
                        std::string(GLOBALS_OR_KEYWORDS), "BootRuntimeLinker")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPositionMemberKeyWord)
{
    std::vector<std::string> files = {"getCompletionsAtPositionMemberKeyWord.ets"};
    std::vector<std::string> texts = {R"delimiter(
class Person {
    name: string = '';
    age: number = 0;

    constructor(name: string, age: number) {
        this.name = name;
        this.age = age;
    }

    introduce(name: string, age: number): void {}
 }

 let p: Person = new Person("ab", 18);
 p.int
 p.in
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset1 = 262;  // p.int
    size_t const offset2 = 268;  // p.in
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res1 = lspApi->getCompletionsAtPosition(ctx, offset1);
    auto res2 = lspApi->getCompletionsAtPosition(ctx, offset2);
    auto expectedEntries1 = std::vector<CompletionEntry> {CompletionEntry(
        "introduce", ark::es2panda::lsp::CompletionEntryKind::METHOD, std::string(GLOBALS_OR_KEYWORDS), "introduce()")};
    AssertCompletionsContainAndNotContainEntries(res1.GetEntries(), expectedEntries1, {});
    AssertCompletionsContainAndNotContainEntries(res2.GetEntries(), expectedEntries1, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPositionParamMember)
{
    std::vector<std::string> files = {"getCompletionsAtPositionParamMember.ets"};
    std::vector<std::string> texts = {R"delimiter(
function json2Array(jsonArr: Array<number>) {
      jsonArr.
}
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset1 = 61;  // jsonArr.
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res1 = lspApi->getCompletionsAtPosition(ctx, offset1);
    auto expectedEntries1 = std::vector<CompletionEntry> {CompletionEntry(
        "at", ark::es2panda::lsp::CompletionEntryKind::METHOD, std::string(GLOBALS_OR_KEYWORDS), "at()")};
    AssertCompletionsContainAndNotContainEntries(res1.GetEntries(), expectedEntries1, {});
    initializer.DestroyContext(ctx);
}
TEST_F(LSPCompletionsTests, getCompletionsAtPosition24)
{
    std::vector<std::string> files = {"getCompletionsAtPosition25.ets"};
    std::vector<std::string> texts = {R"delimiter(
'use static'
export fun
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 24;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "function", ark::es2panda::lsp::CompletionEntryKind::KEYWORD, std::string(GLOBALS_OR_KEYWORDS), "function")};
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition23)
{
    std::vector<std::string> files = {"getCompletionsAtPosition24.ets"};
    std::vector<std::string> texts = {R"delimiter(
'use static'
export function aa(n)
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 34;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), {}, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition22)
{
    std::vector<std::string> files = {"getCompletionsAtPosition23.ets"};
    std::vector<std::string> texts = {R"delimiter(
'use static'
export function a
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 31;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), {}, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition21)
{
    std::vector<std::string> files = {"getCompletionsAtPosition22.ets"};
    std::vector<std::string> texts = {R"delimiter(
export interface CommonMethod {
  width(value: number): this;
  height(value: number): this;
}
export interface ColumnAttribute extends CommonMethod {
  alignItems(value: string): this;
  justifyContent(value: string): this;
}
export declare function Column(content?: string): ColumnAttribute
export class Test {}

Column(new Te) {}
.width(100)
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 329;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "Test", ark::es2panda::lsp::CompletionEntryKind::MODULE, std::string(GLOBALS_OR_KEYWORDS), "Test")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition20)
{
    std::vector<std::string> files = {"getCompletionsAtPosition21.ets"};
    std::vector<std::string> texts = {R"delimiter(
export interface CommonMethod {
  width(value: number): this;
  height(value: number): this;
}
export interface ColumnAttribute extends CommonMethod {
  alignItems(value: string): this;
  justifyContent(value: string): this;
}
export declare function Column(content?: string): ColumnAttribute

Column() {
  Col
}
.width(100)
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 311;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "Column", ark::es2panda::lsp::CompletionEntryKind::FUNCTION, std::string(GLOBALS_OR_KEYWORDS), "Column()")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPositionMultiMember)
{
    std::vector<std::string> files = {"getCompletionsAtPositionMultiMember.ets"};
    std::vector<std::string> texts = {R"delimiter(
class MyC {
  value: number;
  meth(): MyD {};
}
class MyD {
  val: MyC = new MyC()
}
let d = new MyD()
d.val.meth().val.
d.val   // mock next line
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 122;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("meth", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(GLOBALS_OR_KEYWORDS), "meth()"),
                                      CompletionEntry("value", ark::es2panda::lsp::CompletionEntryKind::PROPERTY,
                                                      std::string(GLOBALS_OR_KEYWORDS), "value")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition19)
{
    std::vector<std::string> files = {"getCompletionsAtPosition20.ets"};
    std::vector<std::string> texts = {R"delimiter(
deep
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 5;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "deepcopy", ark::es2panda::lsp::CompletionEntryKind::METHOD, std::string(GLOBALS_OR_KEYWORDS), "deepcopy()")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition18)
{
    std::vector<std::string> files = {"getCompletionsAtPosition19.ets"};
    std::vector<std::string> texts = {R"delimiter(
Readonl
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 8;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("ReadonlyArray", ark::es2panda::lsp::CompletionEntryKind::INTERFACE,
                        std::string(GLOBALS_OR_KEYWORDS), "ReadonlyArray")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition17)
{
    std::vector<std::string> files = {"getCompletionsAtPosition18.ets"};
    std::vector<std::string> texts = {R"delimiter(
con
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 4;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("console", ark::es2panda::lsp::CompletionEntryKind::PROPERTY, std::string(GLOBALS_OR_KEYWORDS),
                        "console"),
        CompletionEntry("Console", ark::es2panda::lsp::CompletionEntryKind::CLASS, std::string(GLOBALS_OR_KEYWORDS),
                        "Console"),
        CompletionEntry("ConcurrentHashMap", ark::es2panda::lsp::CompletionEntryKind::CLASS,
                        std::string(GLOBALS_OR_KEYWORDS), "ConcurrentHashMap")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition16)
{
    std::vector<std::string> files = {"getCompletionsAtPosition17.ets"};
    std::vector<std::string> texts = {R"delimiter(
struct MyClass {
  property: string = '1'
  get() {
    return this.
  }
}
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 69;  // after 'return this.'
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("property", ark::es2panda::lsp::CompletionEntryKind::PROPERTY,
                                                      std::string(SUGGESTED_CLASS_MEMBERS), "property"),
                                      CompletionEntry("get", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "get()")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition15)
{
    std::vector<std::string> files = {"getCompletionsAtPosition16.ets"};
    std::vector<std::string> texts = {R"delimiter(
export interface Method {
  get(value: number): this;
}
export interface CommonMethod {
  width(value: number): this;
  height(value: number): this;
}
export interface TextAttribute extends CommonMethod {
  font(value: number): this;
  fontColor(value: number): this;
}
export declare function Text(
  content?: string,
  value?: string
): TextAttribute
Text("Hello").font(1).
Text("Hello")   // mock next line
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 377;  // after '.font().'
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("font", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "font()"),
                                      CompletionEntry("fontColor", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "fontColor()"),
                                      CompletionEntry("width", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "width()"),
                                      CompletionEntry("height", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "height()")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition14)
{
    std::vector<std::string> files = {"getCompletionsAtPosition15.ets"};
    std::vector<std::string> texts = {R"delimiter(
export class MyClass0 {
  public property0: string = '0'
  public get0() {}
}
export class MyClass extends MyClass0 {
  public property: string = '1'
  public get() {}
}
export class MySonClass extends MyClass {
  public property2: string = '2'
}
let c = new MySonClass()
let p = c.
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 283;  // after 'let p = c.'
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("property", ark::es2panda::lsp::CompletionEntryKind::PROPERTY,
                                                      std::string(SUGGESTED_CLASS_MEMBERS), "property"),
                                      CompletionEntry("property0", ark::es2panda::lsp::CompletionEntryKind::PROPERTY,
                                                      std::string(SUGGESTED_CLASS_MEMBERS), "property0"),
                                      CompletionEntry("property2", ark::es2panda::lsp::CompletionEntryKind::PROPERTY,
                                                      std::string(SUGGESTED_CLASS_MEMBERS), "property2"),
                                      CompletionEntry("get", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "get()"),
                                      CompletionEntry("get0", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "get0()")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition13)
{
    std::vector<std::string> files = {"getCompletionsAtPosition14.ets"};
    std::vector<std::string> texts = {R"delimiter(
export interface Method {
  get(value: number): this;
}
export interface CommonMethod {
  width(value: number): this;
  height(value: number): this;
}
export interface TextAttribute extends CommonMethod {
  font(value: number): this;
  fontColor(value: number): this;
}
export declare function Text(
  content?: string,
  value?: string
): TextAttribute
Text("Hello").
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 369;  // after 'Text("Hello").'
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("font", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "font()"),
                                      CompletionEntry("fontColor", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "fontColor()"),
                                      CompletionEntry("width", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "width()"),
                                      CompletionEntry("height", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "height()")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition12)
{
    std::vector<std::string> files = {"getCompletionsAtPosition13.ets"};
    std::vector<std::string> texts = {R"delimiter(
class JSON {
  public static stringify(d: byte): String {
    return StringBuilder.toString(d)
  }
}
let j = new JSON()
let res = j.
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 133;  // after 'let res = j.'
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("stringify", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "stringify()")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition11)
{
    std::vector<std::string> files = {"getCompletionsAtPosition12.ets"};
    std::vector<std::string> texts = {R"delimiter(
interface Inner { key : string; }
let i: Inner
i.k
let a = 1;)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 51;  // after 'i.k'
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "key", ark::es2panda::lsp::CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS), "key")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition10)
{
    std::vector<std::string> files = {"getCompletionsAtPosition11.ets"};
    std::vector<std::string> texts = {R"delimiter(
interface Inner { key : string; }
let i: Inner
i.
let a = 1;)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 50;  // after 'i.'
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "key", ark::es2panda::lsp::CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS), "key")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition7)
{
    std::vector<std::string> files = {"getCompletionsAtPosition10.ets"};
    std::vector<std::string> texts = {R"delimiter(
class MyClass {
  public myProp: number = 0;
  public prop: number = 0;
}
let obj = new MyClass();
let p = obj.
let a = 1;)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 112;  // after 'obj.' in 'let p = obj._WILDCARD'
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("myProp", ark::es2panda::lsp::CompletionEntryKind::PROPERTY,
                                                      std::string(SUGGESTED_CLASS_MEMBERS), "myProp"),
                                      CompletionEntry("prop", ark::es2panda::lsp::CompletionEntryKind::PROPERTY,
                                                      std::string(SUGGESTED_CLASS_MEMBERS), "prop")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition8)
{
    std::vector<std::string> files = {"getCompletionsAtPosition11.ets"};
    std::vector<std::string> texts = {R"delimiter(
enum Color {
  Red = "red",
  Blue = "blue"
}
let myColor: Color = Color.)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 74;  // after '= Color.'
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("Red", CompletionEntryKind::ENUM_MEMBER,
                                                      std::string(MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), "Red"),
                                      CompletionEntry("Blue", CompletionEntryKind::ENUM_MEMBER,
                                                      std::string(MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), "Blue")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getImportStatementCompletion1)
{
    std::vector<std::string> files = {"exportFile1.ets", "importStatementCompletion1.ets"};
    std::vector<std::string> texts = {R"('use static'
namespace expName {}
export default expName
)",
                                      R"('use static'
import {} f
)"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 24;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "from", CompletionEntryKind::KEYWORD, std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "from")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getImportStatementCompletion2)
{
    std::vector<std::string> files = {"exportFile2.ets", "importStatementCompletion2.ets"};
    std::vector<std::string> texts = {R"(
namespace expName {}
export default expName
)",
                                      R"(
import {x} f
)"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 13;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "from", CompletionEntryKind::KEYWORD, std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "from")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getImportStatementCompletion3)
{
    std::vector<std::string> files = {"exportFile3.ets", "getImportStatementCompletion3.ets"};
    std::vector<std::string> texts = {R"(
export class A {
}
)",
                                      R"(
import { A } from './get'
)"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 24;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "getImportStatementCompletion3", CompletionEntryKind::FILE,
        std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "getImportStatementCompletion3")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, importStatementCompletionPath1)
{
    std::vector<std::string> files = {"exportFile4.ets", "importStatementCompletionPath1.ets"};
    std::vector<std::string> texts = {R"(
export class A {
}
)",
                                      R"(
import { A } from './'
)"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 22;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "importStatementCompletionPath1", CompletionEntryKind::FILE,
        std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "importStatementCompletionPath1")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, importStatementCompletionPath2)
{
    std::vector<std::string> files = {"exportFile5.ets", "importStatementCompletionPath2.ets"};
    std::vector<std::string> texts = {R"(
export class A {
}
)",
                                      R"(
import { A }
)"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 13;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "from", CompletionEntryKind::KEYWORD, std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "from")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, nagetiveGetImportStatementCompletion1)
{
    std::vector<std::string> files = {"exportFile6.ets", "nagetiveGetImportStatementCompletion1.ets"};
    std::vector<std::string> texts = {R"(
export class A {}
)",
                                      R"(
import
)"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 7;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    ASSERT_EQ(entries.size(), 0);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, nagetiveGetImportStatementCompletion2)
{
    std::vector<std::string> files = {"exportFile7.ets", "nagetiveGetImportStatementCompletion2.ets"};
    std::vector<std::string> texts = {R"(
export class A {
}
)",
                                      R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
import { A } from 
)"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 57;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    ASSERT_EQ(entries.size(), 0);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, nagetiveGetImportStatementCompletion3)
{
    std::vector<std::string> files = {"exportFile8.ets", "nagetiveGetImportStatementCompletion3.ets"};
    std::vector<std::string> texts = {R"(
export class A {
}
)",
                                      R"(
import { A } a
)"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 15;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    ASSERT_EQ(entries.size(), 0);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPositionForSpecialCharacters)
{
    std::vector<std::string> files = {"getCompletionsAtPositionForSpecialCharacters.ets"};
    std::vector<std::string> texts = {R"delimiter(
class Person {
    name: string = '';
    age: number = 0;

    constructor(name: string, age: number) {
        this.name = name;
        this.age = age;
    }

    //中文测试
    introduce(name: string, age: number): void {}
 }

 //中文测试
 let p: Person = new Person("中文测试", 18);
 //中文测试
 p.int
 //中文测试
 p.in
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset1 = 291;  // p.int
    size_t const offset2 = 305;  // p.in
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res1 = lspApi->getCompletionsAtPosition(ctx, offset1);
    auto res2 = lspApi->getCompletionsAtPosition(ctx, offset2);
    auto expectedEntries1 = std::vector<CompletionEntry> {CompletionEntry(
        "introduce", ark::es2panda::lsp::CompletionEntryKind::METHOD, std::string(GLOBALS_OR_KEYWORDS), "introduce()")};
    AssertCompletionsContainAndNotContainEntries(res1.GetEntries(), expectedEntries1, {});
    AssertCompletionsContainAndNotContainEntries(res2.GetEntries(), expectedEntries1, {});
    initializer.DestroyContext(ctx);
}

}  // namespace
