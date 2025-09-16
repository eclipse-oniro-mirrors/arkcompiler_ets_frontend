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

#include <cstddef>
#include <cstdio>
#include <gtest/gtest.h>
#include <iostream>
#include <ostream>
#include <regex>
#include <string>
#include <vector>

#include "lsp_api_test.h"
#include "lsp/include/class_hierarchy.h"
#include "lsp/include/internal_api.h"

namespace {
// NOLINTBEGIN
using ark::es2panda::lsp::Initializer;

class LspClassHierarchiesTests : public LSPAPITests {};

std::vector<std::string> fileContents = {
    R"(
interface Iaa {
  name: number
  name1: number
  name2: number

  hello(): void
  hello1(): void
}

interface Iaaa extends Iaa {
  name: number
}

interface Iaab extends Iaa {
  name: number
}

interface Iaaaa extends Iaaa {
  name: number
}

interface Iaaab extends Iaaa {
  name: number
}

abstract class Caa implements Iaa {
  name: number = 1
  name1: number = 1
  name2: number = 1

  hello(): void {
    throw new Error("Method not implemented.")
  }

  age: number = 0
  age2: number = 0
  age3: number = 0

  abstract classHello(): void

  classHello1() {
    return 1
  }
}

class Caaa extends Caa implements Iaaa {
  age: number = 0
  age4: number = 0
  age5: number = 0

  classHello(): void {
    throw new Error("Method not implemented.")
  }
}

class Caab extends Caa implements Iaab {
  name: number = 2
}

class Caaaa extends Caaa implements Iaaaa {
  name: number = 3
}

class Caaab extends Caaa implements Iaaab {
  name: number = 4
}

class Caaba extends Caab implements Iaaa {
  name: number = 5
}

class Caabb extends Caab implements Iaab {
  name: number = 6
}

export class Test extends Caabb implements Iaab {
  name: number = 7
}
)"};

TEST_F(LspClassHierarchiesTests, GetClassHierarchiesImpl_001)
{
    constexpr size_t expectedInfoCount = 5;
    constexpr size_t tokenOffset = 600;

    std::vector<std::string> fileNames = {"GetClassHierarchiesImpl_001_file1.ets"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    std::vector<ark::es2panda::SourceFile> sourceFiles;
    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents[i]);
    }
    ASSERT_EQ(fileNames.size(), sourceFiles.size());
    Initializer initializer;
    size_t sourceIndex = 0;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto *newVector = new std::vector<es2panda_Context *>();
    newVector->push_back(context);
    std::vector<ark::es2panda::lsp::ClassHierarchyItemInfo> infos =
        ark::es2panda::lsp::GetClassHierarchiesImpl(newVector, fileNames[sourceIndex], tokenOffset);
    ASSERT_EQ(expectedInfoCount, infos.size());
}

TEST_F(LspClassHierarchiesTests, GetClassHierarchiesImpl_002)
{
    constexpr size_t expectedInfoCount = 5;
    constexpr size_t tokenOffset = 1100;

    std::vector<std::string> fileNames = {"GetClassHierarchiesImpl_002_file1.ets"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    std::vector<ark::es2panda::SourceFile> sourceFiles;
    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents[i]);
    }
    ASSERT_EQ(fileNames.size(), sourceFiles.size());
    Initializer initializer;
    size_t sourceIndex = 0;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto *newVector = new std::vector<es2panda_Context *>();
    newVector->push_back(context);
    std::vector<ark::es2panda::lsp::ClassHierarchyItemInfo> infos =
        ark::es2panda::lsp::GetClassHierarchiesImpl(newVector, fileNames[sourceIndex], tokenOffset);
    ASSERT_EQ(expectedInfoCount, infos.size());
}

TEST_F(LspClassHierarchiesTests, GetClassHierarchiesImpl_003)
{
    constexpr size_t expectedInfoCount = 7;
    constexpr size_t tokenOffset = 101;  // Note: here will get interface decl: `Iaaa`

    std::vector<std::string> fileNames = {"GetClassHierarchiesImpl_003_file1.ets"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    std::vector<ark::es2panda::SourceFile> sourceFiles;
    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents[i]);
    }
    ASSERT_EQ(fileNames.size(), sourceFiles.size());
    Initializer initializer;
    size_t sourceIndex = 0;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto *newVector = new std::vector<es2panda_Context *>();
    newVector->push_back(context);
    std::vector<ark::es2panda::lsp::ClassHierarchyItemInfo> infos =
        ark::es2panda::lsp::GetClassHierarchiesImpl(newVector, fileNames[sourceIndex], tokenOffset);
    ASSERT_EQ(expectedInfoCount, infos.size());
}

TEST_F(LspClassHierarchiesTests, GetClassHierarchiesImpl_004)
{
    constexpr size_t expectedInfoCount = 7;
    constexpr size_t tokenOffset = 130;

    std::vector<std::string> fileNames = {"GetClassHierarchiesImpl_004_file1.ets"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    std::vector<ark::es2panda::SourceFile> sourceFiles;
    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents[i]);
    }
    ASSERT_EQ(fileNames.size(), sourceFiles.size());
    Initializer initializer;
    size_t sourceIndex = 0;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto *newVector = new std::vector<es2panda_Context *>();
    newVector->push_back(context);
    std::vector<ark::es2panda::lsp::ClassHierarchyItemInfo> infos =
        ark::es2panda::lsp::GetClassHierarchiesImpl(newVector, fileNames[sourceIndex], tokenOffset);
    ASSERT_EQ(expectedInfoCount, infos.size());
}

