/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include <sstream>

#include "lsp_api_test.h"
#include "lsp/include/completions.h"
#include "lsp/include/internal_api.h"

class LSPCompletionsTests : public LSPAPITests {};

using ark::es2panda::lsp::CompletionEntry;
using ark::es2panda::lsp::CompletionEntryKind;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::sort_text::AUTO_IMPORT_SUGGESTIONS;
using ark::es2panda::lsp::sort_text::CLASS_MEMBER_SNIPPETS;
using ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS;
using ark::es2panda::lsp::sort_text::SUGGESTED_CLASS_MEMBERS;

static bool IsMatchedCompletionEntry(const CompletionEntry &actual, const CompletionEntry &expected)
{
    return actual.GetCompletionKind() == expected.GetCompletionKind() &&
           actual.GetInsertText() == expected.GetInsertText() && actual.GetName() == expected.GetName();
}

static std::string FormatCompletionEntry(const CompletionEntry &entry)
{
    std::ostringstream output;
    output << "'" << entry.GetName() << "' (kind: " << static_cast<int>(entry.GetCompletionKind()) << ", insertText: '"
           << entry.GetInsertText() << "')";
    return output.str();
}

static std::string FormatCompletionEntries(const std::vector<CompletionEntry> &entries)
{
    std::ostringstream output;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (i != 0) {
            output << ", ";
        }
        output << FormatCompletionEntry(entries[i]);
    }
    return output.str();
}

static void AssertCompletionsContainAndNotContainEntries(const std::vector<CompletionEntry> &entries,
                                                         const std::vector<CompletionEntry> &expectedEntries,
                                                         const std::vector<CompletionEntry> &unexpectedEntries)
{
    auto emptyCheck = expectedEntries.empty() && !entries.empty();
    ASSERT_FALSE(emptyCheck) << "Expected empty but the result is not. Actual account: " << entries.size();

    for (const auto &expectedEntry : expectedEntries) {
        bool found = false;
        for (const auto &entry : entries) {
            if (IsMatchedCompletionEntry(entry, expectedEntry)) {
                found = true;
                break;
            }
        }
        ASSERT_TRUE(found) << "Expected completion " << FormatCompletionEntry(expectedEntry)
                           << " not found. Actual completions: " << FormatCompletionEntries(entries);
    }

    for (const auto &unexpectedEntry : unexpectedEntries) {
        bool found = false;
        for (const auto &entry : entries) {
            if (IsMatchedCompletionEntry(entry, unexpectedEntry)) {
                found = true;
                break;
            }
        }
        ASSERT_FALSE(found) << "Unexpected completion " << FormatCompletionEntry(unexpectedEntry)
                            << " found. Actual completions: " << FormatCompletionEntries(entries);
    }
}

static void AssertCompletionsNotContainEntries(const std::vector<CompletionEntry> &entries,
                                               const std::vector<CompletionEntry> &unexpectedEntries)
{
    for (const auto &unexpectedEntry : unexpectedEntries) {
        ASSERT_TRUE(std::none_of(entries.begin(), entries.end(),
                                 [&unexpectedEntry](const CompletionEntry &entry) {
                                     return IsMatchedCompletionEntry(entry, unexpectedEntry);
                                 }))
            << "Unexpected completion '" << unexpectedEntry.GetName() << "' found";
    }
}

static void AssertCompletionsOrder(const std::vector<CompletionEntry> &entries,
                                   const std::vector<CompletionEntry> &expectedEntries)
{
    if (expectedEntries.empty()) {
        return;
    }

    size_t prevPos = std::string::npos;
    for (const auto &expectedEntry : expectedEntries) {
        size_t currentPos = std::string::npos;
        for (size_t i = 0; i < entries.size(); ++i) {
            if (IsMatchedCompletionEntry(entries[i], expectedEntry)) {
                currentPos = i;
                break;
            }
        }
        ASSERT_TRUE(currentPos != std::string::npos)
            << "Expected completion '" << expectedEntry.GetName() << "' not found";

        if (prevPos != std::string::npos) {
            ASSERT_TRUE(currentPos > prevPos)
                << "Expected completion '" << expectedEntry.GetName()
                << "' should come after previous expected completion, but found at position " << currentPos
                << " (previous at position " << prevPos << ")";
        }
        prevPos = currentPos;
    }
}

