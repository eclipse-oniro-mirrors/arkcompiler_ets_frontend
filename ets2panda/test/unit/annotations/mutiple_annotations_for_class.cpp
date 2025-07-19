/*
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
#include <gtest/gtest.h>
#include <string>
#include <utility>
#include <vector>
#include "assembly-program.h"
#include "test/unit/annotations/annotations_emit_test.h"

namespace ark::es2panda::compiler::test {

class MutipleAnnotationsforClass : public AnnotationEmitTest {
public:
    MutipleAnnotationsforClass() = default;

    ~MutipleAnnotationsforClass() override = default;

    void RunAnnotationEmitTest(const std::string_view text)
    {
        auto program = GetCurrentProgram(text);
        ASSERT_NE(program, nullptr);

        CheckAnnotations(program.get());
        CheckClassAnnotations(program.get());
        CheckLiteralArrayTable(program.get());
    }

    void CheckAnnotations(pandasm::Program *program)
    {
        const std::string annoName = "Anno2";
        const std::vector<std::pair<std::string, std::string>> expectedAnnotations = {
            {
                {"favorColor", "1"},
                {"color", "ETSGLOBAL%%annotation-Anno2-color-0"},
                {"reviewers", "ETSGLOBAL%%annotation-Anno2-reviewers-1"},
            },
        };
        AnnotationEmitTest::CheckAnnoDecl(program, annoName, expectedAnnotations);
    }

    void CheckClassAnnotations(pandasm::Program *program)
    {
        const std::string recordName = "A";
        const AnnotationMap expectedClassAnnotations = {
            {"Anno1",
             {
                 {"authorName", "Mike"},
                 {"authorAge", "18.000000"},
                 {"testBool", "1"},
             }},
            {"Anno2",
             {
                 {"favorColor", "1"},
                 {"color", "A%%annotation-Anno2-color-2"},
                 {"reviewers", "A%%annotation-Anno2-reviewers-3"},
             }},
            {"Anno3",
             {
                 {"reviewersAge", "A%%annotation-Anno3-reviewersAge-4"},
                 {"testBools", "A%%annotation-Anno3-testBools-5"},
                 {"mutiArray", "A%%annotation-Anno3-mutiArray-9"},
             }},
        };
        AnnotationEmitTest::CheckRecordAnnotations(program, recordName, expectedClassAnnotations);
    }

    void CheckLiteralArrayTable(pandasm::Program *program)
    {
        ExpectedLiteralArrayTable const expectedLiteralArrayTable = {
            {"ETSGLOBAL%%annotation-Anno2-color-0", {COLOR_0, COLOR_1}},
            {"ETSGLOBAL%%annotation-Anno2-reviewers-1", {"Bob", "Jim", "Tom"}},
            {"A%%annotation-Anno2-color-2", {COLOR_0, COLOR_1}},
            {"A%%annotation-Anno2-reviewers-3", {"Bob", "Jim", "Tom"}},
            {"A%%annotation-Anno3-reviewersAge-4", {REVIEWER_AGE_19, REVIEWER_AGE_20, REVIEWER_AGE_24}},
            {"A%%annotation-Anno3-testBools-5", {true, true, true}},
            {"A%%annotation-Anno3-mutiArray-6", {VALUE_9, VALUE_8, VALUE_7}},
            {"A%%annotation-Anno3-mutiArray-7", {VALUE_6, VALUE_5, VALUE_4}},
            {"A%%annotation-Anno3-mutiArray-8", {VALUE_3, VALUE_2, VALUE_1}},
            {"A%%annotation-Anno3-mutiArray-9",
             {
                 "A%%annotation-Anno3-mutiArray-6",
                 "A%%annotation-Anno3-mutiArray-7",
                 "A%%annotation-Anno3-mutiArray-8",
             }},
        };
        AnnotationEmitTest::CheckLiteralArrayTable(program, expectedLiteralArrayTable);
    }

private:
    NO_COPY_SEMANTIC(MutipleAnnotationsforClass);
    NO_MOVE_SEMANTIC(MutipleAnnotationsforClass);
};

TEST_F(MutipleAnnotationsforClass, mutiple_annotations_for_class)
{
    std::string_view text = R"(
        enum Color{RED, BLUE, GREEN}

        @interface Anno1 {
            authorName: string
            authorAge: number
            testBool: boolean
        }

        @interface Anno2 {
            favorColor: Color = Color.BLUE
            color: Color[] = [Color.RED, Color.BLUE]
            reviewers: string[] = ["Bob", "Jim", "Tom"]
        }

        @interface Anno3 {
            reviewersAge: number[]
            testBools: boolean[]
            mutiArray: number[][]
        }

        @Anno1({
            authorName: "Mike",
            authorAge: 18,
            testBool: true
        })
        @Anno2
        @Anno3({
            reviewersAge: [19, 20, 24],
            testBools: [true, true, true],
            mutiArray: [
                [9, 8, 7],
                [6, 5, 4],
                [3, 2, 1]
            ]
        })
        class A {}
    )";

    RunAnnotationEmitTest(text);
}

}  // namespace ark::es2panda::compiler::test