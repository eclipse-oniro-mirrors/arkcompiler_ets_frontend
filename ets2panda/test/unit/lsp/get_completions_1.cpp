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

#include "lsp_api_test.h"
#include "lsp/include/completions.h"
#include "lsp/include/internal_api.h"

class LSPCompletionsTests : public LSPAPITests {};

using ark::es2panda::lsp::CompletionEntry;
using ark::es2panda::lsp::CompletionEntryKind;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS;

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
    int const expectedPropertyCount = 2;
    ASSERT_TRUE(entries.size() == expectedPropertyCount);
    CompletionEntry entry1 = CompletionEntry(propertyName1, CompletionEntryKind::METHOD,
                                             std::string(ark::es2panda::lsp::sort_text::CLASS_MEMBER_SNIPPETS), "F()");
    CompletionEntry entry2 = CompletionEntry(propertyName2, CompletionEntryKind::INTERFACE,
                                             std::string(ark::es2panda::lsp::sort_text::SUGGESTED_CLASS_MEMBERS));
    initializer.DestroyContext(ctx);
    ASSERT_EQ(entry1, entries[0]);
    ASSERT_EQ(entry2, entries[1]);
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
        CompletionEntry("num1", ark::es2panda::lsp::CompletionEntryKind::FUNCTION, std::string(GLOBALS_OR_KEYWORDS),
                        "num1()"),
        CompletionEntry("num2", ark::es2panda::lsp::CompletionEntryKind::FUNCTION, std::string(GLOBALS_OR_KEYWORDS),
                        "num2()"),
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
        CompletionEntry("num1", ark::es2panda::lsp::CompletionEntryKind::FUNCTION, std::string(GLOBALS_OR_KEYWORDS),
                        "num1()"),
        CompletionEntry("num2", ark::es2panda::lsp::CompletionEntryKind::FUNCTION, std::string(GLOBALS_OR_KEYWORDS),
                        "num2()"),
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
        CompletionEntry("aaa", ark::es2panda::lsp::CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                        "aaa"),
        CompletionEntry("abb", ark::es2panda::lsp::CompletionEntryKind::CONSTANT, std::string(GLOBALS_OR_KEYWORDS),
                        "abb"),
        CompletionEntry("axx", ark::es2panda::lsp::CompletionEntryKind::FUNCTION, std::string(GLOBALS_OR_KEYWORDS),
                        "axx()"),
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
        CompletionEntry("baa", ark::es2panda::lsp::CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                        "baa"),
        CompletionEntry("bbb", ark::es2panda::lsp::CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                        "bbb"),
        CompletionEntry("bcc", ark::es2panda::lsp::CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                        "bcc"),
        CompletionEntry("bxx", ark::es2panda::lsp::CompletionEntryKind::FUNCTION, std::string(GLOBALS_OR_KEYWORDS),
                        "bxx()"),
    };
    auto unexpectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("bar", ark::es2panda::lsp::CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                        "bar"),
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
        CompletionEntry("baa", ark::es2panda::lsp::CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                        "baa"),
        CompletionEntry("bbb", ark::es2panda::lsp::CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                        "bbb"),
        CompletionEntry("bxx", ark::es2panda::lsp::CompletionEntryKind::FUNCTION, std::string(GLOBALS_OR_KEYWORDS),
                        "bxx()"),
    };
    auto unexpectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("bar", ark::es2panda::lsp::CompletionEntryKind::PROPERTY, std::string(GLOBALS_OR_KEYWORDS),
                        "bar"),
        CompletionEntry("bcc", ark::es2panda::lsp::CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
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

    std::string propertyName1 = "myProp";
    CompletionEntry entry1 =
        CompletionEntry(propertyName1, CompletionEntryKind::PROPERTY,
                        std::string(ark::es2panda::lsp::sort_text::SUGGESTED_CLASS_MEMBERS), propertyName1);
    initializer.DestroyContext(ctx);
    ASSERT_EQ(entry1, entries[0]);
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
}  // namespace