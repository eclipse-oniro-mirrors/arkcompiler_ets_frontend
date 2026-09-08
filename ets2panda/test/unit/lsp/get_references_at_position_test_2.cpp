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

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/references.h"
#include "lsp/include/symbol_reference_index.h"
#include "public/es2panda_lib.h"

using ark::es2panda::lsp::Initializer;

class LSPGetReferencesAtPositionTests2 : public LSPAPITests {
public:
    LSPGetReferencesAtPositionTests2() = default;
    ~LSPGetReferencesAtPositionTests2() override = default;

    NO_COPY_SEMANTIC(LSPGetReferencesAtPositionTests2);
    NO_MOVE_SEMANTIC(LSPGetReferencesAtPositionTests2);

    void SetUp() override
    {
        LSPAPITests::SetUp();
        // Ensure a clean global index state before every test case.
        ark::es2panda::lsp::InitSymbolReferenceIndex();
    }

    void TearDown() override
    {
        // Clean global state to avoid leaking index entries between test cases.
        ark::es2panda::lsp::ClearSymbolReferenceIndex();
        LSPAPITests::TearDown();
    }

    // Non-index path: resolve the declaration of the identifier at `position` in `fileName`,
    // then collect references from every file in `filePaths`. The declaration occurrence
    // itself is excluded by implementation policy (see GetReferencesAtPositionImpl).
    References MockGetReferencesAtPosition(char const *fileName, size_t position,
                                           const std::vector<std::string> &filePaths)
    {
        Initializer initializer = Initializer();
        auto context = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED);
        auto astNode = ark::es2panda::lsp::GetTouchingToken(context, position, false);
        auto declInfo = ark::es2panda::lsp::GetDeclInfoImpl(astNode);
        initializer.DestroyContext(context);

        References result {};
        for (auto const &file : filePaths) {
            auto fileContext = initializer.CreateContext(file.c_str(), ES2PANDA_STATE_CHECKED);
            auto refInfo = ark::es2panda::lsp::GetReferencesAtPositionImpl(fileContext, declInfo);
            result.referenceInfos.insert(result.referenceInfos.end(), refInfo.referenceInfos.begin(),
                                         refInfo.referenceInfos.end());
            initializer.DestroyContext(fileContext);
        }
        auto comp = [](const ReferenceInfo &lhs, const ReferenceInfo &rhs) {
            if (lhs.fileName != rhs.fileName) {
                return lhs.fileName < rhs.fileName;
            }
            if (lhs.start != rhs.start) {
                return lhs.start < rhs.start;
            }
            return lhs.length < rhs.length;
        };
        ark::es2panda::lsp::RemoveDuplicates(result.referenceInfos, comp);
        return result;
    }

    static void AssertRefs(const References &result, const std::vector<ReferenceInfo> &expected)
    {
        ASSERT_EQ(result.referenceInfos.size(), expected.size()) << DumpRefs(result);
        for (size_t i = 0; i < expected.size(); i++) {
            EXPECT_EQ(result.referenceInfos[i].fileName, expected[i].fileName) << "index " << i;
            EXPECT_EQ(result.referenceInfos[i].start, expected[i].start) << "index " << i;
            EXPECT_EQ(result.referenceInfos[i].length, expected[i].length) << "index " << i;
        }
    }

    static std::string DumpRefs(const References &result)
    {
        std::string dump = "definitionInfo={" + result.definitionInfo.fileName + "," +
                           std::to_string(result.definitionInfo.start) + "," +
                           std::to_string(result.definitionInfo.length) + "} refs=[";
        for (const auto &ref : result.referenceInfos) {
            dump += "{" + ref.fileName + "," + std::to_string(ref.start) + "," + std::to_string(ref.length) + "},";
        }
        dump += "]";
        return dump;
    }

    static bool ContainsRef(const References &result, const std::string &fileName, size_t start, size_t length)
    {
        return std::any_of(result.referenceInfos.begin(), result.referenceInfos.end(),
                           [&fileName, start, length](const ReferenceInfo &ref) {
                               return ref.fileName == fileName && ref.start == start && ref.length == length;
                           });
    }
};

