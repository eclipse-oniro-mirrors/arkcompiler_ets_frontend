/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <gtest/gtest.h>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "es2panda.h"
#include "lsp/include/find_references.h"
#include "lsp/include/cancellation_token.h"
#include "lsp_api_test.h"

// NOLINTBEGIN
using ark::es2panda::SourceFile;
using ark::es2panda::lexer::SourcePosition;
using ark::es2panda::lsp::FindReferences;
using std::string;
using std::vector;

static auto testCase(vector<SourceFile> &sourceFiles, SourceFile selectedFile, size_t tokenOffset,
                     std::set<ark::es2panda::lsp::ReferencedNode> expected)
{
    auto cancellationToken = ark::es2panda::lsp::CancellationToken(123, nullptr);
    auto res = FindReferences(&cancellationToken, sourceFiles, selectedFile, tokenOffset);

    (void)res;

    ASSERT_EQ(res.size(), expected.size());

    for (const ark::es2panda::lsp::ReferencedNode &reference : res) {
        auto found = expected.find(reference);
        ASSERT_NE(found, expected.end());
    }
}

vector<string> fileNames = {"findReferencesOne.ets", "findReferencesTwo.ets"};
vector<string> fileContents = {
    R"(
        export function abc(x: number): void {
        }

        export function dummy(x: number): void {
        }

        export class Foo {
            name: string = "unassigned";
            x: number = 1;
            y: number = 2;
            z: number = 3;
            constructor(name: string, x: number, y: number, z: number) {
                this.name = name;
                this.x = x;
                this.y = y;
                this.z = z;
            }
        };

        export class Oranges {
            name: string = "unassigned";
            x: number = 1;
            y: number = 2;
            z: number = 3;
            constructor(name: string, x: number, y: number, z: number) {
                this.name = name;
                this.x = x;
                this.y = y;
                this.z = z;
            }
        };

        dummy(0);
        dummy(1);
        abc(2);
        abc(3);
        abc(4);
        )",
    R"(
        import { dummy, abc, Foo  } from "./findReferencesOne.ets";

        dummy(4);
        dummy(44);
        abc(5);
        abc(55);
        abc(555);

        let myfoo = new Foo("apples", 1, 2, 3);
        let otherfoo = new Foo("oranges", 4, 5, 6);

        console.log(myfoo)
        console.log(otherfoo)
        console.log(myfoo.name)
    )"};

const std::string tempDir = "/tmp/lsp_api_test_find_references/";

std::set<ark::es2panda::lsp::ReferencedNode> expected_dummy = {
    {tempDir + "findReferencesOne.ets", 83, 88, 4, true},     {tempDir + "findReferencesOne.ets", 863, 868, 33, false},
    {tempDir + "findReferencesOne.ets", 881, 886, 34, false}, {tempDir + "findReferencesTwo.ets", 18, 23, 1, false},
    {tempDir + "findReferencesTwo.ets", 78, 83, 3, false},    {tempDir + "findReferencesTwo.ets", 96, 101, 4, false},
};

std::set<ark::es2panda::lsp::ReferencedNode> expected_abc = {
    {tempDir + "findReferencesOne.ets", 25, 28, 1, true},     {tempDir + "findReferencesOne.ets", 899, 902, 35, false},
    {tempDir + "findReferencesOne.ets", 915, 918, 36, false}, {tempDir + "findReferencesOne.ets", 931, 934, 37, false},
    {tempDir + "findReferencesTwo.ets", 25, 28, 1, false},    {tempDir + "findReferencesTwo.ets", 115, 118, 5, false},
    {tempDir + "findReferencesTwo.ets", 131, 134, 6, false},  {tempDir + "findReferencesTwo.ets", 148, 151, 7, false},
};

std::set<ark::es2panda::lsp::ReferencedNode> expected_myfoo = {
    {tempDir + "findReferencesTwo.ets", 171, 176, 9, true},
    {tempDir + "findReferencesTwo.ets", 280, 285, 12, false},
    {tempDir + "findReferencesTwo.ets", 337, 342, 14, false},
};

std::set<ark::es2panda::lsp::ReferencedNode> expected_Foo = {
    {tempDir + "findReferencesOne.ets", 140, 143, 7, true},
    {tempDir + "findReferencesTwo.ets", 30, 33, 1, false},
    {tempDir + "findReferencesTwo.ets", 183, 186, 9, false},
    {tempDir + "findReferencesTwo.ets", 234, 237, 10, false},
};

std::set<ark::es2panda::lsp::ReferencedNode> expected_name = {
    {tempDir + "findReferencesOne.ets", 158, 162, 8, true},
    {tempDir + "findReferencesOne.ets", 362, 366, 13, false},
    {tempDir + "findReferencesTwo.ets", 343, 347, 14, false},
};

class LspFindRefTests : public LSPAPITests {};

