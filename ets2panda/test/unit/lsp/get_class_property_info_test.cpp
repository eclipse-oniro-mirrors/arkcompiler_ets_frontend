/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "es2panda.h"
#include "lsp/include/api.h"
#include "lsp/include/get_class_property_info.h"
#include "lsp/include/cancellation_token.h"
#include "lsp_api_test.h"
#include <iostream>

namespace {
// NOLINTBEGIN
using ark::es2panda::lsp::Initializer;

class LspGetClassPropertyInfoTests : public LSPAPITests {};
std::vector<std::string> fileContents = {
    R"(
enum BloodType {
  A = 'A',
  AB = 'AB'
}

class Address {
  province: string = '';
  city: string = '';
}

interface Control {
  state: number
}

interface SelectableControl extends Control {
  select(): void
}

class SelectableControlClass extends Address implements SelectableControl {
  select(): void {
    throw new Error("Method not implemented.");
  }

  private state1: number = 0;
  protected readonly hobbies: string[] = [];
}

enum Sex {
  Male = 'Male'
}

export class Person extends SelectableControlClass implements SelectableControl {
  static MAX_HEIGHT: number = 250;
  static BLOOD_TYPES: BloodType = BloodType.AB;
  static defaultAddress: Address = {
    province: '北京',
    city: '北京市',
  };
  name: string = '';
  age: number = Person.MAX_HEIGHT;
  weight: number = 0;
  sex: Sex = Sex.Male;
  bloodType: BloodType = BloodType.A;
  address: Address = new Address();
  hobbies: string[] = [];
  maritalStatus: 'single' | 'married' | 'divorced' = 'single';
  birthday: Date = new Date();
  location: [number, number] = [0, 0];
  avatar: Resource = $r('app.media.startIcon');
  attributes: Map<string, object> = new Map();
  isEmployed: boolean = false;
  private privateIsEmployed: boolean = false;
  protected protectedIsEmployed: boolean = false;
  protected readonly readonlyIsEmployed: boolean = false;
  onUpdate: (() => void) | null = null;
}
)"};
std::vector<std::string> fileContents2 = {
    R"(
export enum BloodType {
  A = 'A',
  AB = 'AB'
}

export class Address {
  state: number = 1;
  province: string = '';
  city: string = '';
}

interface Control {
  state: number
}

export interface SelectableControl extends Control {
  select(): void
}
)",
    R"(
import { Address, SelectableControl } from "./GetClassPropertyInfo_SelectableControl";

export class SelectableControlClass extends Address implements SelectableControl {
  select(): void {
    throw new Error("Method not implemented.");
  }

  private state1: number = 0;
  protected readonly hobbies: string[] = [];
}
)",
    R"(
import { Address, BloodType, SelectableControl } from "./GetClassPropertyInfo_SelectableControl";
import { SelectableControlClass } from "./GetClassPropertyInfo_SelectableControlClass";

enum Sex {
  Male = 'Male'
}

export class Person extends SelectableControlClass implements SelectableControl {
  static MAX_HEIGHT: number = 250;
  static BLOOD_TYPES: BloodType = BloodType.AB;
  static defaultAddress: Address = {
    province: '北京',
    city: '北京市',
    state: 0
  };
  name: string = '';
  age: number = Person.MAX_HEIGHT;
  weight: number = 0;
  sex: Sex = Sex.Male;
  bloodType: BloodType = BloodType.A;
  address: Address = new Address();
  hobbies: string[] = [];
  maritalStatus: 'single' | 'married' | 'divorced' = 'single';
  birthday: Date = new Date();
  location: [number, number] = [0, 0];
  avatar: Resource = $r('app.media.startIcon');
  attributes: Map<string, object> = new Map();
  isEmployed: boolean = false;
  private privateIsEmployed: boolean = false;
  protected protectedIsEmployed: boolean = false;
  protected readonly readonlyIsEmployed: boolean = false;
  onUpdate: (() => void) | null = null;
}
)"};

std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> expectedResultWangz1 = {
    {"MAX_HEIGHT", 561, 585, "classField", {"public", "static"}},
    {"BLOOD_TYPES", 596, 633, "classField", {"public", "static"}},
    {"defaultAddress", 644, 722, "classField", {"public", "static"}},
    {"name", 726, 743, "classField", {"public"}},
    {"age", 747, 778, "classField", {"public"}},
    {"weight", 782, 800, "classField", {"public"}},
    {"sex", 804, 823, "classField", {"public"}},
    {"bloodType", 827, 861, "classField", {"public"}},
    {"address", 865, 898, "classField", {"public"}},
    {"hobbies", 901, 923, "classField", {"public"}},
    {"maritalStatus", 927, 986, "classField", {"public"}},
    {"birthday", 990, 1018, "classField", {"public"}},
    {"location", 1021, 1056, "classField", {"public"}},
    {"avatar", 1060, 1104, "classField", {"public"}},
    {"attributes", 1108, 1152, "classField", {"public"}},
    {"isEmployed", 1155, 1182, "classField", {"public"}},
    {"privateIsEmployed", 1194, 1228, "classField", {"private"}},
    {"protectedIsEmployed", 1242, 1278, "classField", {"protected"}},
    {"readonlyIsEmployed", 1301, 1336, "classField", {"protected", "readonly"}},
    {"onUpdate", 1340, 1376, "classField", {"public"}}};