// Scenario: declaration + read/write references.
// "num" is written in the declaration initializer, read in foo(), and assigned at top level.
// The declaration occurrence itself is excluded by policy; the other 3 occurrences are references.
TEST_F(LSPGetReferencesAtPositionTests2, DeclarationAndReadWriteReferences)
{
    std::vector<std::string> files = {"decl_rw_refs.ets"};
    std::vector<std::string> texts = {R"(let num: number = 1;
function foo(): number {
    return num;
}
num = num + 1;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const size_t defPos = texts[0].find("num: number");
    ASSERT_NE(defPos, std::string::npos);
    References result = MockGetReferencesAtPosition(filePaths[0].c_str(), defPos, filePaths);

    const size_t readInFoo = texts[0].find("num;");
    ASSERT_NE(readInFoo, std::string::npos);
    const size_t writePos = texts[0].rfind("num = num");
    ASSERT_NE(writePos, std::string::npos);
    const size_t readAfterWrite = writePos + std::string("num = ").size();

    std::vector<ReferenceInfo> expected {
        {filePaths[0], readInFoo, 3}, {filePaths[0], writePos, 3}, {filePaths[0], readAfterWrite, 3}};
    AssertRefs(result, expected);
}

// Scenario: the include-definition policy is fixed - the declaration occurrence is
// excluded from referenceInfos no matter whether the query starts at the declaration
// or at a usage of the same symbol.
TEST_F(LSPGetReferencesAtPositionTests2, IncludeDefinitionPolicyIsFixed)
{
    std::vector<std::string> files = {"def_policy.ets"};
    std::vector<std::string> texts = {R"(let num: number = 1;
function foo(): number {
    return num;
}
console.log(num);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const size_t defPos = texts[0].find("num: number");
    const size_t usePos = texts[0].rfind("num)");
    ASSERT_NE(defPos, std::string::npos);
    ASSERT_NE(usePos, std::string::npos);

    References fromDef = MockGetReferencesAtPosition(filePaths[0].c_str(), defPos, filePaths);
    References fromUse = MockGetReferencesAtPosition(filePaths[0].c_str(), usePos, filePaths);

    // Same reference set regardless of the queried occurrence.
    AssertRefs(fromUse, fromDef.referenceInfos);
    // The declaration occurrence itself must not appear among the references.
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t numNameLength = 3;
    EXPECT_FALSE(ContainsRef(fromDef, filePaths[0], defPos, numNameLength));
    // Non-index path does not populate definitionInfo.
    EXPECT_TRUE(fromDef.definitionInfo.fileName.empty());
}

// Scenario: references to a class member through this.foo and obj.foo.
TEST_F(LSPGetReferencesAtPositionTests2, ClassMemberViaThisAndObject)
{
    std::vector<std::string> files = {"member_this_obj.ets"};
    std::vector<std::string> texts = {R"(class Counter {
    count: number = 0;
    bump(): void {
        this.count = this.count + 1;
    }
}
let c = new Counter();
c.count = 5;
console.log(c.count);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const size_t defPos = texts[0].find("count: number");
    ASSERT_NE(defPos, std::string::npos);
    References result = MockGetReferencesAtPosition(filePaths[0].c_str(), defPos, filePaths);

    const size_t thisWrite = texts[0].find("count = this");
    const size_t thisRead = texts[0].rfind("count + 1");
    const size_t objWrite = texts[0].rfind("count = 5");
    const size_t objRead = texts[0].rfind("count);");
    ASSERT_NE(thisWrite, std::string::npos);
    ASSERT_NE(thisRead, std::string::npos);
    ASSERT_NE(objWrite, std::string::npos);
    ASSERT_NE(objRead, std::string::npos);

    // this.count (write + read) and c.count (write + read) all resolve to the member.
    std::vector<ReferenceInfo> expected {{filePaths[0], thisWrite, 5},
                                         {filePaths[0], thisRead, 5},
                                         {filePaths[0], objWrite, 5},
                                         {filePaths[0], objRead, 5}};
    AssertRefs(result, expected);
}

// Scenario: references to a member of a struct type via variable access and struct literal.
TEST_F(LSPGetReferencesAtPositionTests2, StructTypeMemberReferences)
{
    std::vector<std::string> files = {"struct_member_refs.ets"};
    std::vector<std::string> texts = {R"(struct Point {
    x: number = 0;
    y: number = 0;
}
let p = new Point();
p.x = 3;
console.log(p.x);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const size_t defPos = texts[0].find("x: number");
    ASSERT_NE(defPos, std::string::npos);
    References result = MockGetReferencesAtPosition(filePaths[0].c_str(), defPos, filePaths);

    const size_t writePos = texts[0].find("x = 3");
    const size_t readPos = texts[0].rfind("x);");
    ASSERT_NE(writePos, std::string::npos);
    ASSERT_NE(readPos, std::string::npos);

    std::vector<ReferenceInfo> expected {{filePaths[0], writePos, 1}, {filePaths[0], readPos, 1}};
    AssertRefs(result, expected);
    // The sibling member "y" must not leak into the reference set of "x".
    const size_t siblingDecl = texts[0].find("y: number");
    ASSERT_NE(siblingDecl, std::string::npos);
    EXPECT_FALSE(ContainsRef(result, filePaths[0], siblingDecl, 1));
}

// Scenario: a method call and a same-named property access on an unrelated type must
// resolve to different symbols, and each query returns exactly the references of its own symbol.
TEST_F(LSPGetReferencesAtPositionTests2, MethodCallVsSameNamedPropertyAccess)
{
    std::vector<std::string> files = {"method_vs_property2.ets"};
    std::vector<std::string> texts = {R"(class Service {
    run(): void {
        this.run();
    }
}
class Task {
    run: number = 0;
}
function start(s: Service, t: Task): void {
    s.run();
    t.run = 1;
    console.log(t.run);
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const size_t methodPos = texts[0].find("run(): void");
    ASSERT_NE(methodPos, std::string::npos);
    References methodRefs = MockGetReferencesAtPosition(filePaths[0].c_str(), methodPos, filePaths);

    const size_t thisCall = texts[0].find("run();");
    const size_t externalCall = texts[0].rfind("run();");
    ASSERT_NE(thisCall, std::string::npos);
    ASSERT_NE(externalCall, std::string::npos);
    ASSERT_NE(externalCall, thisCall);
    // The method symbol: this.run() inside the class and s.run() outside.
    std::vector<ReferenceInfo> expectedMethod {{filePaths[0], thisCall, 3}, {filePaths[0], externalCall, 3}};
    AssertRefs(methodRefs, expectedMethod);

    const size_t propertyPos = texts[0].find("run: number");
    ASSERT_NE(propertyPos, std::string::npos);
    References propertyRefs = MockGetReferencesAtPosition(filePaths[0].c_str(), propertyPos, filePaths);

    const size_t propWrite = texts[0].find("run = 1");
    const size_t propRead = texts[0].rfind("run);");
    ASSERT_NE(propWrite, std::string::npos);
    ASSERT_NE(propRead, std::string::npos);
    // The same-named property on Task: write and read, no method call mixed in.
    std::vector<ReferenceInfo> expectedProperty {{filePaths[0], propWrite, 3}, {filePaths[0], propRead, 3}};
    AssertRefs(propertyRefs, expectedProperty);
}

// Scenario: import alias references are exact - the alias specifier and every aliased
// usage resolve to the alias declaration, the original name only appears inside the import.
TEST_F(LSPGetReferencesAtPositionTests2, ImportAliasReferencesExact)
{
    std::vector<std::string> files = {"alias_exact_export.ets", "alias_exact_use.ets"};
    std::vector<std::string> texts = {R"(export function compute(): number {
    return 1;
})",
                                      R"(import { compute as calc } from './alias_exact_export';
let result = calc();
console.log(calc);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const size_t aliasDef = texts[1].find("calc }");
    ASSERT_NE(aliasDef, std::string::npos);
    const size_t aliasCall = texts[1].find("calc()");
    ASSERT_NE(aliasCall, std::string::npos);
    const size_t aliasLog = texts[1].rfind("calc)");
    ASSERT_NE(aliasLog, std::string::npos);

    References result = MockGetReferencesAtPosition(filePaths[1].c_str(), aliasCall, filePaths);

    // The implementation resolves each identifier to the alias declaration owner (the
    // whole import statement), so the original name inside the import specifier also
    // resolves to the same declaration and is counted as a reference.
    const size_t originalInImport = texts[1].find("compute as");
    ASSERT_NE(originalInImport, std::string::npos);
    std::vector<ReferenceInfo> expected {{filePaths[1], originalInImport, 7},
                                         {filePaths[1], aliasDef, 4},
                                         {filePaths[1], aliasCall, 4},
                                         {filePaths[1], aliasLog, 4}};
    AssertRefs(result, expected);
    // The exported declaration itself in the export file must not be reported.
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t computeNameLength = 7;
    const size_t exportDecl = texts[0].find("compute()");
    ASSERT_NE(exportDecl, std::string::npos);
    EXPECT_FALSE(ContainsRef(result, filePaths[0], exportDecl, computeNameLength));
}

// Scenario: export alias references - querying the imported alias in the consuming file
// resolves through the "export { alpha as beta }" chain to the original declaration, so
// exactly the usages of the alias in the consuming file are reported.
TEST_F(LSPGetReferencesAtPositionTests2, ExportAliasReferencesExact)
{
    std::vector<std::string> files = {"exp_alias_decl.ets", "exp_alias_consume.ets"};
    std::vector<std::string> texts = {R"(function alpha(): number {
    return 1;
}
export { alpha as beta };)",
                                      R"(import { beta } from './exp_alias_decl';
beta();
console.log(beta);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // Query at the alias usage in the consuming file.
    const size_t callPos = texts[1].find("beta();");
    ASSERT_NE(callPos, std::string::npos);
    References result = MockGetReferencesAtPosition(filePaths[1].c_str(), callPos, filePaths);

    // The import specifier and the two usages resolve to the declaration of "alpha".
    const size_t importSpec = texts[1].find("beta }");
    const size_t logPos = texts[1].rfind("beta);");
    ASSERT_NE(importSpec, std::string::npos);
    ASSERT_NE(logPos, std::string::npos);
    std::vector<ReferenceInfo> expected {
        {filePaths[1], importSpec, 4}, {filePaths[1], callPos, 4}, {filePaths[1], logPos, 4}};
    AssertRefs(result, expected);
    // The export alias specifier itself and the original declaration are declaration-side
    // occurrences and must not be reported as references.
    EXPECT_TRUE(std::none_of(result.referenceInfos.begin(), result.referenceInfos.end(),
                             [&filePaths](const ReferenceInfo &ref) { return ref.fileName == filePaths[0]; }));
}

// Scenario: re-export chain - references to a re-exported function resolve across the
// source file, the middle re-export file, and the consuming file.
TEST_F(LSPGetReferencesAtPositionTests2, ReExportChainReferences)
{
    std::vector<std::string> files = {"reexp_src.ets", "reexp_mid.ets", "reexp_use.ets"};
    std::vector<std::string> texts = {R"(export function core(): number {
    return 42;
})",
                                      R"(export { core } from './reexp_src';)",
                                      R"(import { core } from './reexp_mid';
core();
console.log(core);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // Query at the import specifier in the consuming file.
    const size_t importSpec = texts[2].find("core }");
    ASSERT_NE(importSpec, std::string::npos);
    References result = MockGetReferencesAtPosition(filePaths[2].c_str(), importSpec, filePaths);

    // The whole re-export chain resolves to the declaration of "core" in the source
    // file, but the non-index path only reports non-declaration occurrences, which all
    // live in the consuming file: the import specifier and the two usages.
    const size_t callPos = texts[2].find("core();");
    const size_t logPos = texts[2].rfind("core);");
    ASSERT_NE(callPos, std::string::npos);
    ASSERT_NE(logPos, std::string::npos);
    std::vector<ReferenceInfo> expected {
        {filePaths[2], importSpec, 4}, {filePaths[2], callPos, 4}, {filePaths[2], logPos, 4}};
    AssertRefs(result, expected);
    // The declaration in the source file and the re-export specifier in the middle file
    // are declaration-side occurrences and must not be reported.
    const size_t srcDecl = texts[0].find("core(): number");
    ASSERT_NE(srcDecl, std::string::npos);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t coreNameLength = 4;
    EXPECT_FALSE(ContainsRef(result, filePaths[0], srcDecl, coreNameLength));
    EXPECT_TRUE(std::none_of(result.referenceInfos.begin(), result.referenceInfos.end(),
                             [&filePaths](const ReferenceInfo &ref) { return ref.fileName == filePaths[1]; }));
}

