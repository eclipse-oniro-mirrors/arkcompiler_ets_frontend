/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include <gtest/gtest.h>
#include <string>
#include <utility>
#include <vector>
#include "assembly-literals.h"
#include "assembly-program.h"
#include "test/unit/annotations/annotations_emit_test.h"
#include "test/utils/asm_test.h"

namespace ark::es2panda::compiler::test {

namespace {

using namespace ::test::utils::literals;
// In case of an error, the test will display the actual initialization list,
// which you need to copy and paste to update the variable value.
static pandasm::Program::LiteralArrayTableT const EXPECTED_LITERAL_ARRAY = {
    {"ETSGLOBAL%%annotation-ClassAuthor-color-8",
     LA({{0x0_LT, 2_U8}, {0x2_LT, 0_U32}, {0x0_LT, 2_U8}, {0x2_LT, 1_U32}})},
    {"ETSGLOBAL%%annotation-ClassAuthor-mutiArray-12",
     LA({{0x0_LT, 4_U8}, {0x4_LT, 1.000000}, {0x0_LT, 4_U8}, {0x4_LT, 2.000000}, {0x0_LT, 4_U8}, {0x4_LT, 3.000000}})},
    {"ETSGLOBAL%%annotation-ClassAuthor-mutiArray-13",
     LA({{0x0_LT, 4_U8}, {0x4_LT, 4.000000}, {0x0_LT, 4_U8}, {0x4_LT, 5.000000}, {0x0_LT, 4_U8}, {0x4_LT, 6.000000}})},
    {"ETSGLOBAL%%annotation-ClassAuthor-mutiArray-14",
     LA({{0x0_LT, 4_U8}, {0x4_LT, 7.000000}, {0x0_LT, 4_U8}, {0x4_LT, 8.000000}, {0x0_LT, 4_U8}, {0x4_LT, 9.000000}})},
    {"ETSGLOBAL%%annotation-ClassAuthor-mutiArray-15",
     LA({{0x0_LT, 25_U8},
         {0x19_LT, "ETSGLOBAL%%annotation-ClassAuthor-mutiArray-12"},
         {0x0_LT, 25_U8},
         {0x19_LT, "ETSGLOBAL%%annotation-ClassAuthor-mutiArray-13"},
         {0x0_LT, 25_U8},
         {0x19_LT, "ETSGLOBAL%%annotation-ClassAuthor-mutiArray-14"}})},
    {"ETSGLOBAL%%annotation-ClassAuthor-reviewers-9",
     LA({{0x0_LT, 5_U8}, {0x5_LT, "Bob"}, {0x0_LT, 5_U8}, {0x5_LT, "Jim"}, {0x0_LT, 5_U8}, {0x5_LT, "Tom"}})},
    {"ETSGLOBAL%%annotation-ClassAuthor-reviewersAge-10", LA({{0x0_LT, 4_U8},
                                                              {0x4_LT, 18.000000},
                                                              {0x0_LT, 4_U8},
                                                              {0x4_LT, 21.000000},
                                                              {0x0_LT, 4_U8},
                                                              {0x4_LT, 32.000000}})},
    {"ETSGLOBAL%%annotation-ClassAuthor-testBools-11",
     LA({{0x0_LT, 1_U8}, {0x1_LT, false}, {0x0_LT, 1_U8}, {0x1_LT, true}, {0x0_LT, 1_U8}, {0x1_LT, false}})},
    {"MyClass%%annotation-ClassAuthor-color-20",
     LA({{0x0_LT, 2_U8}, {0x2_LT, 0_U32}, {0x0_LT, 2_U8}, {0x2_LT, 1_U32}})},
    {"MyClass%%annotation-ClassAuthor-mutiArray-16",
     LA({{0x0_LT, 4_U8}, {0x4_LT, 1.000000}, {0x0_LT, 4_U8}, {0x4_LT, 2.000000}, {0x0_LT, 4_U8}, {0x4_LT, 3.000000}})},
    {"MyClass%%annotation-ClassAuthor-mutiArray-17",
     LA({{0x0_LT, 4_U8}, {0x4_LT, 4.000000}, {0x0_LT, 4_U8}, {0x4_LT, 5.000000}, {0x0_LT, 4_U8}, {0x4_LT, 6.000000}})},
    {"MyClass%%annotation-ClassAuthor-mutiArray-18",
     LA({{0x0_LT, 4_U8}, {0x4_LT, 7.000000}, {0x0_LT, 4_U8}, {0x4_LT, 8.000000}, {0x0_LT, 4_U8}, {0x4_LT, 9.000000}})},
    {"MyClass%%annotation-ClassAuthor-mutiArray-19", LA({{0x0_LT, 25_U8},
                                                         {0x19_LT, "MyClass%%annotation-ClassAuthor-mutiArray-16"},
                                                         {0x0_LT, 25_U8},
                                                         {0x19_LT, "MyClass%%annotation-ClassAuthor-mutiArray-17"},
                                                         {0x0_LT, 25_U8},
                                                         {0x19_LT, "MyClass%%annotation-ClassAuthor-mutiArray-18"}})},
    {"MyClass%%annotation-ClassAuthor-reviewers-22",
     LA({{0x0_LT, 5_U8}, {0x5_LT, "Bob"}, {0x0_LT, 5_U8}, {0x5_LT, "Jim"}, {0x0_LT, 5_U8}, {0x5_LT, "Tom"}})},
    {"MyClass%%annotation-ClassAuthor-reviewersAge-23", LA({{0x0_LT, 4_U8},
                                                            {0x4_LT, 18.000000},
                                                            {0x0_LT, 4_U8},
                                                            {0x4_LT, 21.000000},
                                                            {0x0_LT, 4_U8},
                                                            {0x4_LT, 32.000000}})},
    {"MyClass%%annotation-ClassAuthor-testBools-21",
     LA({{0x0_LT, 1_U8}, {0x1_LT, false}, {0x0_LT, 1_U8}, {0x1_LT, true}, {0x0_LT, 1_U8}, {0x1_LT, false}})},
    {"MyClass.foo:void;%%annotation-ClassAuthor-color-4",
     LA({{0x0_LT, 2_U8}, {0x2_LT, 0_U32}, {0x0_LT, 2_U8}, {0x2_LT, 1_U32}})},
    {"MyClass.foo:void;%%annotation-ClassAuthor-mutiArray-0",
     LA({{0x0_LT, 4_U8}, {0x4_LT, 1.000000}, {0x0_LT, 4_U8}, {0x4_LT, 2.000000}, {0x0_LT, 4_U8}, {0x4_LT, 3.000000}})},
    {"MyClass.foo:void;%%annotation-ClassAuthor-mutiArray-1",
     LA({{0x0_LT, 4_U8}, {0x4_LT, 4.000000}, {0x0_LT, 4_U8}, {0x4_LT, 5.000000}, {0x0_LT, 4_U8}, {0x4_LT, 6.000000}})},
    {"MyClass.foo:void;%%annotation-ClassAuthor-mutiArray-2",
     LA({{0x0_LT, 4_U8}, {0x4_LT, 7.000000}, {0x0_LT, 4_U8}, {0x4_LT, 8.000000}, {0x0_LT, 4_U8}, {0x4_LT, 9.000000}})},
    {"MyClass.foo:void;%%annotation-ClassAuthor-mutiArray-3",
     LA({{0x0_LT, 25_U8},
         {0x19_LT, "MyClass.foo:void;%%annotation-ClassAuthor-mutiArray-0"},
         {0x0_LT, 25_U8},
         {0x19_LT, "MyClass.foo:void;%%annotation-ClassAuthor-mutiArray-1"},
         {0x0_LT, 25_U8},
         {0x19_LT, "MyClass.foo:void;%%annotation-ClassAuthor-mutiArray-2"}})},
    {"MyClass.foo:void;%%annotation-ClassAuthor-reviewers-6",
     LA({{0x0_LT, 5_U8}, {0x5_LT, "Bob"}, {0x0_LT, 5_U8}, {0x5_LT, "Jim"}, {0x0_LT, 5_U8}, {0x5_LT, "Tom"}})},
    {"MyClass.foo:void;%%annotation-ClassAuthor-reviewersAge-7", LA({{0x0_LT, 4_U8},
                                                                     {0x4_LT, 18.000000},
                                                                     {0x0_LT, 4_U8},
                                                                     {0x4_LT, 21.000000},
                                                                     {0x0_LT, 4_U8},
                                                                     {0x4_LT, 32.000000}})},
    {"MyClass.foo:void;%%annotation-ClassAuthor-testBools-5",
     LA({{0x0_LT, 1_U8}, {0x1_LT, false}, {0x0_LT, 1_U8}, {0x1_LT, true}, {0x0_LT, 1_U8}, {0x1_LT, false}})},
};

}  // namespace

class StandardEmitTest : public AnnotationEmitTest {
public:
    StandardEmitTest() = default;