void CheckClassPropertiesMatch(
    const std::vector<FieldListProperty> &actualProperties,
    const std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> &expectedResult)
{
    for (size_t i = 0; i < actualProperties.size(); ++i) {
        const auto &perp = actualProperties[i];
        const auto &expected = expectedResult[i];

        const auto &expectedName = std::get<0>(expected);
        const auto expectedStart = std::get<1>(expected);
        const auto expectedEnd = std::get<2>(expected);
        const auto &expectedKind = std::get<3>(expected);
        const auto &expectedModifiers = std::get<4>(expected);

        bool nameMatch = (perp.displayName == expectedName);
        bool startMatch = (perp.start == expectedStart);
        bool endMatch = (perp.end == expectedEnd);
        bool kindMatch = (perp.kind == expectedKind);

        bool modifiersMatch = true;
        if (perp.modifierKinds.has_value()) {
            const auto &actualModifiers = perp.modifierKinds.value();
            modifiersMatch = (actualModifiers == expectedModifiers);  // 严格比较顺序
        } else {
            modifiersMatch = expectedModifiers.empty();
        }

        bool currentMatch = nameMatch && startMatch && endMatch && kindMatch && modifiersMatch;
        ASSERT_EQ(true, currentMatch);
    }
}

TEST_F(LspGetClassPropertyInfoTests, GetClassPropertyInfoMethodTest1)
{
    constexpr size_t EXPECTED_CLASS_COUNT = 3;
    constexpr size_t EXPECTED_CLASS_COUNT_ONE = 1;
    constexpr size_t EXPECTED_PROP_COUNT = 20;

    std::vector<std::string> fileNames = {"GetClassPropertyInfoMethodFile1.ets"};

    auto filePaths = CreateTempFile(fileNames, fileContents);
    std::vector<ark::es2panda::SourceFile> sourceFiles;

    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents[i]);
    }
    ASSERT_EQ(fileNames.size(), sourceFiles.size());

    Initializer initializer;
    size_t sourceIndex = 0;
    size_t tokenOffset = 800;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());

    auto infos = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset, true);
    ASSERT_EQ(EXPECTED_CLASS_COUNT, infos.size());

    auto infos2 = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset);
    initializer.DestroyContext(context);
    ASSERT_EQ(EXPECTED_CLASS_COUNT_ONE, infos2.size());

    FieldsInfo info = infos2[0];
    ASSERT_EQ(EXPECTED_PROP_COUNT, info.properties.size());
    ASSERT_EQ("Person", info.name);
    CheckClassPropertiesMatch(info.properties, expectedResultWangz1);
}

std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> expectedResultWangz2 = {
    {"MAX_HEIGHT", 309, 333, "classField", {"public", "static"}},
    {"BLOOD_TYPES", 344, 381, "classField", {"public", "static"}},
    {"defaultAddress", 392, 483, "classField", {"public", "static"}},
    {"name", 487, 504, "classField", {"public"}},
    {"age", 508, 539, "classField", {"public"}},
    {"weight", 543, 561, "classField", {"public"}},
    {"sex", 565, 584, "classField", {"public"}},
    {"bloodType", 588, 622, "classField", {"public"}},
    {"address", 626, 659, "classField", {"public"}},
    {"hobbies", 662, 684, "classField", {"public"}},
    {"maritalStatus", 688, 747, "classField", {"public"}},
    {"birthday", 751, 779, "classField", {"public"}},
    {"location", 782, 817, "classField", {"public"}},
    {"avatar", 821, 865, "classField", {"public"}},
    {"attributes", 869, 913, "classField", {"public"}},
    {"isEmployed", 916, 943, "classField", {"public"}},
    {"privateIsEmployed", 955, 989, "classField", {"private"}},
    {"protectedIsEmployed", 1003, 1039, "classField", {"protected"}},
    {"readonlyIsEmployed", 1062, 1097, "classField", {"protected", "readonly"}},
    {"onUpdate", 1101, 1137, "classField", {"public"}}};