namespace {
TEST_F(LSPCompletionsTests, KeyWordCompletionsToInclude2)
{
    std::vector<std::string> files = {"KeyWordCompletionsToInclude2.ets"};
    std::vector<std::string> texts = {R"delimiter(
function sortfunc(a: number) {}
so
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 35;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    std::string res1 = "sortfunc(a: number): void";
    std::string res2 = "console: Console";
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry(res1, CompletionEntryKind::FUNCTION,
                        std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "sortfunc()"),
        CompletionEntry(res2, CompletionEntryKind::PROPERTY,
                        std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "console")};
    initializer.DestroyContext(ctx);
    AssertCompletionsOrder(entries, expectedEntries);
}

TEST_F(LSPCompletionsTests, KeyWordCompletionsToInclude1)
{
    std::vector<std::string> files = {"KeyWordCompletionsToInclude1.ets"};
    std::vector<std::string> texts = {R"delimiter(
class cal {}
l
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 15;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    std::string res1 = "let";
    std::string res2 = "cal";
    std::string res3 = "null";
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry(res1, CompletionEntryKind::KEYWORD,
                        std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), res1),
        CompletionEntry(res2, CompletionEntryKind::MODULE,
                        std::string(ark::es2panda::lsp::sort_text::CLASS_MEMBER_SNIPPETS), res2),
        CompletionEntry(res3, CompletionEntryKind::KEYWORD,
                        std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), res3)};
    initializer.DestroyContext(ctx);
    AssertCompletionsOrder(entries, expectedEntries);
}

