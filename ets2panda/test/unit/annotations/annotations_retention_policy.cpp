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

#include <string>

#include <gtest/gtest.h>

#include "assembly-program.h"
#include "test/unit/annotations/annotations_emit_test.h"

namespace ark::es2panda::compiler::test {

class AnnotationRetentionPolicy : public AnnotationEmitTest {
public:
    AnnotationRetentionPolicy() = default;

    ~AnnotationRetentionPolicy() override = default;

    void RunAnnotationEmitTest(const std::string_view text)
    {
        auto program = GetCurrentProgram(text);
        ASSERT_NE(program, nullptr);

        CheckRecordAnnotations(program.get());
        CheckFunctionAnnotations(program.get());
        CheckClasspropertyAnnotations(program.get());
        CheckFunctionParameterAnnotations(program.get());
    }

    void CheckRecordAnnotations(pandasm::Program *program)
    {
        const std::string recordName1 = "A";
        AnnotationEmitTest::CheckRecordWithoutAnnotations(program, recordName1);
        const std::string recordName2 = "NS";
        AnnotationEmitTest::CheckRecordWithoutAnnotations(program, recordName2, true);
    }

    void CheckFunctionAnnotations(pandasm::Program *program)
    {
        const std::string foo1 = "ETSGLOBAL.foo:f64;void;";
        const std::string foo2 = "A.foo:std.core.String;void;";
        AnnotationEmitTest::CheckFunctionWithoutAnnotations(program, foo1, true);
        AnnotationEmitTest::CheckFunctionWithoutAnnotations(program, foo2, false);
    }

    void CheckClasspropertyAnnotations(pandasm::Program *program)
    {
        const std::string recordName = "A";
        const std::string fieldName = "a";
        AnnotationEmitTest::CheckClassFieldWithoutAnnotations(program, recordName, fieldName);
    }

    void CheckFunctionParameterAnnotations(pandasm::Program *program)
    {
        const std::string foo1 = "ETSGLOBAL.foo:f64;void;";
        const std::string foo2 = "A.foo:std.core.String;void;";
        AnnotationEmitTest::CheckFunctionParameterWithoutAnnotations(program, foo1, true, 0);
        AnnotationEmitTest::CheckFunctionParameterWithoutAnnotations(program, foo2, false, 0);
    }

private:
    NO_COPY_SEMANTIC(AnnotationRetentionPolicy);
    NO_MOVE_SEMANTIC(AnnotationRetentionPolicy);
};

TEST_F(AnnotationRetentionPolicy, annotations_retention_policy)
{
    std::string_view text = R"(
    @Retention("SOURCE")
    @interface Anno{}

    @Anno()
    function foo(@Anno() a: number) {}

    @Anno()
    class A {
        @Anno()
        a:string = ""

        @Anno()
        foo(@Anno() a: string) {}
    }

    @Anno()
    namespace NS{})";

    RunAnnotationEmitTest(text);
}

}  // namespace ark::es2panda::compiler::test