TEST_F(LspGetClassPropertyInfoTests, GetClassPropertyInfoMethodTest2)
{
    constexpr size_t EXPECTED_CLASS_COUNT = 3;
    constexpr size_t EXPECTED_CLASS_COUNT_ONE = 1;
    constexpr size_t EXPECTED_PROP_COUNT = 20;

    std::vector<std::string> fileNames2 = {"GetClassPropertyInfo_SelectableControl.ets",
                                           "GetClassPropertyInfo_SelectableControlClass.ets",
                                           "GetClassPropertyInfo_Person.ets"};

    auto filePaths = CreateTempFile(fileNames2, fileContents2);
    std::vector<ark::es2panda::SourceFile> sourceFiles;

    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents2[i]);
    }
    ASSERT_EQ(fileNames2.size(), sourceFiles.size());

    Initializer initializer;
    size_t sourceIndex = 2;
    size_t tokenOffset = 300;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());

    auto infos = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset, true);
    ASSERT_EQ(EXPECTED_CLASS_COUNT, infos.size());

    auto infos2 = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset);
    initializer.DestroyContext(context);
    ASSERT_EQ(EXPECTED_CLASS_COUNT_ONE, infos2.size());

    FieldsInfo info = infos2[0];
    ASSERT_EQ(EXPECTED_PROP_COUNT, info.properties.size());
    ASSERT_EQ("Person", info.name);
    CheckClassPropertiesMatch(info.properties, expectedResultWangz2);
}

std::vector<std::string> fileContents3 = {
    R"(
class A {
}
)",
    R"(
class B {
  p1
  p2
  static p3
  readonly p4
  private p5
}
)",
    R"(
class KKK {
  tr:string;
  constructor(tr: string) {
      this.tr = tr;
  }
}

class NNN extends KKK {
}
)",
    R"(
class GGG {
  tr:string;
  constructor(tr: string) {
      this.tr = tr;
  }
}

class HHH extends GGG {
  pop:string
}
)",
    R"(
abstract class AA {
  test: number;
}

class NN extends AA {
  jkk: string;
}
)",
    R"(
abstract class AA2 {
  test: number;
  private kn: string;
  das: string;

  constructor(test: number, kn: string) {
      this.test = test;
      this.kn = kn;
  }
}

class NN2 extends AA2 {
  jkk: string;
  wwa: number;
}
)",
    R"(
class ok {
  test?:string;
}
)",
    R"(
class TY {
  private _a: string = "foo"
}
)",
    R"(
class TwY {
  static a: string = "foo"
}
)",
    R"(
class TwasY {
  a: string|number;
}
)",
    R"(
abstract class TY {
  abstract a: string|number;
}
)",
    R"(
class Demo {
  private name = 'jack';
  private age = 25;
  private doTask = () => 34;
}
)",
    R"(
interface AA {
  aa: number;
  getAA(): number;
}

class BaseNN {
  aa: number;

  constructor(aa: number) {
    this.aa = aa;
  }

  getAA(): number {
    return 1;
  }
}

class MM extends BaseNN implements AA {
  aa: number;
  getAA(): number {
    return super.getAA();
  }
}
)",
    R"(
import CommonEventManager from '@ohos.commonEventManager';

interface SubscribeInfoType {
  events: string[];
}

class CommonEventRegister {
  subscriber: CommonEventManager.CommonEventSubscriber | null = null;
  public subscribeInfo: SubscribeInfoType;
}
)"};

std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> expectedResult1 = {};

TEST_F(LspGetClassPropertyInfoTests, GetClassPropertyInfoMethod1)
{
    constexpr size_t EXPECTED_CLASS_COUNT = 1;
    constexpr size_t EXPECTED_PROP_COUNT = 0;

    std::vector<std::string> fileNames3 = {
        "get_class_property_info_test_m1_01.ets", "get_class_property_info_test_m1_02.ets",
        "get_class_property_info_test_m1_03.ets", "get_class_property_info_test_m1_04.ets",
        "get_class_property_info_test_m1_05.ets", "get_class_property_info_test_m1_06.ets",
        "get_class_property_info_test_m1_07.ets", "get_class_property_info_test_m1_08.ets",
        "get_class_property_info_test_m1_09.ets", "get_class_property_info_test_m1_10.ets",
        "get_class_property_info_test_m1_11.ets", "get_class_property_info_test_m1_12.ets",
        "get_class_property_info_test_m1_13.ets", "get_class_property_info_test_m1_14.ets"};

    auto filePaths = CreateTempFile(fileNames3, fileContents3);
    std::vector<ark::es2panda::SourceFile> sourceFiles;

    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents3[i]);
    }
    ASSERT_EQ(fileNames3.size(), sourceFiles.size());

    Initializer initializer;
    size_t sourceIndex = 0;
    size_t tokenOffset = 12;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());

    auto infos = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset, true);
    ASSERT_EQ(EXPECTED_CLASS_COUNT, infos.size());
    ASSERT_EQ(EXPECTED_PROP_COUNT, infos[0].properties.size());
    CheckClassPropertiesMatch(infos[0].properties, expectedResult1);
}