TEST_F(LSPCompletionsTests, MemberCompletionsForClassTest8848)
{
    // test interface
    std::vector<std::string> files = {"getCompletionsAtPositionMember8848.ets"};
    std::vector<std::string> texts = {R"delimiter(
namespace AA {
  interface B {}
  function F(): void {}
}

AA.
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 63;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    std::string propertyName1 = "F";
    std::string propertyName2 = "B";
    int const expectedPropertyCount = 0;
    ASSERT_TRUE(entries.size() == expectedPropertyCount);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPositionAnnotation2200)
{
    std::vector<std::string> files = {"defaultExport2200.ets"};
    std::vector<std::string> texts = {R"(
interface MyI {
  get(value :number): this
}
My
)"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 48;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto unexpectedDEntries = std::vector<CompletionEntry> {CompletionEntry(
        "MyI", ark::es2panda::lsp::CompletionEntryKind::KEYWORD, std::string(SUGGESTED_CLASS_MEMBERS), "MyI")};
    AssertCompletionsNotContainEntries(res.GetEntries(), unexpectedDEntries);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPositionAnnotation4396)
{
    std::vector<std::string> files = {"defaultExport4396.ets"};
    std::vector<std::string> texts = {R"(
class MyC {
  value: number;
}

My
)"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 35;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    std::string packetName = "MyC";
    auto expectedEntries = CompletionEntry(packetName, CompletionEntryKind::MODULE,
                                           std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "MyC");
    ASSERT_EQ(expectedEntries, entries[0]);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPositionAnnotation4598)
{
    std::vector<std::string> files = {"defaultExport4598.ets", "importCompletion4598.ets"};
    std::vector<std::string> texts = {R"(
namespace expName {}
export default expName
)",
                                      R"(
import expName from './defaultExport4598.ets'
exp
)"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 50;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    std::string packetName = "expName";
    auto expectedEntries = CompletionEntry(packetName, CompletionEntryKind::MODULE,
                                           std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "expName");
    ASSERT_EQ(expectedEntries, entries[0]);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, MemberCompletionsForClassTest9)
{
    std::vector<std::string> files = {"getCompletionsAtPositionMember9.ets"};
    std::vector<std::string> texts = {R"delimiter(
namespace space {
  export class classInSpace {
    public  c: number = 2;
  }
}
let numOfSpace: space.)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 104;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    std::string propertyName1 = "classInSpace";
    ASSERT_TRUE(entries.size() == 1);
    CompletionEntry entry1 = CompletionEntry(
        propertyName1, CompletionEntryKind::CLASS,
        std::string(ark::es2panda::lsp::sort_text::MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), propertyName1);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(entry1, entries[0]);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition6)
{
    std::vector<std::string> files = {"getCompletionsAtPosition9.ets"};
    std::vector<std::string> texts = {R"delimiter(
let a: num
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 11;  // after 'n' in 'let a = n'
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "number", ark::es2panda::lsp::CompletionEntryKind::KEYWORD, std::string(GLOBALS_OR_KEYWORDS), "number")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition5)
{
    std::vector<std::string> files = {"getCompletionsAtPosition8.ets"};
    std::vector<std::string> texts = {R"delimiter(
class
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 6;  // after 'ss' in 'class'
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition0)
{
    std::vector<std::string> files = {"getCompletionsAtPosition7.ets"};
    std::vector<std::string> texts = {R"delimiter(
function num1() {
    return 1;
}

function num2() {
    return 2;
}

console.log(1);

let a = n
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 97;  // after 'n' in 'let a = n'
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("num1(): Int", ark::es2panda::lsp::CompletionEntryKind::FUNCTION,
                        std::string(GLOBALS_OR_KEYWORDS), "num1()"),
        CompletionEntry("num2(): Int", ark::es2panda::lsp::CompletionEntryKind::FUNCTION,
                        std::string(GLOBALS_OR_KEYWORDS), "num2()"),
    };
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition1)
{
    std::vector<std::string> files = {"getCompletionsAtPosition1.ets"};
    std::vector<std::string> texts = {R"delimiter(
function num1() {
    return 1;
}

function num2() {
    return 2;
}

let a = n
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 80;  // after 'n' in 'let a = n'
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("num1(): Int", ark::es2panda::lsp::CompletionEntryKind::FUNCTION,
                        std::string(GLOBALS_OR_KEYWORDS), "num1()"),
        CompletionEntry("num2(): Int", ark::es2panda::lsp::CompletionEntryKind::FUNCTION,
                        std::string(GLOBALS_OR_KEYWORDS), "num2()"),
    };
    initializer.DestroyContext(ctx);
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition2)
{
    std::vector<std::string> files = {"getCompletionsAtPosition2.ets"};
    std::vector<std::string> texts = {R"delimiter(
let aaa = 123;
const abb = 333;

function axx() {
    return 444;
}

function foo() {
    let bbb = 222;
    let ccc = bbb + a
    return bbb + ccc;
}
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 127;  // after 'a' in 'let ccc = bbb + a'
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("aaa: Int", ark::es2panda::lsp::CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                        "aaa"),
        CompletionEntry("abb: Int", ark::es2panda::lsp::CompletionEntryKind::CONSTANT, std::string(GLOBALS_OR_KEYWORDS),
                        "abb"),
        CompletionEntry("axx(): Int", ark::es2panda::lsp::CompletionEntryKind::FUNCTION,
                        std::string(GLOBALS_OR_KEYWORDS), "axx()"),
    };
    initializer.DestroyContext(ctx);
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition3)
{
    std::vector<std::string> files = {"getCompletionsAtPosition3.ets"};
    std::vector<std::string> texts = {R"delimiter(
class Foo {
    bar: number = 1;
}

let foo = new Foo();
foo.bar = 2;
let baa = 3;
let bbb = 4;

function bxx() {
    return 5;
}

function fxx() {
    let bcc = 6;
    let axx = b
}
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 181;  // after 'b' in 'let axx = b'
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("baa: Int", ark::es2panda::lsp::CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                        "baa"),
        CompletionEntry("bbb: Int", ark::es2panda::lsp::CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                        "bbb"),
        CompletionEntry("bcc: Int", ark::es2panda::lsp::CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                        "bcc"),
        CompletionEntry("bxx(): Int", ark::es2panda::lsp::CompletionEntryKind::FUNCTION,
                        std::string(GLOBALS_OR_KEYWORDS), "bxx()"),
    };
    auto unexpectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("bar: number", ark::es2panda::lsp::CompletionEntryKind::VARIABLE,
                        std::string(GLOBALS_OR_KEYWORDS), "bar"),
    };
    initializer.DestroyContext(ctx);
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, unexpectedEntries);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition4)
{
    std::vector<std::string> files = {"getCompletionsAtPosition4.ets"};
    std::vector<std::string> texts = {R"delimiter(
class Foo {
    bar: number = 1;
}

let foo = new Foo();
foo.bar = 2;
let baa = 3;
let bbb = 4;

function bxx() {
    let bcc = 6;
    return 5;
}
let axx = b
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 159;  // after 'b' in 'let axx = b'
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("baa: Int", ark::es2panda::lsp::CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                        "baa"),
        CompletionEntry("bbb: Int", ark::es2panda::lsp::CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                        "bbb"),
        CompletionEntry("bxx(): Int", ark::es2panda::lsp::CompletionEntryKind::FUNCTION,
                        std::string(GLOBALS_OR_KEYWORDS), "bxx()"),
    };
    auto unexpectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("bar: number", ark::es2panda::lsp::CompletionEntryKind::PROPERTY,
                        std::string(GLOBALS_OR_KEYWORDS), "bar"),
        CompletionEntry("bcc: Int", ark::es2panda::lsp::CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                        "bcc"),
    };
    initializer.DestroyContext(ctx);
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, unexpectedEntries);
}