TEST_F(LspFindRefTests, FindReferencesMethodDefinition1)
{
    // Create the files
    auto filePaths = CreateTempFile(fileNames, fileContents);
    vector<SourceFile> sourceFiles;
    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(SourceFile {filePaths[i], fileContents[i]});
    }
    ASSERT_TRUE(sourceFiles.size() == fileNames.size());

    // Case 1: Search for the first occurance of "abc" within "findReferencesOne.ets" which is a method definition
    {
        auto srcIndex = 0;
        size_t tokenOffset = 25;
        testCase(sourceFiles, sourceFiles[srcIndex], tokenOffset, expected_abc);
    }
}

TEST_F(LspFindRefTests, FindReferencesMethodDefinition2)
{
    // Create the files
    auto filePaths = CreateTempFile(fileNames, fileContents);
    vector<SourceFile> sourceFiles;
    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(SourceFile {filePaths[i], fileContents[i]});
    }
    ASSERT_TRUE(sourceFiles.size() == fileNames.size());

    // Case 2: Search for the first occurance of "dummy" within "findReferencesOne.ets" which is a method definition
    {
        auto srcIndex = 0;
        size_t tokenOffset = 83;
        testCase(sourceFiles, sourceFiles[srcIndex], tokenOffset, expected_dummy);
    }
}

TEST_F(LspFindRefTests, FindReferencesImportSpecifier)
{
    // Create the files
    auto filePaths = CreateTempFile(fileNames, fileContents);
    vector<SourceFile> sourceFiles;
    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(SourceFile {filePaths[i], fileContents[i]});
    }
    ASSERT_TRUE(sourceFiles.size() == fileNames.size());

    // Case 3: Search for the first occurance of "abc" within "findReferencesTwo.ets" which is an import specifier
    {
        auto srcIndex = 1;
        size_t tokenOffset = 25;
        testCase(sourceFiles, sourceFiles[srcIndex], tokenOffset, expected_abc);
    }
}

TEST_F(LspFindRefTests, FindReferencesCallExpression1)
{
    // Create the files
    auto filePaths = CreateTempFile(fileNames, fileContents);
    vector<SourceFile> sourceFiles;
    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(SourceFile {filePaths[i], fileContents[i]});
    }
    ASSERT_TRUE(sourceFiles.size() == fileNames.size());

    // Case 4: Search for the second occurance of "abc" within "findReferencesTwo.ets" which is a function call
    // expression
    {
        auto srcIndex = 1;
        size_t tokenOffset = 115;
        testCase(sourceFiles, sourceFiles[srcIndex], tokenOffset, expected_abc);
    }
}

TEST_F(LspFindRefTests, FindReferencesCallExpression2)
{
    // Create the files
    auto filePaths = CreateTempFile(fileNames, fileContents);
    vector<SourceFile> sourceFiles;
    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(SourceFile {filePaths[i], fileContents[i]});
    }
    ASSERT_TRUE(sourceFiles.size() == fileNames.size());

    // Case 5: Search for the second occurance of "dummy" within "findReferencesTwo.ets" which is a function call
    // expression
    {
        auto srcIndex = 1;
        size_t tokenOffset = 78;
        testCase(sourceFiles, sourceFiles[srcIndex], tokenOffset, expected_dummy);
    }
}

TEST_F(LspFindRefTests, FindReferencesVariableDefinition)
{
    // Create the files
    auto filePaths = CreateTempFile(fileNames, fileContents);
    vector<SourceFile> sourceFiles;
    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(SourceFile {filePaths[i], fileContents[i]});
    }
    ASSERT_TRUE(sourceFiles.size() == fileNames.size());

    // Case 6: Search for the first occurance of "myfoo" within "findReferencesTwo.ets" which is a variable definition
    {
        auto srcIndex = 1;
        size_t tokenOffset = 171;
        testCase(sourceFiles, sourceFiles[srcIndex], tokenOffset, expected_myfoo);
    }
}

TEST_F(LspFindRefTests, FindReferencesInstanceCreation)
{
    // Create the files
    auto filePaths = CreateTempFile(fileNames, fileContents);
    vector<SourceFile> sourceFiles;
    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(SourceFile {filePaths[i], fileContents[i]});
    }
    ASSERT_TRUE(sourceFiles.size() == fileNames.size());

    // Case 7: Search for the first occurance of "Foo" within "findReferencesTwo.ets" which is a class instance creation
    {
        auto srcIndex = 1;
        size_t tokenOffset = 30;
        testCase(sourceFiles, sourceFiles[srcIndex], tokenOffset, expected_Foo);
    }
}

TEST_F(LspFindRefTests, FindReferencesMemberAccess)
{
    // Create the files
    auto filePaths = CreateTempFile(fileNames, fileContents);
    vector<SourceFile> sourceFiles;
    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(SourceFile {filePaths[i], fileContents[i]});
    }
    ASSERT_TRUE(sourceFiles.size() == fileNames.size());

    // Case 7: Search for the first occurance of "name" within "findReferencesTwo.ets" which is a reference to a member
    {
        auto srcIndex = 1;
        size_t tknIndex = 343;

        testCase(sourceFiles, sourceFiles[srcIndex], tknIndex, expected_name);
    }
}

// NOLINTEND