std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> expectedResult2 = {
    {"p1", 13, 15, "classField", {"public"}},
    {"p2", 18, 20, "classField", {"public"}},
    {"p3", 30, 32, "classField", {"public", "static"}},
    {"p4", 44, 46, "classField", {"public", "readonly"}},
    {"p5", 57, 59, "classField", {"private"}}};

TEST_F(LspGetClassPropertyInfoTests, GetClassPropertyInfoMethod2)
{
    constexpr size_t EXPECTED_CLASS_COUNT = 1;
    constexpr size_t EXPECTED_PROP_COUNT = 5;

    std::vector<std::string> fileNames3 = {
        "get_class_property_info_test_m2_01.ets", "get_class_property_info_test_m2_02.ets",
        "get_class_property_info_test_m2_03.ets", "get_class_property_info_test_m2_04.ets",
        "get_class_property_info_test_m2_05.ets", "get_class_property_info_test_m2_06.ets",
        "get_class_property_info_test_m2_07.ets", "get_class_property_info_test_m2_08.ets",
        "get_class_property_info_test_m2_09.ets", "get_class_property_info_test_m2_10.ets",
        "get_class_property_info_test_m2_11.ets", "get_class_property_info_test_m2_12.ets",
        "get_class_property_info_test_m2_13.ets", "get_class_property_info_test_m2_14.ets"};

    auto filePaths = CreateTempFile(fileNames3, fileContents3);
    std::vector<ark::es2panda::SourceFile> sourceFiles;

    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents3[i]);
    }
    ASSERT_EQ(fileNames3.size(), sourceFiles.size());

    Initializer initializer;
    size_t sourceIndex = 1;
    size_t tokenOffset = 61;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto infos = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset, true);
    ASSERT_EQ(EXPECTED_CLASS_COUNT, infos.size());
    ASSERT_EQ(EXPECTED_PROP_COUNT, infos[0].properties.size());
    CheckClassPropertiesMatch(infos[0].properties, expectedResult2);
}

std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> expectedResult3 = {};

TEST_F(LspGetClassPropertyInfoTests, GetClassPropertyInfoMethod3)
{
    constexpr size_t EXPECTED_CLASS_COUNT = 2;
    constexpr size_t EXPECTED_PROP_COUNT = 0;

    std::vector<std::string> fileNames3 = {
        "get_class_property_info_test_m3_01.ets", "get_class_property_info_test_m3_02.ets",
        "get_class_property_info_test_m3_03.ets", "get_class_property_info_test_m3_04.ets",
        "get_class_property_info_test_m3_05.ets", "get_class_property_info_test_m3_06.ets",
        "get_class_property_info_test_m3_07.ets", "get_class_property_info_test_m3_08.ets",
        "get_class_property_info_test_m3_09.ets", "get_class_property_info_test_m3_10.ets",
        "get_class_property_info_test_m3_11.ets", "get_class_property_info_test_m3_12.ets",
        "get_class_property_info_test_m3_13.ets", "get_class_property_info_test_m3_14.ets"};

    auto filePaths = CreateTempFile(fileNames3, fileContents3);
    std::vector<ark::es2panda::SourceFile> sourceFiles;

    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents3[i]);
    }
    ASSERT_EQ(fileNames3.size(), sourceFiles.size());

    Initializer initializer;
    size_t sourceIndex = 2;
    size_t tokenOffset = 104;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto infos = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset, true);
    ASSERT_EQ(EXPECTED_CLASS_COUNT, infos.size());
    ASSERT_EQ(EXPECTED_PROP_COUNT, infos[0].properties.size());
    CheckClassPropertiesMatch(infos[0].properties, expectedResult3);
}

std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> expectedResult4 = {
    {"pop", 107, 117, "classField", {"public"}}};

