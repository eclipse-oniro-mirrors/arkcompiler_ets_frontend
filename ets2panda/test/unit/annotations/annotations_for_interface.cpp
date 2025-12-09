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
#include "assembly-program.h"
#include "test/unit/annotations/annotations_emit_test.h"

namespace ark::es2panda::compiler::test {

class AnnotationsforInterface : public AnnotationEmitTest {
public:
    AnnotationsforInterface() = default;

    ~AnnotationsforInterface() override = default;

    void RunAnnotationEmitTest(const std::string_view text)
    {
        auto program = GetCurrentProgram(text);
        ASSERT_NE(program, nullptr);

        CheckAnnotations(program.get());
        CheckInterfaceAnnotations(program.get());
        CheckFunctionAnnotations(program.get());
    }

    void CheckAnnotations(pandasm::Program *program)
    {
        const std::string annoName = "Anno";
        const std::vector<std::pair<std::string, std::string>> expectedAnnotations = {
            {
                {"authorName", ""},
                {"authorAge", "0"},
                {"testBool", "0"},
            },
        };
        AnnotationEmitTest::CheckAnnoDecl(program, annoName, expectedAnnotations);
    }

    void CheckInterfaceAnnotations(pandasm::Program *program)
    {
        const std::string recordName = "A";
        const AnnotationMap expectedClassAnnotations = {
            {"Anno",
             {
                 {"authorName", "Mike"},
                 {"authorAge", "18"},
                 {"testBool", "1"},
             }},
        };
        AnnotationEmitTest::CheckRecordAnnotations(program, recordName, expectedClassAnnotations);
    }

    void CheckFunctionAnnotations(pandasm::Program *program)
    {
        const std::string getter = "A.%%get-x:i32;";
        const std::string setter = "A.%%set-x:i32;void;";
        const std::string funcName = "A.foo:void;";
        const AnnotationMap expectedFuncAnnotations1 = {
            {"Anno",
             {
                 {"authorName", ""},
                 {"authorAge", "0"},
                 {"testBool", "0"},
             }},
        };
        const AnnotationMap expectedFuncAnnotations2 = {
            {"Anno",
             {
                 {"authorName", "John"},
                 {"authorAge", "23"},
                 {"testBool", "0"},
             }},
        };
        AnnotationEmitTest::CheckFunctionAnnotations(program, getter, false, expectedFuncAnnotations1);
        AnnotationEmitTest::CheckFunctionAnnotations(program, setter, false, expectedFuncAnnotations1);
        AnnotationEmitTest::CheckFunctionAnnotations(program, funcName, false, expectedFuncAnnotations2);
    }

private:
    NO_COPY_SEMANTIC(AnnotationsforInterface);
    NO_MOVE_SEMANTIC(AnnotationsforInterface);
};

TEST_F(AnnotationsforInterface, annotations_for_interface)
{
    std::string_view text = R"(
    @interface Anno {
        authorName: string = ""
        authorAge: int = 0
        testBool: boolean = false
    }

    @Anno({
        authorName: "Mike",
        authorAge: 18,
        testBool: true
    })
    interface A {
        @Anno
        x:int

        @Anno({
            authorName: "John",
            authorAge: 23,
            testBool: false
        })
        foo()
    })";

    RunAnnotationEmitTest(text);
}

}  // namespace ark::es2panda::compiler::test
