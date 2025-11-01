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

class AnnotationsforClassMethod : public AnnotationEmitTest {
public:
    AnnotationsforClassMethod() = default;

    ~AnnotationsforClassMethod() override = default;

    void RunAnnotationEmitTest(const std::string_view text)
    {
        auto program = GetCurrentProgram(text);
        ASSERT_NE(program, nullptr);

        CheckFunctionAnnotations(program.get());
    }

    void CheckFunctionAnnotations(pandasm::Program *program)
    {
        const std::string funcNameOfClassA = "A.test:void;";
        const AnnotationMap expectedFuncAnnoOfClassA = {
            {"TEST",
             {
                 {"value", "test-path"},
             }},
        };

        const std::string funcNameOfClassB = "B.test:void;";
        const AnnotationMap expectedFuncAnnoOfClassB = {
            {"TEST",
             {
                 {"value", "inner-path"},
             }},
        };

        const std::string funcNameOfClassC = "C.test:void;";
        const AnnotationMap expectedFuncAnnoOfClassC = {
            {"TEST",
             {
                 {"value", "inner-path-namespace"},
             }},
        };
        AnnotationEmitTest::CheckFunctionAnnotations(program, funcNameOfClassA, false, expectedFuncAnnoOfClassA);
        AnnotationEmitTest::CheckFunctionAnnotations(program, funcNameOfClassB, false, expectedFuncAnnoOfClassB);
        AnnotationEmitTest::CheckFunctionAnnotations(program, funcNameOfClassC, false, expectedFuncAnnoOfClassC);
    }

private:
    NO_COPY_SEMANTIC(AnnotationsforClassMethod);
    NO_MOVE_SEMANTIC(AnnotationsforClassMethod);
};

TEST_F(AnnotationsforClassMethod, annotations_for_class_method)
{
    std::string_view text = R"(
    namespace NS {
        export const TS = 'inner-path-namespace'
    }

    @interface TEST {
        path: string
    }

    const TS: string = 'test-path'
    class A {
        TS: string = "xxx"
        @TEST(TS)
        test() {}
    }
    
    class B {
        readonly static TS: string = 'inner-path'
        @TEST(B.TS)
        test() {}
    }

    class C {
        TS: string = 'inner-path'
        @TEST(NS.TS)
        test() {}
    }
    )";

    RunAnnotationEmitTest(text);
}

}  // namespace ark::es2panda::compiler::test