// Scenario: same name inside a string literal and a comment must not be counted;
// only the real usages are references (the declaration is excluded by policy).
TEST_F(LSPGetReferencesAtPositionTests2, StringAndCommentNotCountedExactly)
{
    std::vector<std::string> files = {"str_comment_refs.ets"};
    std::vector<std::string> texts = {R"(// token appears here
let token: number = 1;
let label: string = "token";
/* token again */
console.log(token);
token = 2;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const size_t defPos = texts[0].find("token: number");
    ASSERT_NE(defPos, std::string::npos);
    References result = MockGetReferencesAtPosition(filePaths[0].c_str(), defPos, filePaths);

    const size_t logPos = texts[0].find("token)");
    ASSERT_NE(logPos, std::string::npos);
    const size_t writePos = texts[0].rfind("token = 2");
    ASSERT_NE(writePos, std::string::npos);

    std::vector<ReferenceInfo> expected {{filePaths[0], logPos, 5}, {filePaths[0], writePos, 5}};
    AssertRefs(result, expected);

    // Explicitly verify the comment and string literal occurrences are absent.
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t tokenNameLength = 5;
    const size_t commentPos = texts[0].find("token appears");
    const size_t stringPos = texts[0].find("\"token\"") + 1;
    const size_t blockCommentPos = texts[0].find("token again");
    EXPECT_FALSE(ContainsRef(result, filePaths[0], commentPos, tokenNameLength));
    EXPECT_FALSE(ContainsRef(result, filePaths[0], stringPos, tokenNameLength));
    EXPECT_FALSE(ContainsRef(result, filePaths[0], blockCommentPos, tokenNameLength));
}

// Scenario: shadowing must not cross - the shadowed outer variable and the inner
// variable each keep their own exact reference sets.
TEST_F(LSPGetReferencesAtPositionTests2, ShadowingDoesNotCrossScopes)
{
    std::vector<std::string> files = {"shadow_refs.ets"};
    std::vector<std::string> texts = {R"(let val: number = 1;
function outer(): void {
    let val: string = "inner";
    console.log(val);
}
console.log(val);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const size_t innerUse = texts[0].find("val);");
    ASSERT_NE(innerUse, std::string::npos);
    References innerRefs = MockGetReferencesAtPosition(filePaths[0].c_str(), innerUse, filePaths);
    // The inner variable is only used once inside outer(); with its declaration
    // excluded, exactly the single queried usage remains.
    std::vector<ReferenceInfo> expectedInner {{filePaths[0], innerUse, 3}};
    AssertRefs(innerRefs, expectedInner);

    const size_t outerUse = texts[0].rfind("val);");
    ASSERT_NE(outerUse, std::string::npos);
    ASSERT_NE(outerUse, innerUse);
    References outerRefs = MockGetReferencesAtPosition(filePaths[0].c_str(), outerUse, filePaths);
    // The outer variable is only used once at top level; the shadowed inner usage
    // must not appear in its reference set.
    std::vector<ReferenceInfo> expectedOuter {{filePaths[0], outerUse, 3}};
    AssertRefs(outerRefs, expectedOuter);
}

// Scenario: same-named symbols in unrelated files must not be cross-referenced.
TEST_F(LSPGetReferencesAtPositionTests2, MultiFileSameNameSymbolsDoNotCross)
{
    std::vector<std::string> files = {"same_name_a.ets", "same_name_b.ets"};
    std::vector<std::string> texts = {R"(export let shared: number = 1;
console.log(shared);)",
                                      R"(let shared: string = "local";
console.log(shared);
shared = "again";)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // Query the exported "shared" in file A: only file A references are returned.
    const size_t posA = texts[0].find("shared: number");
    ASSERT_NE(posA, std::string::npos);
    References resultA = MockGetReferencesAtPosition(filePaths[0].c_str(), posA, filePaths);

    const size_t logPosA = texts[0].find("shared)");
    ASSERT_NE(logPosA, std::string::npos);
    std::vector<ReferenceInfo> expectedA {{filePaths[0], logPosA, 6}};
    AssertRefs(resultA, expectedA);

    // Query the local "shared" in file B: only file B references are returned.
    const size_t posB = texts[1].find("shared: string");
    ASSERT_NE(posB, std::string::npos);
    References resultB = MockGetReferencesAtPosition(filePaths[1].c_str(), posB, filePaths);

    const size_t logPosB = texts[1].find("shared)");
    ASSERT_NE(logPosB, std::string::npos);
    const size_t writePosB = texts[1].rfind("shared =");
    ASSERT_NE(writePosB, std::string::npos);
    std::vector<ReferenceInfo> expectedB {{filePaths[1], logPosB, 6}, {filePaths[1], writePosB, 6}};
    AssertRefs(resultB, expectedB);
}

// Scenario: references stay stable after the context is destroyed and recreated.
TEST_F(LSPGetReferencesAtPositionTests2, ReferencesStableAfterContextRecreation)
{
    std::vector<std::string> files = {"stable_refs.ets"};
    std::vector<std::string> texts = {R"(let stable: number = 7;
function readIt(): number {
    return stable;
}
stable = stable + 1;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const size_t defPos = texts[0].find("stable: number");
    ASSERT_NE(defPos, std::string::npos);

    References first = MockGetReferencesAtPosition(filePaths[0].c_str(), defPos, filePaths);
    // Every call creates and destroys its own contexts; repeating the whole query
    // must yield the exact same reference set.
    References second = MockGetReferencesAtPosition(filePaths[0].c_str(), defPos, filePaths);

    ASSERT_EQ(first.referenceInfos.size(), second.referenceInfos.size());
    AssertRefs(second, first.referenceInfos);

    const size_t readInFn = texts[0].find("stable;");
    ASSERT_NE(readInFn, std::string::npos);
    const size_t writePos = texts[0].rfind("stable = stable");
    ASSERT_NE(writePos, std::string::npos);
    const size_t readAfterWrite = writePos + std::string("stable = ").size();
    std::vector<ReferenceInfo> expected {
        {filePaths[0], readInFn, 6}, {filePaths[0], writePos, 6}, {filePaths[0], readAfterWrite, 6}};
    AssertRefs(first, expected);
}

// Scenario: index path and non-index path return the same reference set for the same
// position; the index path additionally reports the definition in definitionInfo and
// still excludes the definition occurrence from referenceInfos.
TEST_F(LSPGetReferencesAtPositionTests2, IndexPathAndNonIndexPathAreConsistent)
{
    std::vector<std::string> files = {"index_path_refs.ets"};
    std::vector<std::string> texts = {R"(let idx: number = 3;
function useIdx(): number {
    return idx;
}
idx = idx + 1;
console.log(idx);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const size_t defPos = texts[0].find("idx: number");
    ASSERT_NE(defPos, std::string::npos);

    // Non-index path.
    References nonIndex = MockGetReferencesAtPosition(filePaths[0].c_str(), defPos, filePaths);

    // Index path.
    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));
    References indexed = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(context, defPos);
    initializer.DestroyContext(context);

    // Both paths return exactly the same occurrences: the read in useIdx, the
    // assignment, the read after the assignment, and the console.log read.
    const size_t readInFn = texts[0].find("idx;");
    const size_t writePos = texts[0].rfind("idx = idx");
    const size_t readAfterWrite = writePos + std::string("idx = ").size();
    const size_t logPos = texts[0].rfind("idx);");
    ASSERT_NE(readInFn, std::string::npos);
    ASSERT_NE(writePos, std::string::npos);
    ASSERT_NE(logPos, std::string::npos);
    std::vector<ReferenceInfo> expected {{filePaths[0], readInFn, 3},
                                         {filePaths[0], writePos, 3},
                                         {filePaths[0], readAfterWrite, 3},
                                         {filePaths[0], logPos, 3}};
    AssertRefs(nonIndex, expected);
    AssertRefs(indexed, expected);

    // The index path additionally pins the definition.
    EXPECT_EQ(indexed.definitionInfo.fileName, filePaths[0]);
    EXPECT_EQ(indexed.definitionInfo.start, defPos);
    EXPECT_EQ(indexed.definitionInfo.length, std::string("idx").size());
    // The definition occurrence stays excluded from referenceInfos in both paths.
    EXPECT_TRUE(nonIndex.definitionInfo.fileName.empty());
    EXPECT_FALSE(ContainsRef(indexed, filePaths[0], defPos, std::string("idx").size()));
}

// Wrapper-level test: GetDeclInfo resolves the declaration of the identifier at
// the position, and GetReferencesAtPosition(es2panda_Context*, DeclInfo*) finds
// the exact usages. The wrapper converts spans to code-point offsets.
TEST_F(LSPGetReferencesAtPositionTests2, DeclInfoThenWrapperGetReferencesAtPosition)
{
    std::vector<std::string> files = {"decl_info_wrapper_refs.ets"};
    std::vector<std::string> texts = {R"(let wrapperNum: number = 1;
function readWrapper(): number {
    return wrapperNum;
}
wrapperNum = wrapperNum + 1;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    const size_t defPos = texts[0].find("wrapperNum: number");
    ASSERT_NE(defPos, std::string::npos);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    LSPAPI const *lspApi = GetImpl();

    DeclInfo declInfo = lspApi->getDeclInfo(context, defPos);
    ASSERT_FALSE(declInfo.fileName.empty());
    ASSERT_FALSE(declInfo.fileText.empty());

    References result = lspApi->getReferencesAtPosition(context, &declInfo);

    const size_t readInFn = texts[0].find("wrapperNum;");
    const size_t writePos = texts[0].rfind("wrapperNum = wrapperNum");
    constexpr size_t wrapperNumLength = 10;
    const size_t readAfterWrite = writePos + std::string("wrapperNum = ").size();
    ASSERT_NE(readInFn, std::string::npos);
    ASSERT_NE(writePos, std::string::npos);

    std::vector<ReferenceInfo> expected {{filePaths[0], readInFn, wrapperNumLength},
                                         {filePaths[0], writePos, wrapperNumLength},
                                         {filePaths[0], readAfterWrite, wrapperNumLength}};
    AssertRefs(result, expected);
    initializer.DestroyContext(context);
}

// Wrapper-level test: GetReferencesAtPosition(es2panda_Context*, DeclInfo*) with
// a DeclInfo that does not match any declaration in the file returns no references.
TEST_F(LSPGetReferencesAtPositionTests2, DeclInfoUnmatchedReturnsEmptyReferences)
{
    std::vector<std::string> files = {"decl_info_unmatched_refs.ets"};
    std::vector<std::string> texts = {R"(let unrelated: number = 1;
console.log(unrelated);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    LSPAPI const *lspApi = GetImpl();

    // A DeclInfo that refers to a non-existent declaration can never match any
    // node in this file, so no references are reported.
    DeclInfo declInfo {"nonexistent_file.ets", "let nonexistent: number = 0;"};
    References result = lspApi->getReferencesAtPosition(context, &declInfo);

    ASSERT_TRUE(result.referenceInfos.empty());
    initializer.DestroyContext(context);
}