TEST_F(LspClassHierarchiesTests, GetClassHierarchiesImpl_005)
{
    constexpr size_t expectedInfoCount = 5;
    constexpr size_t tokenOffset = 1000;

    std::vector<std::string> fileNames = {"GetClassHierarchiesImpl_005_file1.ets"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    std::vector<ark::es2panda::SourceFile> sourceFiles;
    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents[i]);
    }
    ASSERT_EQ(fileNames.size(), sourceFiles.size());
    Initializer initializer;
    size_t sourceIndex = 0;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto *newVector = new std::vector<es2panda_Context *>();
    newVector->push_back(context);
    std::vector<ark::es2panda::lsp::ClassHierarchyItemInfo> infos =
        ark::es2panda::lsp::GetClassHierarchiesImpl(newVector, fileNames[sourceIndex], tokenOffset);
    ASSERT_EQ(expectedInfoCount, infos.size());
}

std::vector<std::string> fileContentsDisributed = {
    R"(
import { Iaa } from "./GetClassHierarchiesImpl_006_Iaa"

export abstract class Caa implements Iaa {
  name: number = 1
  name1: number = 1
  name2: number = 1

  hello(): void {
    throw new Error("Method not implemented.")
  }

  age: number = 0
  age2: number = 0
  age3: number = 0

  abstract classHello(): void

  classHello1() {
    return 1
  }
}
)",
    R"(
import { Caa } from "./GetClassHierarchiesImpl_006_Caa"
import { Iaaa, Iaab } from "./GetClassHierarchiesImpl_006_Iaaa_Iaab"

export class Caaa extends Caa implements Iaaa {
  age: number = 0
  age4: number = 0
  age5: number = 0

  classHello(): void {
    throw new Error("Method not implemented.")
  }
}

export class Caab extends Caa implements Iaab {
  name: number = 2
}
)",
    R"(
import { Caaa, Caab } from "./GetClassHierarchiesImpl_006_Caaa_Caab"
import { Iaa } from "./GetClassHierarchiesImpl_006_Iaa"
import { Iaaaa, Iaaab } from "./GetClassHierarchiesImpl_006_Iaaaa_Iaaab"
import { Iaaa, Iaab } from "./GetClassHierarchiesImpl_006_Iaaa_Iaab"

export class Caaaa extends Caaa implements Iaaaa {
  name: number = 3
}

export class Caaab extends Caaa implements Iaaab {
  name: number = 4
}

export class Caaba extends Caab implements Iaaa {
  name: number = 5
}

export class Caabb extends Caab implements Iaab {
  name: number = 6
}
)",
    R"(
export interface Iaa {
  name: number
  name1: number
  name2: number

  hello(): void
  hello1(): void
}
)",
    R"(
import { Iaa } from "./GetClassHierarchiesImpl_006_Iaa"

export interface Iaaa extends Iaa {
  name: number
}

export interface Iaab extends Iaa {
  name: number
}
)",
    R"(
import { Iaa } from "./GetClassHierarchiesImpl_006_Iaa"
import { Iaaa } from "./GetClassHierarchiesImpl_006_Iaaa_Iaab"

export interface Iaaaa extends Iaaa {
  name: number
}

export interface Iaaab extends Iaaa {
  name: number
}
)",
    R"(
import { Caabb } from "./GetClassHierarchiesImpl_006_Caaaa_Caaab_Caaba_Caabb";
import { Iaab } from "./GetClassHierarchiesImpl_006_Iaaa_Iaab";

export class Test extends Caabb implements Iaab {
  name: number = 7
}
)"};

TEST_F(LspClassHierarchiesTests, GetClassHierarchiesImpl_006)
{
    constexpr size_t expectedInfoCount = 8;
    constexpr size_t tokenOffset = 80;

    std::vector<std::string> fileNames = {"GetClassHierarchiesImpl_006_Caa.ets",
                                          "GetClassHierarchiesImpl_006_Caaa_Caab.ets",
                                          "GetClassHierarchiesImpl_006_Caaaa_Caaab_Caaba_Caabb.ets",
                                          "GetClassHierarchiesImpl_006_Iaa.ets",
                                          "GetClassHierarchiesImpl_006_Iaaa_Iaab.ets",
                                          "GetClassHierarchiesImpl_006_Iaaaa_Iaaab.ets",
                                          "GetClassHierarchiesImpl_006_Test.ets"};
    auto filePaths = CreateTempFile(fileNames, fileContentsDisributed);
    std::vector<ark::es2panda::SourceFile> sourceFiles;
    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContentsDisributed[i]);
    }
    ASSERT_EQ(fileNames.size(), sourceFiles.size());
    auto *newVector = new std::vector<es2panda_Context *>();
    auto *initVector = new std::vector<Initializer *>();
    for (size_t i = 0; i < filePaths.size(); ++i) {
        auto initializer = new Initializer();
        auto filePath = std::string {sourceFiles[i].filePath};
        auto fileContent = std::string {sourceFiles[i].source};
        auto context = initializer->CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
        newVector->push_back(context);
        initVector->push_back(initializer);
    }
    size_t sourceIndex = 0;
    std::vector<ark::es2panda::lsp::ClassHierarchyItemInfo> infos =
        ark::es2panda::lsp::GetClassHierarchiesImpl(newVector, fileNames[sourceIndex], tokenOffset);
    for (size_t i = 0; i < filePaths.size(); ++i) {
        initVector->at(i)->DestroyContext(newVector->at(i));
    }
    ASSERT_EQ(expectedInfoCount, infos.size());
}
// NOLINTEND
}  // namespace