TEST_F(LspGetClassPropertyInfoTests, GetClassPropertyInfoMethod4)
{
    constexpr size_t EXPECTED_CLASS_COUNT = 2;
    constexpr size_t EXPECTED_PROP_COUNT = 1;

    std::vector<std::string> fileNames3 = {
        "get_class_property_info_test_m4_01.ets", "get_class_property_info_test_m4_02.ets",
        "get_class_property_info_test_m4_03.ets", "get_class_property_info_test_m4_04.ets",
        "get_class_property_info_test_m4_05.ets", "get_class_property_info_test_m4_06.ets",
        "get_class_property_info_test_m4_07.ets", "get_class_property_info_test_m4_08.ets",
        "get_class_property_info_test_m4_09.ets", "get_class_property_info_test_m4_10.ets",
        "get_class_property_info_test_m4_11.ets", "get_class_property_info_test_m4_12.ets",
        "get_class_property_info_test_m4_13.ets", "get_class_property_info_test_m4_14.ets"};

    auto filePaths = CreateTempFile(fileNames3, fileContents3);
    std::vector<ark::es2panda::SourceFile> sourceFiles;

    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents3[i]);
    }
    ASSERT_EQ(fileNames3.size(), sourceFiles.size());

    Initializer initializer;
    size_t sourceIndex = 3;
    size_t tokenOffset = 117;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto infos = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset, true);
    ASSERT_EQ(EXPECTED_CLASS_COUNT, infos.size());
    ASSERT_EQ(EXPECTED_PROP_COUNT, infos[0].properties.size());
    CheckClassPropertiesMatch(infos[0].properties, expectedResult4);
}

std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> expectedResult5 = {
    {"jkk", 64, 75, "classField", {"public"}}};

TEST_F(LspGetClassPropertyInfoTests, GetClassPropertyInfoMethod5)
{
    constexpr size_t EXPECTED_CLASS_COUNT = 2;
    constexpr size_t EXPECTED_PROP_COUNT = 1;

    std::vector<std::string> fileNames3 = {
        "get_class_property_info_test_m5_01.ets", "get_class_property_info_test_m5_02.ets",
        "get_class_property_info_test_m5_03.ets", "get_class_property_info_test_m5_04.ets",
        "get_class_property_info_test_m5_05.ets", "get_class_property_info_test_m5_06.ets",
        "get_class_property_info_test_m5_07.ets", "get_class_property_info_test_m5_08.ets",
        "get_class_property_info_test_m5_09.ets", "get_class_property_info_test_m5_10.ets",
        "get_class_property_info_test_m5_11.ets", "get_class_property_info_test_m5_12.ets",
        "get_class_property_info_test_m5_13.ets", "get_class_property_info_test_m5_14.ets"};

    auto filePaths = CreateTempFile(fileNames3, fileContents3);
    std::vector<ark::es2panda::SourceFile> sourceFiles;

    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents3[i]);
    }
    ASSERT_EQ(fileNames3.size(), sourceFiles.size());

    Initializer initializer;
    size_t sourceIndex = 4;
    size_t tokenOffset = 76;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto infos = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset, true);
    ASSERT_EQ(EXPECTED_CLASS_COUNT, infos.size());
    ASSERT_EQ(EXPECTED_PROP_COUNT, infos[0].properties.size());
    CheckClassPropertiesMatch(infos[0].properties, expectedResult5);
}

std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> expectedResult6 = {
    {"jkk", 195, 206, "classField", {"public"}}, {"wwa", 210, 221, "classField", {"public"}}};

TEST_F(LspGetClassPropertyInfoTests, GetClassPropertyInfoMethod6)
{
    constexpr size_t EXPECTED_CLASS_COUNT = 2;
    constexpr size_t EXPECTED_PROP_COUNT = 2;

    std::vector<std::string> fileNames3 = {
        "get_class_property_info_test_m6_01.ets", "get_class_property_info_test_m6_02.ets",
        "get_class_property_info_test_m6_03.ets", "get_class_property_info_test_m6_04.ets",
        "get_class_property_info_test_m6_05.ets", "get_class_property_info_test_m6_06.ets",
        "get_class_property_info_test_m6_07.ets", "get_class_property_info_test_m6_08.ets",
        "get_class_property_info_test_m6_09.ets", "get_class_property_info_test_m6_10.ets",
        "get_class_property_info_test_m6_11.ets", "get_class_property_info_test_m6_12.ets",
        "get_class_property_info_test_m6_13.ets", "get_class_property_info_test_m6_14.ets"};

    auto filePaths = CreateTempFile(fileNames3, fileContents3);
    std::vector<ark::es2panda::SourceFile> sourceFiles;

    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents3[i]);
    }
    ASSERT_EQ(fileNames3.size(), sourceFiles.size());

    Initializer initializer;
    size_t sourceIndex = 5;
    size_t tokenOffset = 222;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto infos = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset, true);
    ASSERT_EQ(EXPECTED_CLASS_COUNT, infos.size());
    ASSERT_EQ(EXPECTED_PROP_COUNT, infos[0].properties.size());
    CheckClassPropertiesMatch(infos[0].properties, expectedResult6);
}

std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> expectedResult7 = {
    {"test", 14, 26, "classField", {"public"}}};

