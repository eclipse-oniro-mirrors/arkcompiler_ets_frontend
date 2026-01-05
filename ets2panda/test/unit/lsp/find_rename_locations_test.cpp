/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>
#include <regex>
#include "lsp_api_test.h"
#include "lsp/include/find_rename_locations.h"
#include <gtest/gtest.h>

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::RenameLocation;

// NOLINTBEGIN
std::vector<std::string> fileNames = {"findRenameLocsOne.ets", "findRenameLocsTwo.ets"};
std::vector<std::string> fileContents = {
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
        import { dummy, abc, Foo  } from "./findRenameLocsOne.ets";

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

std::vector<std::string> fileNamesForSpecialCharacters = {"findRenameLocsThree.ets", "findRenameLocsFour.ets"};
std::vector<std::string> fileContentsForSpecialCharacters = {
    R"(
        //中文测试
        export function abc(x: string): void {
        }

        export function dummy(x: number): void {
        }

        //中文测试
        export class Foo {
            name: string = "中文测试";
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
        //中文测试
        abc("中文测试");
        //中文测试
        abc("中文测试");
        //中文测试
        abc("中文测试");
        )",
    R"(
        //中文测试
        import { dummy, abc, Foo  } from "./findRenameLocsThree.ets";

        dummy(4);
        dummy(44);
        abc("5");
        abc("55");
        abc("555");
        //中文测试
        let myfoo = new Foo("中文测试", 1, 2, 3);
        //中文测试
        let otherfoo = new Foo("中文测试", 4, 5, 6);

        console.log(myfoo)
        console.log(otherfoo)
        console.log(myfoo.name)
    )"};

static size_t getLine(std::string source, size_t pos)
{
    size_t line = 0;
    for (auto it = source.begin(); it < source.end() && it < source.begin() + pos; ++it) {
        if (*it == '\n') {
            ++line;
        }
    }
    return line;
}

class LspFindRenameLocationsTests1 : public LSPAPITests {
public:
    std::set<RenameLocation> genTestData(std::string word, std::string filePath, std::string source)
    {
        std::set<RenameLocation> data;
        std::regex regex {"\\W" + word + "\\W"};
        auto matchBeg = std::sregex_iterator {source.begin(), source.end(), regex};
        auto matchEnd = std::sregex_iterator();

        for (auto it = matchBeg; it != matchEnd; ++it) {
            size_t pos = it->position() + 1;
            size_t line = getLine(source, pos);
            RenameLocation loc {filePath, pos, pos + word.length(), line};
            printf("{R\"(%s)\", %ld, %ld, %ld, R\"(%s)\", R\"(%s)\"},\n", loc.fileName.c_str(), loc.start, loc.end,
                   loc.line, loc.prefixText.has_value() ? loc.prefixText->c_str() : "null",
                   loc.suffixText.has_value() ? loc.suffixText->c_str() : "null");
            data.insert(loc);
        }

        return data;
    }

    std::set<RenameLocation> genTestData(std::string pattern)
    {
        // Create the files
        auto filePaths = CreateTempFile(fileNames, fileContents);

        std::set<RenameLocation> data;
        printf("std::set<RenameLocation> expected_%s = {\n", pattern.c_str());
        for (size_t i = 0; i < filePaths.size(); ++i) {
            auto entries = genTestData(pattern, filePaths[i], fileContents[i]);
            for (const auto &entry : entries) {
                data.insert(entry);
            }
        }
        printf("};\n");
        return data;
    }
};

std::set<RenameLocation> expected_Foo = {
    {R"(/tmp/findRenameLocsTwo.ets)", 30, 33, 1, "Foo as "},
    {R"(/tmp/findRenameLocsTwo.ets)", 183, 186, 9},
    {R"(/tmp/findRenameLocsTwo.ets)", 234, 237, 10},
};
std::set<RenameLocation> expected_abc = {
    {R"(/tmp/findRenameLocsOne.ets)", 25, 28, 1},    {R"(/tmp/findRenameLocsOne.ets)", 899, 902, 35},
    {R"(/tmp/findRenameLocsOne.ets)", 915, 918, 36}, {R"(/tmp/findRenameLocsOne.ets)", 931, 934, 37},
    {R"(/tmp/findRenameLocsTwo.ets)", 25, 28, 1},    {R"(/tmp/findRenameLocsTwo.ets)", 115, 118, 5},
    {R"(/tmp/findRenameLocsTwo.ets)", 131, 134, 6},  {R"(/tmp/findRenameLocsTwo.ets)", 148, 151, 7},
};
std::set<RenameLocation> expected_dummy = {
    {R"(/tmp/findRenameLocsOne.ets)", 83, 88, 4},    {R"(/tmp/findRenameLocsOne.ets)", 863, 868, 33},
    {R"(/tmp/findRenameLocsOne.ets)", 881, 886, 34}, {R"(/tmp/findRenameLocsTwo.ets)", 18, 23, 1},
    {R"(/tmp/findRenameLocsTwo.ets)", 78, 83, 3},    {R"(/tmp/findRenameLocsTwo.ets)", 96, 101, 4},
};
std::set<RenameLocation> expected_name = {
    {R"(/tmp/findRenameLocsOne.ets)", 158, 162, 8},
    {R"(/tmp/findRenameLocsOne.ets)", 362, 366, 13},
    {R"(/tmp/findRenameLocsTwo.ets)", 343, 347, 14},
};
std::set<RenameLocation> expected_Foo_ForSpecialCharacters = {
    {R"(/tmp/findRenameLocsFour.ets)", 45, 48, 3},
    {R"(/tmp/findRenameLocsFour.ets)", 220, 223, 3},
    {R"(/tmp/findRenameLocsFour.ets)", 284, 287, 3},
    {R"(/tmp/findRenameLocsThree.ets)", 170, 173, 3},
};
std::set<RenameLocation> expected_abc_ForSpecialCharacters = {
    {R"(/tmp/findRenameLocsThree.ets)", 40, 43, 3},
    {R"(/tmp/findRenameLocsThree.ets)", 938, 941, 3},
    {R"(/tmp/findRenameLocsThree.ets)", 974, 977, 3},
    {R"(/tmp/findRenameLocsThree.ets)", 1010, 1013, 3},
};
std::set<RenameLocation> expected_abc_WithCancellation_ForSpecialCharacters = {
    {R"(/tmp/findRenameLocsThree.ets)", 40, 43, 3},   {R"(/tmp/findRenameLocsThree.ets)", 938, 941, 3},
    {R"(/tmp/findRenameLocsThree.ets)", 974, 977, 3}, {R"(/tmp/findRenameLocsThree.ets)", 1010, 1013, 3},
    {R"(/tmp/findRenameLocsFour.ets)", 40, 43, 3},    {R"(/tmp/findRenameLocsFour.ets)", 132, 135, 3},
    {R"(/tmp/findRenameLocsFour.ets)", 150, 153, 3},  {R"(/tmp/findRenameLocsFour.ets)", 169, 172, 3},
};

TEST_F(LspFindRenameLocationsTests1, FindRenameLocationsConstantName)
{
    // Create the files
    std::vector<std::string> files = {"getCompletionsAtPositionMemberKeyWord.ets"};
    std::vector<std::string> texts = {R"delimiter(
const newLocal = 1 + 1;
class test {
    newLocal = newLocal;
    static getDouble(): number {
        return newLocal;
    }
}
)delimiter"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::CancellationToken cancellationToken {123, nullptr};

    auto res = ark::es2panda::lsp::FindRenameLocationsInCurrentFile(context, texts[0].find("newLocal"));
    int renameCount1 = 3;
    ASSERT_EQ(res.size(), renameCount1);

    res = ark::es2panda::lsp::FindRenameLocationsInCurrentFile(context, texts[0].find("newLocal;"));
    int renameCount2 = 3;
    ASSERT_EQ(res.size(), renameCount2);
    initializer.DestroyContext(context);
}

TEST_F(LspFindRenameLocationsTests1, FindRenameLocationsClassName)
{
    // Create the files
    auto filePaths = CreateTempFile(fileNames, fileContents);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ark::es2panda::lsp::CancellationToken cancellationToken {123, nullptr};
    auto res = ark::es2panda::lsp::FindRenameLocationsInCurrentFile(context, fileContents[1].find("Foo  }"));
    ASSERT_EQ(res.size(), expected_Foo.size());
    for (auto renameLoc : res) {
        auto found = expected_Foo.find(renameLoc);
        ASSERT_TRUE(found != expected_Foo.end());
    }
    initializer.DestroyContext(context);
}

TEST_F(LspFindRenameLocationsTests1, FindRenameLocationsFunctionName)
{
    // Create the files
    auto filePaths = CreateTempFile(fileNames, fileContents);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto fileContexts = std::vector<es2panda_Context *>();
    for (const auto &filePath : filePaths) {
        auto fileContext = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);
        fileContexts.push_back(fileContext);
    }

    // Search for rename locations
    ark::es2panda::lsp::CancellationToken cancellationToken {123, nullptr};
    auto res = ark::es2panda::lsp::FindRenameLocations(&cancellationToken, fileContexts, context, 25);
    ASSERT_EQ(res.size(), expected_abc.size());
    for (auto renameLoc : res) {
        auto found = expected_abc.find(renameLoc);
        ASSERT_TRUE(found != expected_abc.end());
    }
    for (size_t i = 0; i < fileContexts.size(); ++i) {
        initializer.DestroyContext(fileContexts[i]);
    }
    initializer.DestroyContext(context);
}