TEST_F(LSPCompletionsTests, MemberCompletionsForClassTest1)
{
    std::vector<std::string> files = {"getCompletionsAtPosition5.ets"};
    std::vector<std::string> texts = {R"delimiter(
class MyClass1 {
  public myProp: number = 0;
  public prop: number = 1;
}
let obj1 = new MyClass1()
let prop = obj1.yp)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 120;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    ASSERT_TRUE(entries.size() == 1);

    std::string propertyName1 = "myProp: number";
    initializer.DestroyContext(ctx);
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry(propertyName1, CompletionEntryKind::PROPERTY,
                        std::string(ark::es2panda::lsp::sort_text::SUGGESTED_CLASS_MEMBERS), "myProp"),
    };
    AssertCompletionsContainAndNotContainEntries(entries, expectedEntries, {});
}

TEST_F(LSPCompletionsTests, MemberCompletionsForClassTest2)
{
    std::vector<std::string> files = {"getCompletionsAtPosition6.ets"};
    std::vector<std::string> texts = {R"delimiter(
namespace space {
  export class classInSpace {
    public  c: number = 2;
  }
}
let numOfSpace: space.classi)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 110;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    std::string propertyName1 = "classInSpace";
    ASSERT_TRUE(entries.size() == 1);
    CompletionEntry entry1 = CompletionEntry(
        propertyName1, CompletionEntryKind::CLASS,
        std::string(ark::es2panda::lsp::sort_text::MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), propertyName1);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(entry1, entries[0]);
}

TEST_F(LSPCompletionsTests, MemberCompletionsForClassTest4)
{
    std::vector<std::string> files = {"getCompletionsAtPosition6.ets"};
    std::vector<std::string> texts = {R"delimiter(
enum Color {
  Red = "red",
  Blue = "blue"
}
let myColor: Color = Color.R)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 75;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    ASSERT_TRUE(entries.size() == 1);

    std::string propertyName1 = "Red";
    CompletionEntry entry1 = CompletionEntry(
        propertyName1, CompletionEntryKind::ENUM_MEMBER,
        std::string(ark::es2panda::lsp::sort_text::MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), propertyName1);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(entry1, entries[0]);
}

TEST_F(LSPCompletionsTests, MemberCompletionsForClassOverloadMethods)
{
    std::vector<std::string> files = {"getCompletionsAtPositionOverloadMethods.ets"};
    std::vector<std::string> texts = {R"delimiter(
class AAA {
  foo(a: number) {}
  foo(a: number, b: string) {}
}
let a: AAA = new AAA();
a.fo
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    size_t const offset = texts[0].find("a.fo") + std::string("a.fo").size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("foo(a: number): void", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                        std::string(GLOBALS_OR_KEYWORDS), "foo()"),
        CompletionEntry("foo(a: number, b: string): void", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                        std::string(GLOBALS_OR_KEYWORDS), "foo()"),
    };
    initializer.DestroyContext(ctx);
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
}

