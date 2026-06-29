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

#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include "assembly-function.h"
#include "assembly-parser.h"
#include "test/utils/metadata_test.h"
#include "flatbuffers/flatbuffers.h"
#include "schemaMetadataGenerated.h"
#include "utils/assertions.h"

namespace ark::es2panda::compiler::test {

using namespace Metadata;
using namespace metadata_test;

using Metadata::GetDecls, Metadata::BuiltinTypeKind, Metadata::TypeParamDecl;

class MetadataTestSerialization : public ::test::utils::MetadataTest {
public:
    MetadataTestSerialization() = default;
    ~MetadataTestSerialization() override = default;

private:
    NO_COPY_SEMANTIC(MetadataTestSerialization);
    NO_MOVE_SEMANTIC(MetadataTestSerialization);
};

TEST_F(MetadataTestSerialization, basic)
{
    const auto program =
        RunCheckerWithMetadata(std::string(TEST_DATA_PATH) + "serialization/" + test_info_->name() + ".ets");

    MetadataAssertions::AssertClassPresented(program.get(), "MyClass");
}

TEST_F(MetadataTestSerialization, method)
{
    const auto program =
        RunCheckerWithMetadata(std::string(TEST_DATA_PATH) + "serialization/" + test_info_->name() + ".ets");

    MetadataAssertions::AssertMethodPresented(program.get(), "MyClass", "hehehe");
}

TEST_F(MetadataTestSerialization, several_methods)
{
    const auto program =
        RunCheckerWithMetadata(std::string(TEST_DATA_PATH) + "serialization/" + test_info_->name() + ".ets");

    MetadataAssertions::AssertMethodPresented(program.get(), "MyClass", "hehehe");
    MetadataAssertions::AssertMethodPresented(program.get(), "MyClass", "hohoho");
    MetadataAssertions::AssertMethodPresented(program.get(), "MyClass", "hahaha");
}

TEST_F(MetadataTestSerialization, method_return_type_builtin)
{
    const auto program =
        RunCheckerWithMetadata(std::string(TEST_DATA_PATH) + "serialization/" + test_info_->name() + ".ets");

    MetadataAssertions::AssertBuiltinReturnTypeForMethod(program.get(), "MyClass", "hehehe", BuiltinTypeKind_void_);
    MetadataAssertions::AssertBuiltinReturnTypeForMethod(program.get(), "MyClass", "hohoho", BuiltinTypeKind_null);
    MetadataAssertions::AssertBuiltinReturnTypeForMethod(program.get(), "MyClass", "hahaha", BuiltinTypeKind_undefined);
    MetadataAssertions::AssertBuiltinReturnTypeForMethod(program.get(), "MyClass", "foo", BuiltinTypeKind_any);
    MetadataAssertions::AssertBuiltinReturnTypeForMethod(program.get(), "MyClass", "fee", BuiltinTypeKind_never);
    MetadataAssertions::AssertRefReturnTypeForMethod(program.get(), "MyClass", "faa", "String");
    MetadataAssertions::AssertRefReturnTypeForMethod(program.get(), "MyClass", "lol", "BigInt");
    MetadataAssertions::AssertRefReturnTypeForMethod(program.get(), "MyClass", "lel", "Object");
}

TEST_F(MetadataTestSerialization, method_return_type_primitive)
{
    const auto program =
        RunCheckerWithMetadata(std::string(TEST_DATA_PATH) + "serialization/" + test_info_->name() + ".ets");

    MetadataAssertions::AssertRefReturnTypeForMethod(program.get(), "MyClass", "hehehe", "Int");
    MetadataAssertions::AssertRefReturnTypeForMethod(program.get(), "MyClass", "hohoho", "Long");
    MetadataAssertions::AssertRefReturnTypeForMethod(program.get(), "MyClass", "hahaha", "Double");
    MetadataAssertions::AssertRefReturnTypeForMethod(program.get(), "MyClass", "foo", "Boolean");
    MetadataAssertions::AssertRefReturnTypeForMethod(program.get(), "MyClass", "fee", "Byte");
    MetadataAssertions::AssertRefReturnTypeForMethod(program.get(), "MyClass", "faa", "Short");
    MetadataAssertions::AssertRefReturnTypeForMethod(program.get(), "MyClass", "lol", "Char");
    MetadataAssertions::AssertRefReturnTypeForMethod(program.get(), "MyClass", "lel", "Float");
}

TEST_F(MetadataTestSerialization, method_return_type_string_literal)
{
    const auto program =
        RunCheckerWithMetadata(std::string(TEST_DATA_PATH) + "serialization/" + test_info_->name() + ".ets");

    MetadataAssertions::AssertClassPresented(program.get(), "MyClass");
    MetadataAssertions::AssertMethodPresented(program.get(), "MyClass", "getString");
    MetadataAssertions::AssertMethodReturnTypeStringLiteral(program.get(), "MyClass", "getString", "hello world");
}

TEST_F(MetadataTestSerialization, several_methods_return_type_string_literal)
{
    const auto program =
        RunCheckerWithMetadata(std::string(TEST_DATA_PATH) + "serialization/" + test_info_->name() + ".ets");

    MetadataAssertions::AssertClassPresented(program.get(), "MyClass");
    MetadataAssertions::AssertMethodPresented(program.get(), "MyClass", "getFirst");
    MetadataAssertions::AssertMethodPresented(program.get(), "MyClass", "getSecond");
    MetadataAssertions::AssertMethodPresented(program.get(), "MyClass", "getNumber");

    // Verify string literal return types
    MetadataAssertions::AssertMethodReturnTypeStringLiteral(program.get(), "MyClass", "getFirst", "first");
    MetadataAssertions::AssertMethodReturnTypeStringLiteral(program.get(), "MyClass", "getSecond", "second");
}

TEST_F(MetadataTestSerialization, method_type_params)
{
    const auto program =
        RunCheckerWithMetadata(std::string(TEST_DATA_PATH) + "serialization/" + test_info_->name() + ".ets");

    MetadataAssertions::AssertTypeParamPresented(program.get(), "MyClass", "hehehe", "T");
    MetadataAssertions::AssertTypeParamPresented(program.get(), "MyClass", "hehehe", "K");
}

TEST_F(MetadataTestSerialization, annotation)
{
    const auto program =
        RunCheckerWithMetadata(std::string(TEST_DATA_PATH) + "serialization/" + test_info_->name() + ".ets");

    MetadataAssertions::AssertClassPresented(program.get(), "MyClass");
    MetadataAssertions::AssertAnnotationPresented(program.get(), "MyAnnotation");
    constexpr auto numberOfAnnotations = 3;
    MetadataAssertions::AssertAnnotationsCount(program.get(), numberOfAnnotations);
}

TEST_F(MetadataTestSerialization, annotation_not_exported)
{
    const auto program =
        RunCheckerWithMetadata(std::string(TEST_DATA_PATH) + "serialization/" + test_info_->name() + ".ets");

    MetadataAssertions::AssertAnnotationNotPresented(program.get(), "InternalAnnotation");
    MetadataAssertions::AssertClassPresented(program.get(), "MyClass");
    MetadataAssertions::AssertAnnotationsCount(program.get(), 0);
}

}  // namespace ark::es2panda::compiler::test