    ~StandardEmitTest() override = default;

    void RunAnnotationEmitTest(const std::string_view text)
    {
        auto program = GetCurrentProgram(text);
        ASSERT_NE(program, nullptr);

        CheckAnnotations(program.get());
        CheckClassAnnotations(program.get());
        CheckFunctionAnnotations(program.get());
        CheckLiteralArrayTable(program.get());
    }

    void CheckAnnotations(pandasm::Program *program)
    {
        ASSERT_NE(program, nullptr);
        const std::string annoName = "ClassAuthor";
        const std::vector<std::pair<std::string, std::string>> expectedAnnotations = {
            {"authorName", "Jim"},
            {"authorAge", "35.000000"},
            {"testBool", "0"},
            {"favorColor", "1"},
            {"color", "ETSGLOBAL%%annotation-ClassAuthor-color-8"},
            {"reviewers", "ETSGLOBAL%%annotation-ClassAuthor-reviewers-9"},
            {"reviewersAge", "ETSGLOBAL%%annotation-ClassAuthor-reviewersAge-10"},
            {"testBools", "ETSGLOBAL%%annotation-ClassAuthor-testBools-11"},
            {"mutiArray", "ETSGLOBAL%%annotation-ClassAuthor-mutiArray-15"},
        };
        AnnotationEmitTest::CheckAnnoDecl(program, annoName, expectedAnnotations);
    }