TEST_F(LspGetClassPropertyInfoTests, GetClassPropertyInfoMethod7)
{
    constexpr size_t EXPECTED_CLASS_COUNT = 1;
    constexpr size_t EXPECTED_PROP_COUNT = 1;

    std::vector<std::string> fileNames3 = {
        "get_class_property_info_test_m7_01.ets", "get_class_property_info_test_m7_02.ets",
        "get_class_property_info_test_m7_03.ets", "get_class_property_info_test_m7_04.ets",
        "get_class_property_info_test_m7_05.ets", "get_class_property_info_test_m7_06.ets",
        "get_class_property_info_test_m7_07.ets", "get_class_property_info_test_m7_08.ets",
        "get_class_property_info_test_m7_09.ets", "get_class_property_info_test_m7_10.ets",
        "get_class_property_info_test_m7_11.ets", "get_class_property_info_test_m7_12.ets",
        "get_class_property_info_test_m7_13.ets", "get_class_property_info_test_m7_14.ets"};

    auto filePaths = CreateTempFile(fileNames3, fileContents3);
    std::vector<ark::es2panda::SourceFile> sourceFiles;

    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents3[i]);
    }
    ASSERT_EQ(fileNames3.size(), sourceFiles.size());

    Initializer initializer;
    size_t sourceIndex = 6;
    size_t tokenOffset = 27;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto infos = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset, true);
    ASSERT_EQ(EXPECTED_CLASS_COUNT, infos.size());
    ASSERT_EQ(EXPECTED_PROP_COUNT, infos[0].properties.size());
    CheckClassPropertiesMatch(infos[0].properties, expectedResult7);
}

std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> expectedResult8 = {
    {"_a", 22, 40, "classField", {"private"}}};

TEST_F(LspGetClassPropertyInfoTests, GetClassPropertyInfoMethod8)
{
    constexpr size_t EXPECTED_CLASS_COUNT = 1;
    constexpr size_t EXPECTED_PROP_COUNT = 1;

    std::vector<std::string> fileNames3 = {
        "get_class_property_info_test_m8_01.ets", "get_class_property_info_test_m8_02.ets",
        "get_class_property_info_test_m8_03.ets", "get_class_property_info_test_m8_04.ets",
        "get_class_property_info_test_m8_05.ets", "get_class_property_info_test_m8_06.ets",
        "get_class_property_info_test_m8_07.ets", "get_class_property_info_test_m8_08.ets",
        "get_class_property_info_test_m8_09.ets", "get_class_property_info_test_m8_10.ets",
        "get_class_property_info_test_m8_11.ets", "get_class_property_info_test_m8_12.ets",
        "get_class_property_info_test_m8_13.ets", "get_class_property_info_test_m8_14.ets"};

    auto filePaths = CreateTempFile(fileNames3, fileContents3);
    std::vector<ark::es2panda::SourceFile> sourceFiles;

    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents3[i]);
    }
    ASSERT_EQ(fileNames3.size(), sourceFiles.size());

    Initializer initializer;
    size_t sourceIndex = 7;
    size_t tokenOffset = 40;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto infos = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset, true);
    ASSERT_EQ(EXPECTED_CLASS_COUNT, infos.size());
    ASSERT_EQ(EXPECTED_PROP_COUNT, infos[0].properties.size());
    CheckClassPropertiesMatch(infos[0].properties, expectedResult8);
}

std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> expectedResult9 = {
    {"a", 22, 39, "classField", {"public", "static"}}};

TEST_F(LspGetClassPropertyInfoTests, GetClassPropertyInfoMethod9)
{
    constexpr size_t EXPECTED_CLASS_COUNT = 1;
    constexpr size_t EXPECTED_PROP_COUNT = 1;

    std::vector<std::string> fileNames3 = {
        "get_class_property_info_test_m9_01.ets", "get_class_property_info_test_m9_02.ets",
        "get_class_property_info_test_m9_03.ets", "get_class_property_info_test_m9_04.ets",
        "get_class_property_info_test_m9_05.ets", "get_class_property_info_test_m9_06.ets",
        "get_class_property_info_test_m9_07.ets", "get_class_property_info_test_m9_08.ets",
        "get_class_property_info_test_m9_09.ets", "get_class_property_info_test_m9_10.ets",
        "get_class_property_info_test_m9_11.ets", "get_class_property_info_test_m9_12.ets",
        "get_class_property_info_test_m9_13.ets", "get_class_property_info_test_m9_14.ets"};

    auto filePaths = CreateTempFile(fileNames3, fileContents3);
    std::vector<ark::es2panda::SourceFile> sourceFiles;

    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents3[i]);
    }
    ASSERT_EQ(fileNames3.size(), sourceFiles.size());

    Initializer initializer;
    size_t sourceIndex = 8;
    size_t tokenOffset = 39;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto infos = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset, true);
    ASSERT_EQ(EXPECTED_CLASS_COUNT, infos.size());
    ASSERT_EQ(EXPECTED_PROP_COUNT, infos[0].properties.size());
    CheckClassPropertiesMatch(infos[0].properties, expectedResult9);
}

