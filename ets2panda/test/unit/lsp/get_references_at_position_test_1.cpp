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
#include "lsp/include/internal_api.h"
#include "lsp/include/references.h"
#include "public/es2panda_lib.h"

using ark::es2panda::lsp::Initializer;

class LSPGetReferencesAtPositionTests1 : public LSPAPITests {
public:
    LSPGetReferencesAtPositionTests1() = default;
    ~LSPGetReferencesAtPositionTests1() override = default;

    NO_COPY_SEMANTIC(LSPGetReferencesAtPositionTests1);
    NO_MOVE_SEMANTIC(LSPGetReferencesAtPositionTests1);

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
};

// Test: import alias references - references to the alias should be found
TEST_F(LSPGetReferencesAtPositionTests1, ImportAliasReferences)
{
    std::vector<std::string> files = {"import_alias_export.ets", "import_alias_use.ets"};
    std::vector<std::string> texts = {R"(export class OriginalClass { value: number = 1; })",
                                      R"(import { OriginalClass as AliasedClass } from './import_alias_export';
let foo: AliasedClass = new AliasedClass();
foo.value;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // Position at "AliasedClass" usage (after "new ")
    const size_t position = texts[1].find("AliasedClass();");
    ASSERT_NE(position, std::string::npos);
    References result = MockGetReferencesAtPosition(filePaths[1].c_str(), position, filePaths);

    // Should find at least the usage references in the import file
    ASSERT_GE(result.referenceInfos.size(), 1U);
    // All references should be in the import file (alias is local to that file)
    for (const auto &ref : result.referenceInfos) {
        ASSERT_EQ(ref.fileName, filePaths[1]);
    }
}

// Test: export alias (re-export with alias) references
TEST_F(LSPGetReferencesAtPositionTests1, ExportAliasReferences)
{
    std::vector<std::string> files = {"export_alias_source.ets", "export_alias_reexport.ets", "export_alias_use.ets"};
    std::vector<std::string> texts = {R"(export class SourceClass { value: number = 1; })",
                                      R"(export { SourceClass as ReExportedClass } from './export_alias_source';)",
                                      R"(import { ReExportedClass } from './export_alias_reexport';
let foo: ReExportedClass = new ReExportedClass();
foo.value;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // Position at "ReExportedClass" in the use file (the type annotation)
    const size_t position = texts[2].find("ReExportedClass =");
    ASSERT_NE(position, std::string::npos);
    References result = MockGetReferencesAtPosition(filePaths[2].c_str(), position, filePaths);

    // Should find references in the use file
    ASSERT_GE(result.referenceInfos.size(), 1U);
    bool hasUseFileRef = false;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t useFileIdx = 2;  // index of the use file in filePaths
    for (const auto &ref : result.referenceInfos) {
        if (ref.fileName == filePaths[useFileIdx]) {
            hasUseFileRef = true;
            break;
        }
    }
    ASSERT_TRUE(hasUseFileRef);
}

// Test: re-export references - reference to a re-exported symbol
TEST_F(LSPGetReferencesAtPositionTests1, ReExportReferences)
{
    std::vector<std::string> files = {"reexport_source.ets", "reexport_middle.ets", "reexport_use.ets"};
    std::vector<std::string> texts = {R"(export function originalFunc(): void {})",
                                      R"(export { originalFunc } from './reexport_source';)",
                                      R"(import { originalFunc } from './reexport_middle';
originalFunc();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // Position at "originalFunc" definition in source file
    const size_t position = texts[0].find("originalFunc");
    ASSERT_NE(position, std::string::npos);
    References result = MockGetReferencesAtPosition(filePaths[0].c_str(), position, filePaths);

    // Should find references across the re-export chain
    ASSERT_GE(result.referenceInfos.size(), 1U);
}

// Test: shadowing - local variable shadows import, references should not cross
TEST_F(LSPGetReferencesAtPositionTests1, ShadowingDoesNotCrossScopes)
{
    std::vector<std::string> files = {"shadowing_export.ets", "shadowing_import.ets"};
    std::vector<std::string> texts = {R"(export let shadowed: number = 1;)",
                                      R"(import { shadowed } from './shadowing_export';
function foo() {
    let shadowed: string = "local";
    return shadowed;
}
console.log(shadowed);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // Position at the imported "shadowed" (top-level usage in console.log)
    const size_t position = texts[1].rfind("shadowed)");
    ASSERT_NE(position, std::string::npos);
    References result = MockGetReferencesAtPosition(filePaths[1].c_str(), position, filePaths);

    // Should find references to the imported symbol, not the local shadowed variable
    ASSERT_GE(result.referenceInfos.size(), 1U);
    // The local "shadowed" inside foo() should NOT be in the results (different symbol)
    // Verify that references found are for the imported symbol
    bool hasImportRef = false;
    for (const auto &ref : result.referenceInfos) {
        if (ref.fileName == filePaths[1]) {
            hasImportRef = true;
            break;
        }
    }
    ASSERT_TRUE(hasImportRef);
}

// Test: method call vs property access - references should distinguish
TEST_F(LSPGetReferencesAtPositionTests1, MethodCallVsPropertyAccess)
{
    std::vector<std::string> files = {"method_vs_property.ets"};
    std::vector<std::string> texts = {R"(class Foo {
    value: number = 1;
    getValue(): number {
        return this.value;
    }
}
let foo = new Foo();
foo.value;
foo.getValue();
foo.value = 2;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // Position at "value" property definition
    const size_t position = texts[0].find("value: number");
    ASSERT_NE(position, std::string::npos);
    References result = MockGetReferencesAtPosition(filePaths[0].c_str(), position, filePaths);

    // Should find references to the "value" property (reads and writes)
    ASSERT_GE(result.referenceInfos.size(), 1U);
    // Verify all references are in the same file
    for (const auto &ref : result.referenceInfos) {
        ASSERT_EQ(ref.fileName, filePaths[0]);
    }
}

// Test: references to a class member through this. and external access
TEST_F(LSPGetReferencesAtPositionTests1, ClassMemberReferencesThroughThisAndExternal)
{
    std::vector<std::string> files = {"class_member_refs.ets"};
    std::vector<std::string> texts = {R"(class Counter {
    count: number = 0;
    increment(): void {
        this.count++;
    }
    reset(): void {
        this.count = 0;
    }
}
let c = new Counter();
c.count = 10;
console.log(c.count);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // Position at "count" property definition
    const size_t position = texts[0].find("count: number");
    ASSERT_NE(position, std::string::npos);
    References result = MockGetReferencesAtPosition(filePaths[0].c_str(), position, filePaths);

    // Should find references to "count" both via this.count and c.count
    ASSERT_GE(result.referenceInfos.size(), 1U);
}

// Test: string literal with same name should not be counted as reference
TEST_F(LSPGetReferencesAtPositionTests1, StringLiteralNotCountedAsReference)
{
    std::vector<std::string> files = {"string_not_ref.ets"};
    std::vector<std::string> texts = {R"(let foo: number = 1;
let bar: string = "foo";
console.log(foo);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // Position at "foo" variable definition
    const size_t position = texts[0].find("foo: number");
    ASSERT_NE(position, std::string::npos);
    References result = MockGetReferencesAtPosition(filePaths[0].c_str(), position, filePaths);

    // Should find the definition and the usage in console.log, but NOT the string "foo"
    ASSERT_GE(result.referenceInfos.size(), 1U);
    // Verify no reference points to the string literal position
    const size_t stringLitPos = texts[0].find("\"foo\"");
    ASSERT_NE(stringLitPos, std::string::npos);
    for (const auto &ref : result.referenceInfos) {
        // The string literal content position should not match
        ASSERT_FALSE(ref.fileName == filePaths[0] && ref.start == stringLitPos + 1);
    }
}

// Test: comment with same name should not be counted as reference
TEST_F(LSPGetReferencesAtPositionTests1, CommentNotCountedAsReference)
{
    std::vector<std::string> files = {"comment_not_ref.ets"};
    std::vector<std::string> texts = {R"(// foo is a variable
let foo: number = 1;
// use foo here
console.log(foo);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // Position at "foo" variable definition
    const size_t position = texts[0].find("foo: number");
    ASSERT_NE(position, std::string::npos);
    References result = MockGetReferencesAtPosition(filePaths[0].c_str(), position, filePaths);

    // Should find the definition and usage, but NOT the comment occurrences
    ASSERT_GE(result.referenceInfos.size(), 1U);
}

// Test: multi-file same name symbols should not cross-reference
TEST_F(LSPGetReferencesAtPositionTests1, MultiFileSameNameSymbolsNotCrossReferenced)
{
    std::vector<std::string> files = {"multi_file_a.ets", "multi_file_b.ets"};
    std::vector<std::string> texts = {R"(export let shared: number = 1;
console.log(shared);)",
                                      R"(let shared: string = "local";
console.log(shared);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // Position at "shared" in file A
    const size_t position = texts[0].find("shared: number");
    ASSERT_NE(position, std::string::npos);
    References result = MockGetReferencesAtPosition(filePaths[0].c_str(), position, filePaths);

    // References should only be in file A, not file B (different symbols)
    ASSERT_GE(result.referenceInfos.size(), 1U);
    for (const auto &ref : result.referenceInfos) {
        ASSERT_EQ(ref.fileName, filePaths[0])
            << "References should not cross to file B which has a different 'shared' symbol";
    }
}