    void CheckClassAnnotations(pandasm::Program *program)
    {
        ASSERT_NE(program, nullptr);
        const std::string recordName = "MyClass";
        const AnnotationMap expectedClassAnnotations = {
            {"ClassAuthor",
             {
                 {"authorName", "Jim"},
                 {"authorAge", "35.000000"},
                 {"testBool", "0"},
                 {"favorColor", "1"},
                 {"color", "MyClass%%annotation-ClassAuthor-color-20"},
                 {"reviewers", "MyClass%%annotation-ClassAuthor-reviewers-22"},
                 {"reviewersAge", "MyClass%%annotation-ClassAuthor-reviewersAge-23"},
                 {"testBools", "MyClass%%annotation-ClassAuthor-testBools-21"},
                 {"mutiArray", "MyClass%%annotation-ClassAuthor-mutiArray-19"},
             }},
        };
        AnnotationEmitTest::CheckRecordAnnotations(program, recordName, expectedClassAnnotations);
    }

    void CheckFunctionAnnotations(pandasm::Program *program)
    {
        ASSERT_NE(program, nullptr);
        const std::string functionName = "MyClass.foo:void;";
        const AnnotationMap expectedFuncAnnotations = {
            {"ClassAuthor",
             {
                 {"mutiArray", "MyClass.foo:void;%%annotation-ClassAuthor-mutiArray-3"},
                 {"color", "MyClass.foo:void;%%annotation-ClassAuthor-color-4"},
                 {"testBools", "MyClass.foo:void;%%annotation-ClassAuthor-testBools-5"},
                 {"reviewers", "MyClass.foo:void;%%annotation-ClassAuthor-reviewers-6"},
                 {"favorColor", "1"},
                 {"testBool", "0"},
                 {"reviewersAge", "MyClass.foo:void;%%annotation-ClassAuthor-reviewersAge-7"},
                 {"authorAge", "35.000000"},
                 {"authorName", "Jim"},
             }},
        };
        AnnotationEmitTest::CheckFunctionAnnotations(program, functionName, false, expectedFuncAnnotations);
    }

    void CheckLiteralArrayTable(pandasm::Program *program)
    {
        ASSERT_NE(program, nullptr);
        ExpectLiteralArrayTable(program, EXPECTED_LITERAL_ARRAY);
    }

private:
    NO_COPY_SEMANTIC(StandardEmitTest);
    NO_MOVE_SEMANTIC(StandardEmitTest);
};

TEST_F(StandardEmitTest, standard_test)
{
    std::string_view text = R"(
        enum Color{RED, BLUE, GREEN}

        @interface ClassAuthor {
        authorName: string = "Jim"
        authorAge: number = 35
        testBool: boolean = false
        favorColor: Color = Color.BLUE
        color: Color[] = [Color.RED, Color.BLUE]
        reviewers: string[] = ["Bob", "Jim", "Tom"]
        reviewersAge: number[] = [18, 21, 32]
        testBools: boolean[] = [false, true, false]
        mutiArray: number[][] = [
            [1, 2, 3],
            [4, 5, 6],
            [7, 8, 9]
        ]
    }
    @ClassAuthor()
    class MyClass {
        @ClassAuthor()
        foo() {}
    })";

    RunAnnotationEmitTest(text);
}

}  // namespace ark::es2panda::compiler::test
