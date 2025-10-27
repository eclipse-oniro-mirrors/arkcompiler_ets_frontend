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
#include <vector>
#include "assembly-program.h"
#include "test/unit/annotations/annotations_emit_test.h"

namespace ark::es2panda::compiler::test {

class ModuleAnnotations : public AnnotationEmitTest {
public:
    ModuleAnnotations() = default;

    ~ModuleAnnotations() override = default;

    void RunAnnotationEmitTest(const std::string_view text, const std::function<void(pandasm::Program *program)> &test)
    {
        auto program = GetCurrentProgram(text);
        ASSERT_NE(program, nullptr);

        test(program.get());
    }

private:
    NO_COPY_SEMANTIC(ModuleAnnotations);
    NO_MOVE_SEMANTIC(ModuleAnnotations);
};

TEST_F(ModuleAnnotations, module_annotation)
{
    std::string_view text = R"(
        class A {}
        export class ExportedClass {}
        namespace B {}
        export namespace ExportedNamespace {}
    )";

    auto test = [this](pandasm::Program *program) {
        AnnotationEmitTest::CheckModuleAnnotation(program, "ETSGLOBAL", true, {"ExportedClass", "ExportedNamespace"});
        AnnotationEmitTest::CheckModuleAnnotation(program, "ExportedClass", false);
        AnnotationEmitTest::CheckModuleAnnotation(program, "ExportedNamespace", true, {});
    };
    RunAnnotationEmitTest(text, test);
}

TEST_F(ModuleAnnotations, module_annotation_nested)
{
    std::string_view text = R"(
        export namespace ExportedNamespace {
            class A {}
            export class B {}
            namespace C {}
            export namespace D {
                export class A {}
            }
        }
    )";

    auto test = [this](pandasm::Program *program) {
        AnnotationEmitTest::CheckModuleAnnotation(program, "ETSGLOBAL", true, {"ExportedNamespace"});
        AnnotationEmitTest::CheckModuleAnnotation(program, "ExportedNamespace", true,
                                                  {"ExportedNamespace.B", "ExportedNamespace.D"});
        AnnotationEmitTest::CheckModuleAnnotation(program, "ExportedNamespace.D", true, {"ExportedNamespace.D.A"});
    };
    RunAnnotationEmitTest(text, test);
}

}  // namespace ark::es2panda::compiler::test