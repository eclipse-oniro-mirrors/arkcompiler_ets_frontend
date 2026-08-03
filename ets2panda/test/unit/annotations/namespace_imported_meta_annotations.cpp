/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <algorithm>
#include <string>

#include <gtest/gtest.h>

#include "assembly-program.h"
#include "test/unit/annotations/annotations_emit_test.h"

namespace ark::es2panda::compiler::test {

class NamespaceImportedMetaAnnotations : public AnnotationEmitTest {
public:
    NamespaceImportedMetaAnnotations() = default;

    ~NamespaceImportedMetaAnnotations() override = default;

private:
    NO_COPY_SEMANTIC(NamespaceImportedMetaAnnotations);
    NO_MOVE_SEMANTIC(NamespaceImportedMetaAnnotations);
};

TEST_F(NamespaceImportedMetaAnnotations, EmitAnnotationWithNamespaceImportedMetaAnnotations)
{
    std::string_view text = R"(
        import * as annotations from "std/annotations"

        @annotations.Retention("RUNTIME")
        @annotations.Target([annotations.AnnotationTargets.CLASS])
        @interface ClassOnly {
            value: int = 1
        }

        @annotations.Retention("SOURCE")
        @interface SourceOnly {}

        @ClassOnly({value: 2})
        @SourceOnly
        class AnnotatedClass {}
    )";

    auto program = GetCurrentProgram(text);
    ASSERT_NE(program, nullptr);

    const AnnotationMap expectedAnnotations = {
        {"ClassOnly", {{"value", "2"}}},
    };
    CheckRecordAnnotations(program.get(), "AnnotatedClass", expectedAnnotations);

    const auto &record = program->recordTable.at("AnnotatedClass");
    const auto annotations = record.metadata->GetAnnotations();
    const auto sourceOnly =
        std::find_if(annotations.begin(), annotations.end(),
                     [](const pandasm::AnnotationData &anno) { return anno.GetName() == "SourceOnly"; });
    EXPECT_EQ(sourceOnly, annotations.end());
}

}  // namespace ark::es2panda::compiler::test