TEST_F(LSPCompletionsTests, MemberCompletionsForInterfaceOverloadMethods)
{
    std::vector<std::string> files = {"getCompletionsAtPositionInterfaceOverloadMethods.ets"};
    std::vector<std::string> texts = {R"delimiter(
interface AAA {
  foo(a: number)
  foo(a: number, b: string)
}

function test(a: AAA) {
  a.fo
}
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    size_t const offset = texts[0].find("a.fo") + std::string("a.fo").size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("foo(a: number): void", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                        std::string(GLOBALS_OR_KEYWORDS), "foo()"),
        CompletionEntry("foo(a: number, b: string): void", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                        std::string(GLOBALS_OR_KEYWORDS), "foo()"),
    };
    initializer.DestroyContext(ctx);
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
}

std::vector<std::string> MakeCompletionsAtPositionAnnotationTexts()
{
    return {
        R"(
export @interface Entry {
    routeName: string = "";
    storage: string = "";
}
export @interface TestAnnotation {
    routeName: string = "";
    storage: string = "";
}
)",
        R"(
import { Entry, TestAnnotation } from './CompletionAnnotation1';
export @interface Entry2 {
    routeName: string = "";
    storage: string = "";
}
@E
struct Index {}
@
struct Index1 {}
)"};
}