std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> expectedResult10 = {
    {"a", 17, 33, "classField", {"public"}}};

TEST_F(LspGetClassPropertyInfoTests, GetClassPropertyInfoMethod10)
{
    constexpr size_t EXPECTED_CLASS_COUNT = 1;
    constexpr size_t EXPECTED_PROP_COUNT = 1;

    std::vector<std::string> fileNames3 = {
        "get_class_property_info_test_m10_01.ets", "get_class_property_info_test_m10_02.ets",
        "get_class_property_info_test_m10_03.ets", "get_class_property_info_test_m10_04.ets",
        "get_class_property_info_test_m10_05.ets", "get_class_property_info_test_m10_06.ets",
        "get_class_property_info_test_m10_07.ets", "get_class_property_info_test_m10_08.ets",
        "get_class_property_info_test_m10_09.ets", "get_class_property_info_test_m10_10.ets",
        "get_class_property_info_test_m10_11.ets", "get_class_property_info_test_m10_12.ets",
        "get_class_property_info_test_m10_13.ets", "get_class_property_info_test_m10_14.ets"};

    auto filePaths = CreateTempFile(fileNames3, fileContents3);
    std::vector<ark::es2panda::SourceFile> sourceFiles;

    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents3[i]);
    }
    ASSERT_EQ(fileNames3.size(), sourceFiles.size());

    Initializer initializer;
    size_t sourceIndex = 9;
    size_t tokenOffset = 35;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto infos = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset, true);
    ASSERT_EQ(EXPECTED_CLASS_COUNT, infos.size());
    ASSERT_EQ(EXPECTED_PROP_COUNT, infos[0].properties.size());
    CheckClassPropertiesMatch(infos[0].properties, expectedResult10);
}

std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> expectedResult11 = {
    {"a", 32, 48, "classField", {"public"}}};

TEST_F(LspGetClassPropertyInfoTests, GetClassPropertyInfoMethod11)
{
    constexpr size_t EXPECTED_CLASS_COUNT = 1;
    constexpr size_t EXPECTED_PROP_COUNT = 1;

    std::vector<std::string> fileNames3 = {
        "get_class_property_info_test_m11_01.ets", "get_class_property_info_test_m11_02.ets",
        "get_class_property_info_test_m11_03.ets", "get_class_property_info_test_m11_04.ets",
        "get_class_property_info_test_m11_05.ets", "get_class_property_info_test_m11_06.ets",
        "get_class_property_info_test_m11_07.ets", "get_class_property_info_test_m11_08.ets",
        "get_class_property_info_test_m11_09.ets", "get_class_property_info_test_m11_10.ets",
        "get_class_property_info_test_m11_11.ets", "get_class_property_info_test_m11_12.ets",
        "get_class_property_info_test_m11_13.ets", "get_class_property_info_test_m11_14.ets"};

    auto filePaths = CreateTempFile(fileNames3, fileContents3);
    std::vector<ark::es2panda::SourceFile> sourceFiles;

    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents3[i]);
    }
    ASSERT_EQ(fileNames3.size(), sourceFiles.size());

    Initializer initializer;
    size_t sourceIndex = 10;
    size_t tokenOffset = 50;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto infos = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset, true);
    ASSERT_EQ(EXPECTED_CLASS_COUNT, infos.size());
    ASSERT_EQ(EXPECTED_PROP_COUNT, infos[0].properties.size());
    CheckClassPropertiesMatch(infos[0].properties, expectedResult11);
}

std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> expectedResult12 = {
    {"name", 24, 37, "classField", {"private"}},
    {"age", 49, 57, "classField", {"private"}},
    {"doTask", 69, 86, "classField", {"private"}}};

TEST_F(LspGetClassPropertyInfoTests, GetClassPropertyInfoMethod12)
{
    constexpr size_t EXPECTED_CLASS_COUNT = 1;
    constexpr size_t EXPECTED_PROP_COUNT = 3;

    std::vector<std::string> fileNames3 = {
        "get_class_property_info_test_m12_01.ets", "get_class_property_info_test_m12_02.ets",
        "get_class_property_info_test_m12_03.ets", "get_class_property_info_test_m12_04.ets",
        "get_class_property_info_test_m12_05.ets", "get_class_property_info_test_m12_06.ets",
        "get_class_property_info_test_m12_07.ets", "get_class_property_info_test_m12_08.ets",
        "get_class_property_info_test_m12_09.ets", "get_class_property_info_test_m12_10.ets",
        "get_class_property_info_test_m12_11.ets", "get_class_property_info_test_m12_12.ets",
        "get_class_property_info_test_m12_13.ets", "get_class_property_info_test_m12_14.ets"};

    auto filePaths = CreateTempFile(fileNames3, fileContents3);
    std::vector<ark::es2panda::SourceFile> sourceFiles;

    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents3[i]);
    }
    ASSERT_EQ(fileNames3.size(), sourceFiles.size());

    Initializer initializer;
    size_t sourceIndex = 11;
    size_t tokenOffset = 87;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto infos = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset, true);
    ASSERT_EQ(EXPECTED_CLASS_COUNT, infos.size());
    ASSERT_EQ(EXPECTED_PROP_COUNT, infos[0].properties.size());
    CheckClassPropertiesMatch(infos[0].properties, expectedResult12);
}