TEST_F(LspFindRenameLocationsTests1, FindRenameLocationsFunctionName2)
{
    // Create the files
    auto filePaths = CreateTempFile(fileNames, fileContents);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto fileContexts = std::vector<es2panda_Context *>();
    for (const auto &filePath : filePaths) {
        auto fileContext = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);
        fileContexts.push_back(fileContext);
    }

    // Search for rename locations
    ark::es2panda::lsp::CancellationToken cancellationToken {123, nullptr};
    auto res = ark::es2panda::lsp::FindRenameLocations(&cancellationToken, fileContexts, context, 83);
    ASSERT_EQ(res.size(), expected_dummy.size());
    for (auto renameLoc : res) {
        auto found = expected_dummy.find(renameLoc);
        ASSERT_TRUE(found != expected_dummy.end());
    }
    for (size_t i = 0; i < fileContexts.size(); ++i) {
        initializer.DestroyContext(fileContexts[i]);
    }
    initializer.DestroyContext(context);
}

TEST_F(LspFindRenameLocationsTests1, FindRenameLocationsClassMemberName)
{
    // Create the files
    auto filePaths = CreateTempFile(fileNames, fileContents);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto fileContexts = std::vector<es2panda_Context *>();
    for (const auto &filePath : filePaths) {
        auto fileContext = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);
        fileContexts.push_back(fileContext);
    }

    // Search for rename locations
    ark::es2panda::lsp::CancellationToken cancellationToken {123, nullptr};
    auto res = ark::es2panda::lsp::FindRenameLocations(&cancellationToken, fileContexts, context, 158);
    ASSERT_EQ(res.size(), expected_name.size());
    for (auto renameLoc : res) {
        auto found = expected_name.find(renameLoc);
        ASSERT_TRUE(found != expected_name.end());
    }
    for (size_t i = 0; i < fileContexts.size(); ++i) {
        initializer.DestroyContext(fileContexts[i]);
    }
    initializer.DestroyContext(context);
}