TEST_F(LSPCompletionsTests, getCompletionsAtPositionAnnotation1)
{
    std::vector<std::string> files = {"CompletionAnnotation1.ets", "CompletionAnnotation2.ets"};
    std::vector<std::string> texts = MakeCompletionsAtPositionAnnotationTexts();
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset1 = 151;
    size_t const offset2 = 169;
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto res1 = lspApi->getCompletionsAtPosition(ctx, offset1);
    auto res2 = lspApi->getCompletionsAtPosition(ctx, offset2);
    auto firstExpectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("Entry", ark::es2panda::lsp::CompletionEntryKind::ANNOTATION,
                                                      std::string(GLOBALS_OR_KEYWORDS), "Entry"),
                                      CompletionEntry("Entry2", ark::es2panda::lsp::CompletionEntryKind::ANNOTATION,
                                                      std::string(GLOBALS_OR_KEYWORDS), "Entry2")};
    auto firstUnexpectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("TestAnnotation", ark::es2panda::lsp::CompletionEntryKind::ANNOTATION,
                        std::string(GLOBALS_OR_KEYWORDS), "TestAnnotation")};
    auto secondExpectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("Entry", ark::es2panda::lsp::CompletionEntryKind::ANNOTATION, std::string(GLOBALS_OR_KEYWORDS),
                        "Entry"),
        CompletionEntry("Entry2", ark::es2panda::lsp::CompletionEntryKind::ANNOTATION, std::string(GLOBALS_OR_KEYWORDS),
                        "Entry2"),
        CompletionEntry("TestAnnotation", ark::es2panda::lsp::CompletionEntryKind::ANNOTATION,
                        std::string(GLOBALS_OR_KEYWORDS), "TestAnnotation")};
    AssertCompletionsContainAndNotContainEntries(res1.GetEntries(), firstExpectedEntries, firstUnexpectedEntries);
    AssertCompletionsContainAndNotContainEntries(res2.GetEntries(), secondExpectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, MemberCompletionsForClassTest5)
{
    std::vector<std::string> files = {"getCompletionsAtPosition6.ets"};
    std::vector<std::string> texts = {R"delimiter(
class A {
}
let a = new A();
class B {
}
cla
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 45;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    std::string propertyName1 = "class";
    CompletionEntry entry1 =
        CompletionEntry(propertyName1, CompletionEntryKind::KEYWORD,
                        std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), propertyName1);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(entry1, entries[0]);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition1)
{
    std::vector<std::string> files = {"getApiCompletionsAtPosition1.ets"};
    const std::string text = R"delimiter(
interface I {
    str: string;
    b: boolean;
}
I.
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    const std::string marker = "I.";
    auto markerPos = text.find(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto unexpectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("str: string", ark::es2panda::lsp::CompletionEntryKind::PROPERTY,
                        std::string(CLASS_MEMBER_SNIPPETS), "str", std::nullopt, "string"),
        CompletionEntry("b: boolean", ark::es2panda::lsp::CompletionEntryKind::PROPERTY,
                        std::string(CLASS_MEMBER_SNIPPETS), "b", std::nullopt, "boolean")};
    AssertCompletionsNotContainEntries(res.GetEntries(), unexpectedEntries);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition2)
{
    std::vector<std::string> files = {"getApiCompletionsAtPosition2.ets"};
    const std::string text = R"delimiter(
class C {
    private hidden(): void {}
    show(): void {}
}
let c: C = new C();
c.
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    const std::string marker = "c.";
    auto markerPos = text.find(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "show(): void", ark::es2panda::lsp::CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS), "show()")};
    auto unexpectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("hidden(): void", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "hidden()")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, unexpectedEntries);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition3)
{
    std::vector<std::string> files = {"getApiCompletionsAtPosition3.ets"};
    const std::string text = R"delimiter(
class C {
    static make() {}
    private static make1() {}
}
C.
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    const std::string marker = "C.";
    auto markerPos = text.find(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "make(): void", ark::es2panda::lsp::CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS), "make()")};
    auto unexpectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("make1(): void", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "make1()")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, unexpectedEntries);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition4)
{
    std::vector<std::string> files = {"getApiCompletionsAtPosition4.ets"};
    const std::string text = R"delimiter(
class C {
    private show(): void {}
    constructor() {
        this.
    }
}
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    const std::string marker = "this.";
    auto markerPos = text.find(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "show(): void", ark::es2panda::lsp::CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS), "show()")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition5)
{
    std::vector<std::string> files = {"getApiCompletionsAtPosition5.ets"};
    const std::string text = R"delimiter(
class C {
    private static show(): void {}
    foo(): void {
        C.
    }
}
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    const std::string marker = "C.";
    auto markerPos = text.find(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "show(): void", ark::es2panda::lsp::CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS), "show()")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition6)
{
    std::vector<std::string> files = {"getApiCompletionsAtPosition6.ets"};
    const std::string text = R"delimiter(
class C {
    private secret(): void {}
    expose(other: C): void {
        other.
    }
}
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    const std::string marker = "other.";
    auto markerPos = text.find(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("secret(): void", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "secret()")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition7)
{
    std::vector<std::string> files = {"getApiCompletionsAtPosition7.ets"};
    const std::string text = R"delimiter(
class Base {
    private baseOnly(): void {}
}
class Derived extends Base {
    own(): void {}
    check(): void {
        this.
    }
}
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    const std::string marker = "this.";
    auto markerPos = text.find(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "own(): void", ark::es2panda::lsp::CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS), "own()")};
    auto unexpectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("baseOnly(): void", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                        std::string(CLASS_MEMBER_SNIPPETS), "baseOnly()")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, unexpectedEntries);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition8)
{
    std::vector<std::string> files = {"getApiCompletionsAtPosition8.ets"};
    const std::string text = R"delimiter(
class Base {
    protected prot(): void {}
}
class Derived extends Base {
    checkThis(): void {
        this.
    }
}
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string markerThis = "this.";
    auto markerThisPos = text.find(markerThis);
    ASSERT_NE(markerThisPos, std::string::npos);
    size_t const offsetThis = markerThisPos + markerThis.size();
    auto resThis = lspApi->getCompletionsAtPosition(ctx, offsetThis);
    auto expectedThisEntries = std::vector<CompletionEntry> {CompletionEntry(
        "prot(): void", ark::es2panda::lsp::CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS), "prot()")};
    AssertCompletionsContainAndNotContainEntries(resThis.GetEntries(), expectedThisEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition9)
{
    std::vector<std::string> files = {"getApiCompletionsAtPosition9.ets"};
    const std::string text = R"delimiter(
class Base {
    protected prot(): void {}
}
class Derived extends Base {
    checkThis(): void {
    }
}
let d: Derived = new Derived();
d.
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string markerD = "d.";
    auto markerDPos = text.rfind(markerD);
    ASSERT_NE(markerDPos, std::string::npos);
    size_t const offsetD = markerDPos + markerD.size();
    auto resD = lspApi->getCompletionsAtPosition(ctx, offsetD);
    auto unexpectedDEntries = std::vector<CompletionEntry> {CompletionEntry(
        "prot(): void", ark::es2panda::lsp::CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS), "prot()")};
    AssertCompletionsNotContainEntries(resD.GetEntries(), unexpectedDEntries);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition10)
{
    std::vector<std::string> files = {"getApiCompletionsAtPosition10.ets"};
    const std::string text = R"delimiter(
class innn {}
let a: in
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string markerD = "in";
    auto markerDPos = text.rfind(markerD);
    ASSERT_NE(markerDPos, std::string::npos);
    size_t const offsetD = markerDPos + markerD.size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offsetD);
    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("int", ark::es2panda::lsp::CompletionEntryKind::KEYWORD,
                                                      std::string(GLOBALS_OR_KEYWORDS), "int"),
                                      CompletionEntry("innn", ark::es2panda::lsp::CompletionEntryKind::MODULE,
                                                      std::string(GLOBALS_OR_KEYWORDS), "innn")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition11)
{
    std::vector<std::string> files = {"getApiCompletionsAtPosition11.ets"};
    const std::string text = R"delimiter(
interface MyInterface {
    a: int;
}
My
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string markerD = "My";
    auto markerDPos = text.rfind(markerD);
    ASSERT_NE(markerDPos, std::string::npos);
    size_t const offsetD = markerDPos + markerD.size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offsetD);
    auto unexpectedDEntries =
        std::vector<CompletionEntry> {CompletionEntry("MyInterface", ark::es2panda::lsp::CompletionEntryKind::KEYWORD,
                                                      std::string(SUGGESTED_CLASS_MEMBERS), "MyInterface")};
    AssertCompletionsNotContainEntries(res.GetEntries(), unexpectedDEntries);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition12)
{
    std::vector<std::string> files = {"getApiCompletionsAtPosition12.ets"};
    const std::string text = R"delimiter(
interface MyInterface {
    a: int;
}
let x: My
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string markerD = "My";
    auto markerDPos = text.rfind(markerD);
    ASSERT_NE(markerDPos, std::string::npos);
    size_t const offsetD = markerDPos + markerD.size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offsetD);
    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("MyInterface", ark::es2panda::lsp::CompletionEntryKind::KEYWORD,
                                                      std::string(SUGGESTED_CLASS_MEMBERS), "MyInterface")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition13)
{
    std::vector<std::string> files = {"getApiCompletionsAtPosition13.ets"};
    const std::string text = R"delimiter(
interface MyInterface {
    a: int;
}
interface Host {
    p: My
}
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string marker = "p: My";
    auto markerPos = text.find(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("MyInterface", ark::es2panda::lsp::CompletionEntryKind::KEYWORD,
                                                      std::string(SUGGESTED_CLASS_MEMBERS), "MyInterface")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition14)
{
    std::vector<std::string> files = {"getApiCompletionsAtPosition14.ets"};
    const std::string text = R"delimiter(
interface MyInterface {
    a: int;
}
class Box<T extends My> {}
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const std::string marker = "extends My";
    auto markerPos = text.find(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("MyInterface", ark::es2panda::lsp::CompletionEntryKind::KEYWORD,
                                                      std::string(SUGGESTED_CLASS_MEMBERS), "MyInterface")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition27)
{
    std::vector<std::string> files = {"getCompletionsAtPosition27.ets"};
    const std::string text = R"delimiter(
@ohos.
class A {}
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    auto markerPos = text.find("@ohos.");
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + std::string("@ohos.").size();

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    ASSERT_TRUE(res.GetEntries().empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getCompletionsAtPosition28)
{
    std::vector<std::string> files = {"getCompletionsAtPosition29.ets", "getCompletionsAtPosition30.ets"};
    std::vector<std::string> texts = {R"(
export function foo(): void {}
)",
                                      R"(
import { fo } from './getCompletionsAtPosition29'
exp
)"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    auto markerPos = texts[1].find("fo");
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + std::string("fo").size();

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "foo(): void", ark::es2panda::lsp::CompletionEntryKind::METHOD, std::string(GLOBALS_OR_KEYWORDS), "foo")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition29)
{
    std::vector<std::string> apiFiles = {"test291.ets", "test292.ets"};
    std::vector<std::string> texts = {R"(
export function foo1(): string {
    return "1";
}
export function foo2(): string {
    return "1";
}
)",
                                      R"(
'use static'
import {foo1, } from './test291'
)"};
    auto filePaths = CreateTempFile(apiFiles, texts);
    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto collected = lspApi->collectApiInfo(ctx);
    ASSERT_TRUE(collected);

    auto markerPos = texts[1].find("foo1, ");
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + std::string("foo1, ").size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "foo2(): string", ark::es2panda::lsp::CompletionEntryKind::METHOD, std::string(GLOBALS_OR_KEYWORDS), "foo2")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition30)
{
    std::vector<std::string> apiFiles = {"test301.ets", "test302.ets"};
    std::vector<std::string> texts = {R"(
function foo1(): string {
    return "1";
}
export function foo2(): string {
    return "1";
}
export class foo3 {}
)",
                                      R"(
foo
)"};
    auto filePaths = CreateTempFile(apiFiles, texts);
    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx0 = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx1 = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    lspApi->buildSymbolReferenceIndexForContextWithExternal(ctx0);
    lspApi->buildSymbolReferenceIndexForContextWithExternal(ctx1);
    auto collected = lspApi->collectApiInfo(ctx1);
    ASSERT_TRUE(collected);

    auto markerPos = texts[1].find("foo");
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + std::string("foo").size();
    auto res = lspApi->getCompletionsAtPosition(ctx1, offset);
    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("foo2(): string", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                                                      std::string(GLOBALS_OR_KEYWORDS), "foo2()"),
                                      CompletionEntry("foo3", ark::es2panda::lsp::CompletionEntryKind::CLASS,
                                                      std::string(AUTO_IMPORT_SUGGESTIONS), "foo3")};
    auto unexpectedEntries = std::vector<CompletionEntry> {CompletionEntry(
        "foo1(): string", ark::es2panda::lsp::CompletionEntryKind::METHOD, std::string(GLOBALS_OR_KEYWORDS), "foo1()")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, unexpectedEntries);
    initializer.DestroyContext(ctx0);
    initializer.DestroyContext(ctx1);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition31)
{
    std::vector<std::string> apiFiles = {"test311.ets", "test312.ets"};
    std::vector<std::string> texts = {R"(
export function foo1(): void {
}
export function foo2(): undifined {
}
)",
                                      R"(
import {foo1, foo2} from './test311';
foo
)"};
    auto filePaths = CreateTempFile(apiFiles, texts);
    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto collected = lspApi->collectApiInfo(ctx);
    ASSERT_TRUE(collected);

    size_t const offset = 42;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("foo1(): void", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                        std::string(GLOBALS_OR_KEYWORDS), "foo1()"),
        CompletionEntry("foo2(): undifined", ark::es2panda::lsp::CompletionEntryKind::METHOD,
                        std::string(GLOBALS_OR_KEYWORDS), "foo2()")};
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCompletionsTests, getApiCompletionsAtPosition32)
{
    std::vector<std::string> files = {"getApiCompletionsAtPosition32.ets"};
    std::vector<std::string> texts = {R"delimiter(
let data: number[] = [10, 20, 30, 40, 50];
let indexxxx: number = 2;
let value = /*start*/data.indexOf(indexx)/*end*/;
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = 110;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("indexxxx: number", CompletionEntryKind::VARIABLE,
                        std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "indexxxx")};
    initializer.DestroyContext(ctx);
    AssertCompletionsContainAndNotContainEntries(res.GetEntries(), expectedEntries, {});
}

}  // namespace