std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> expectedResult13 = {
    {"aa", 216, 226, "classField", {"public"}}};

TEST_F(LspGetClassPropertyInfoTests, GetClassPropertyInfoMethod13)
{
    constexpr size_t EXPECTED_CLASS_COUNT = 2;
    constexpr size_t EXPECTED_PROP_COUNT = 1;

    std::vector<std::string> fileNames3 = {
        "get_class_property_info_test_m13_01.ets", "get_class_property_info_test_m13_02.ets",
        "get_class_property_info_test_m13_03.ets", "get_class_property_info_test_m13_04.ets",
        "get_class_property_info_test_m13_05.ets", "get_class_property_info_test_m13_06.ets",
        "get_class_property_info_test_m13_07.ets", "get_class_property_info_test_m13_08.ets",
        "get_class_property_info_test_m13_09.ets", "get_class_property_info_test_m13_10.ets",
        "get_class_property_info_test_m13_11.ets", "get_class_property_info_test_m13_12.ets",
        "get_class_property_info_test_m13_13.ets", "get_class_property_info_test_m13_14.ets"};

    auto filePaths = CreateTempFile(fileNames3, fileContents3);
    std::vector<ark::es2panda::SourceFile> sourceFiles;

    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents3[i]);
    }
    ASSERT_EQ(fileNames3.size(), sourceFiles.size());

    Initializer initializer;
    size_t sourceIndex = 12;
    size_t tokenOffset = 232;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto infos = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset, true);
    ASSERT_EQ(EXPECTED_CLASS_COUNT, infos.size());
    ASSERT_EQ(EXPECTED_PROP_COUNT, infos[0].properties.size());
    CheckClassPropertiesMatch(infos[0].properties, expectedResult13);
}

std::vector<std::tuple<std::string, size_t, size_t, std::string, std::vector<std::string>>> expectedResult14 = {
    {"subscriber", 144, 210, "classField", {"public"}}, {"subscribeInfo", 221, 253, "classField", {"public"}}};

TEST_F(LspGetClassPropertyInfoTests, GetClassPropertyInfoMethod14)
{
    constexpr size_t EXPECTED_CLASS_COUNT = 1;
    constexpr size_t EXPECTED_PROP_COUNT = 2;

    std::vector<std::string> fileNames3 = {
        "get_class_property_info_test_m14_01.ets", "get_class_property_info_test_m14_02.ets",
        "get_class_property_info_test_m14_03.ets", "get_class_property_info_test_m14_04.ets",
        "get_class_property_info_test_m14_05.ets", "get_class_property_info_test_m14_06.ets",
        "get_class_property_info_test_m14_07.ets", "get_class_property_info_test_m14_08.ets",
        "get_class_property_info_test_m14_09.ets", "get_class_property_info_test_m14_10.ets",
        "get_class_property_info_test_m14_11.ets", "get_class_property_info_test_m14_12.ets",
        "get_class_property_info_test_m14_13.ets", "get_class_property_info_test_m14_14.ets"};

    auto filePaths = CreateTempFile(fileNames3, fileContents3);
    std::vector<ark::es2panda::SourceFile> sourceFiles;

    for (size_t i = 0; i < filePaths.size(); ++i) {
        sourceFiles.emplace_back(filePaths[i], fileContents3[i]);
    }
    ASSERT_EQ(fileNames3.size(), sourceFiles.size());

    Initializer initializer;
    size_t sourceIndex = 13;
    size_t tokenOffset = 256;
    auto filePath = std::string {sourceFiles[sourceIndex].filePath};
    auto fileContent = std::string {sourceFiles[sourceIndex].source};
    auto context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto infos = ark::es2panda::lsp::GetClassPropertyInfo(context, tokenOffset, true);
    ASSERT_EQ(EXPECTED_CLASS_COUNT, infos.size());
    ASSERT_EQ(EXPECTED_PROP_COUNT, infos[0].properties.size());
    CheckClassPropertiesMatch(infos[0].properties, expectedResult14);
}

// NOLINTEND
}  // namespace