TEST_F(LspFindRenameLocationsTests1, FindRenameLocationsForSpecialCharacters)
{
    // Create the files
    auto filePaths = CreateTempFile(fileNamesForSpecialCharacters, fileContentsForSpecialCharacters);
    Initializer initializer = Initializer();

    auto context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto fileContexts = std::vector<es2panda_Context *>();
    for (const auto &filePath : filePaths) {
        auto fileContext = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);
        fileContexts.push_back(fileContext);
    }
    LSPAPI const *lspApi = GetImpl();
    size_t position = 46;
    auto res = lspApi->findRenameLocations(fileContexts, context, position);
    ASSERT_EQ(res.size(), expected_Foo_ForSpecialCharacters.size());
    for (size_t i = 0; i < fileContexts.size(); ++i) {
        initializer.DestroyContext(fileContexts[i]);
    }
    for (auto renameLoc : res) {
        auto found = expected_Foo_ForSpecialCharacters.find(renameLoc);
        ASSERT_TRUE(found != expected_Foo_ForSpecialCharacters.end());
    }
    initializer.DestroyContext(context);
}

TEST_F(LspFindRenameLocationsTests1, FindRenameLocationsInCurrentFileForSpecialCharacters)
{
    // Create the files
    auto filePaths = CreateTempFile(fileNamesForSpecialCharacters, fileContentsForSpecialCharacters);
    Initializer initializer = Initializer();

    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    size_t position = 40;
    auto res = lspApi->findRenameLocationsInCurrentFile(context, position);
    ASSERT_EQ(res.size(), expected_abc_ForSpecialCharacters.size());
    for (auto renameLoc : res) {
        auto found = expected_abc_ForSpecialCharacters.find(renameLoc);
        ASSERT_TRUE(found != expected_abc_ForSpecialCharacters.end());
    }
    initializer.DestroyContext(context);
}

TEST_F(LspFindRenameLocationsTests1, FindRenameLocationsWithCancellationForSpecialCharacters)
{
    // Create the files
    auto filePaths = CreateTempFile(fileNamesForSpecialCharacters, fileContentsForSpecialCharacters);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto fileContexts = std::vector<es2panda_Context *>();
    for (const auto &filePath : filePaths) {
        auto fileContext = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);
        fileContexts.push_back(fileContext);
    }

    // Search for rename locations
    ark::es2panda::lsp::CancellationToken cancellationToken {123, nullptr};
    LSPAPI const *lspApi = GetImpl();
    size_t position = 40;
    auto res = lspApi->findRenameLocationsWithCancellationToken(&cancellationToken, fileContexts, context, position);
    ASSERT_EQ(res.size(), expected_abc_WithCancellation_ForSpecialCharacters.size());
    for (auto renameLoc : res) {
        auto found = expected_abc_WithCancellation_ForSpecialCharacters.find(renameLoc);
        ASSERT_TRUE(found != expected_abc_WithCancellation_ForSpecialCharacters.end());
    }
    for (size_t i = 0; i < fileContexts.size(); ++i) {
        initializer.DestroyContext(fileContexts[i]);
    }
    initializer.DestroyContext(context);
}

// NOLINTEND