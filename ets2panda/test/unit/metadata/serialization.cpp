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
#include "assembly-program.h"
#include "test/utils/checker_test.h"
#include "flatbuffers/flatbuffers.h"
#include "schemaMetadataGenerated.h"
#include "util/generateBin.h"
#include "parser/program/ImportCache.h"

namespace ark::es2panda::compiler::test {

using Metadata::GetRoot, Metadata::BuiltinTypeKind, Metadata::TypeParamDecl;

class MetadataTestSerialization : public ::test::utils::CheckerTest {
public:
    MetadataTestSerialization() = default;

    ~MetadataTestSerialization() override = default;

    static void SetUpTestCase()
    {
        if (!ScopedAllocatorsManager::IsInitialized()) {
            ScopedAllocatorsManager::Initialize();
        }
    }

    static void AssertClassPresented(const pandasm::Program *program, const std::string &expectedClassName)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);

        for (const auto classDecl : *root->classes()) {
            if (classDecl->name()->str() == expectedClassName) {
                return;
            }
        }

        FAIL() << "Class " + expectedClassName + " not found in the metadata";
    }

    static void AssertMethodPresented(const pandasm::Program *program, const std::string &className,
                                      const std::string &expectedMethodName)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);
        for (const auto classDecl : *root->classes()) {
            if (classDecl->name()->str() != className) {
                continue;
            }
            for (auto method : *classDecl->methods()) {
                if (method->name()->str() == expectedMethodName) {
                    return;
                }
            }
        }

        FAIL() << "Method " + expectedMethodName + " not found in the metadata of class " << className;
    }

    static void AssertReturnTypeForMethod(const pandasm::Program *program, const std::string &className,
                                          const std::string &methodName, const BuiltinTypeKind expectedReturnType)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);
        for (const auto classDecl : *root->classes()) {
            if (classDecl->name()->str() != className) {
                continue;
            }
            for (auto method : *classDecl->methods()) {
                if (method->name()->str() != methodName) {
                    continue;
                }
                if (method->return_type_as_Builtin()->kind() == expectedReturnType) {
                    return;
                }
            }
        }

        FAIL() << "Method return type " << static_cast<int16_t>(expectedReturnType)
               << " not found in the metadata of method " << methodName << " for class " << className;
    }

    static bool HasTypeParam(const flatbuffers::Vector<::flatbuffers::Offset<TypeParamDecl>> &typeParams,
                             const std::string &expectedTypeParamName)
    {
        for (auto typeParam : typeParams) {
            if (typeParam->name()->str() == expectedTypeParamName) {
                return true;
            }
        }
        return false;
    }

    static void AssertTypeParamPresented(const pandasm::Program *program, const std::string &className,
                                         const std::string &methodName, const std::string &expectedTypeParamName)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);
        for (const auto classDecl : *root->classes()) {
            if (classDecl->name()->str() != className) {
                continue;
            }
            for (const auto method : *classDecl->methods()) {
                if (method->name()->str() == methodName &&
                    HasTypeParam(*method->type_params(), expectedTypeParamName)) {
                    return;
                }
            }
        }

        auto msg = "Type parameter " + expectedTypeParamName + " of method " + methodName +
                   " not found in the metadata for class " + className;
        FAIL() << msg << std::endl;
    }

    static void AssertAnnotationPresented(const pandasm::Program *program, const std::string &expectedAnnotationName)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);

        if (root->annotations() == nullptr) {
            FAIL() << "No annotations found in the metadata";
        }

        for (const auto annotationDecl : *root->annotations()) {
            if (annotationDecl->name()->str() == expectedAnnotationName) {
                return;
            }
        }

        FAIL() << "Annotation " + expectedAnnotationName + " not found in the metadata";
    }

    static void AssertAnnotationNotPresented(const pandasm::Program *program, const std::string &annotationName)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);

        if (root->annotations() == nullptr) {
            return;
        }

        for (const auto annotationDecl : *root->annotations()) {
            if (annotationDecl->name()->str() == annotationName) {
                FAIL() << "Annotation " + annotationName + " should not be found in the metadata";
            }
        }
    }

    static void AssertAnnotationsCount(const pandasm::Program *program, size_t expectedCount)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);

        if (root->annotations() == nullptr) {
            ASSERT_EQ(expectedCount, 0) << "Expected " << expectedCount << " annotations but found none";
            return;
        }

        ASSERT_EQ(root->annotations()->size(), expectedCount)
            << "Expected " << expectedCount << " annotations but found " << root->annotations()->size();
    }

    static void AssertMethodReturnTypeStringLiteral(const pandasm::Program *program, const std::string &className,
                                                    const std::string &methodName,
                                                    const std::string &expectedStringValue)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);
        for (const auto classDecl : *root->classes()) {
            if (classDecl->name()->str() != className) {
                continue;
            }
            for (const auto method : *classDecl->methods()) {
                if (method->name()->str() != methodName) {
                    continue;
                }
                const auto stringLiteralType = method->return_type_as_StringLiteral();
                ASSERT_NE(stringLiteralType, nullptr)
                    << "Method " << methodName << " should have string literal return type";
                ASSERT_EQ(stringLiteralType->value()->str(), expectedStringValue)
                    << "Method " << methodName << " should return string literal with value " << expectedStringValue;
                return;
            }
        }

        FAIL() << "Method " << methodName << " not found in metadata of class " << className;
    }

private:
    NO_COPY_SEMANTIC(MetadataTestSerialization);
    NO_MOVE_SEMANTIC(MetadataTestSerialization);
};

TEST_F(MetadataTestSerialization, metadata_basic_test)
{
    std::string_view text = R"(
    export class MyClass {
        x: number = 11

        constructor() {}
        constructor(x: number) {}
        constructor(p1: number, p2: string, p3: string) {}
    })";

    parser::ImportCache<parser::CacheType::SOURCES>::ActivateCache();
    EnableMetadataEmitting();
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    AssertClassPresented(program.get(), "MyClass");
}

TEST_F(MetadataTestSerialization, metadata_method_name)
{
    std::string_view text = R"(
    export class MyClass {
        x: number = 11
        hehehe(): void {
            console.log("hi")
        }
    })";

    parser::ImportCache<parser::CacheType::SOURCES>::ActivateCache();
    EnableMetadataEmitting();
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    AssertMethodPresented(program.get(), "MyClass", "hehehe");
}

TEST_F(MetadataTestSerialization, metadata_several_methods_name)
{
    std::string_view text = R"(
    export class MyClass {
        x: number = 11
        hehehe(): void {
            console.log("hehehe")
        }
        hohoho(): void {
            console.log("hohoho")
        }
        hahaha(): void {
            console.log("hahaha")
        }

        constructor() {}
        constructor(x: number) {}
        constructor(p1: number, p2: string, p3: string) {}
    })";

    parser::ImportCache<parser::CacheType::SOURCES>::ActivateCache();
    EnableMetadataEmitting();
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    AssertMethodPresented(program.get(), "MyClass", "hehehe");
    AssertMethodPresented(program.get(), "MyClass", "hohoho");
    AssertMethodPresented(program.get(), "MyClass", "hahaha");
}

TEST_F(MetadataTestSerialization, metadata_method_return_type_builtin)
{
    std::string_view text = R"(
    export class MyClass {
        x: number = 11
        hehehe(): void {
            console.log("aaa");
        }
        hohoho(): null {
            return null;
        }
        hahaha(): undefined {
            return undefined
        }
        foo(): Any {
            return undefined
        }
        fee(error: string): never {
            throw new Error(error);
        }
        faa(): string {
            return "faa"
        }
        lol(): bigint {
            return 1n
        }
        lel(): Object {
            let x: Object = new Object();
            if (true) {
                x = 10
            }
            return x
        }
    })";

    parser::ImportCache<parser::CacheType::SOURCES>::ActivateCache();
    EnableMetadataEmitting();
    const auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    AssertReturnTypeForMethod(program.get(), "MyClass", "hehehe", Metadata::BuiltinTypeKind_void_);
    AssertReturnTypeForMethod(program.get(), "MyClass", "hohoho", Metadata::BuiltinTypeKind_null);
    AssertReturnTypeForMethod(program.get(), "MyClass", "hahaha", Metadata::BuiltinTypeKind_undefined);
    AssertReturnTypeForMethod(program.get(), "MyClass", "foo", Metadata::BuiltinTypeKind_any);
    AssertReturnTypeForMethod(program.get(), "MyClass", "fee", Metadata::BuiltinTypeKind_never);
    AssertReturnTypeForMethod(program.get(), "MyClass", "faa", Metadata::BuiltinTypeKind_string_);
    AssertReturnTypeForMethod(program.get(), "MyClass", "lol", Metadata::BuiltinTypeKind_bigint);
    AssertReturnTypeForMethod(program.get(), "MyClass", "lel", Metadata::BuiltinTypeKind_object);
}

TEST_F(MetadataTestSerialization, metadata_method_return_type_builtin_primitive)
{
    std::string_view text = R"(
    export class MyClass {
        x: number = 11
        hehehe(): int {
            return 1;
        }
        hohoho(): long {
            return 1 as long;
        }
        hahaha(): double {
            return 1.0
        }
        foo(): boolean {
            return false;
        }
        fee(): byte {
            return 1 as byte;
        }
        faa(): short {
            return 1 as short
        }
        lol(): char {
            return c'a'
        }
        lel(): float {
            return 1.0f
        }
    })";

    parser::ImportCache<parser::CacheType::SOURCES>::ActivateCache();
    EnableMetadataEmitting();
    const auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    AssertReturnTypeForMethod(program.get(), "MyClass", "hehehe", Metadata::BuiltinTypeKind_int_);
    AssertReturnTypeForMethod(program.get(), "MyClass", "hohoho", Metadata::BuiltinTypeKind_long_);
    AssertReturnTypeForMethod(program.get(), "MyClass", "hahaha", Metadata::BuiltinTypeKind_double_);
    AssertReturnTypeForMethod(program.get(), "MyClass", "foo", Metadata::BuiltinTypeKind_boolean);
    AssertReturnTypeForMethod(program.get(), "MyClass", "fee", Metadata::BuiltinTypeKind_byte_);
    AssertReturnTypeForMethod(program.get(), "MyClass", "faa", Metadata::BuiltinTypeKind_short_);
    AssertReturnTypeForMethod(program.get(), "MyClass", "lol", Metadata::BuiltinTypeKind_char_);
    AssertReturnTypeForMethod(program.get(), "MyClass", "lel", Metadata::BuiltinTypeKind_float_);
}

TEST_F(MetadataTestSerialization, metadata_method_type_param_decls)
{
    std::string_view text = R"(
    export class MyClass {
        x: number = 11
        hehehe<T, K>(): void {
            console.log("aaa");
        }
    })";

    parser::ImportCache<parser::CacheType::SOURCES>::ActivateCache();
    EnableMetadataEmitting();
    const auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    AssertTypeParamPresented(program.get(), "MyClass", "hehehe", "T");
    AssertTypeParamPresented(program.get(), "MyClass", "hehehe", "K");
}

TEST_F(MetadataTestSerialization, metadata_annotation_basic)
{
    std::string_view text = R"(
    export @interface MyAnnotation {}
    export default @interface DefaultAnnotation {}
    @interface InternalAnnotation {}
    export @interface ThirdAnnotation {}
    export class MyClass {
        x: number = 11
        constructor() {}
    }
    class InternalClass {
        constructor() {}
    }
    )";

    parser::ImportCache<parser::CacheType::SOURCES>::ActivateCache();
    EnableMetadataEmitting();
    const auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    AssertClassPresented(program.get(), "MyClass");
    AssertAnnotationPresented(program.get(), "MyAnnotation");
    constexpr auto numberOfAnnotations = 3;
    AssertAnnotationsCount(program.get(), numberOfAnnotations);
}

TEST_F(MetadataTestSerialization, metadata_annotation_not_exported)
{
    std::string_view text = R"(
    @interface InternalAnnotation {}
    export class MyClass {
        x: number = 11
        constructor() {}
    }
    )";

    parser::ImportCache<parser::CacheType::SOURCES>::ActivateCache();
    EnableMetadataEmitting();
    const auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    AssertAnnotationNotPresented(program.get(), "InternalAnnotation");
    AssertClassPresented(program.get(), "MyClass");
    AssertAnnotationsCount(program.get(), 0);
}

TEST_F(MetadataTestSerialization, metadata_string_literal_type)
{
    std::string_view text = R"(
    export class MyClass {
        getString(): "hello world" {
            return "hello world"
        }
    }
    )";

    parser::ImportCache<parser::CacheType::SOURCES>::ActivateCache();
    EnableMetadataEmitting();
    const auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    AssertClassPresented(program.get(), "MyClass");
    AssertMethodPresented(program.get(), "MyClass", "getString");
    AssertMethodReturnTypeStringLiteral(program.get(), "MyClass", "getString", "hello world");
}

TEST_F(MetadataTestSerialization, metadata_multiple_string_literals)
{
    std::string_view text = R"(
    export class MyClass {
        getFirst(): "first" {
            return "first"
        }
        getSecond(): "second" {
            return "second"
        }
        getNumber(): number {
            return 42
        }
    }
    )";

    parser::ImportCache<parser::CacheType::SOURCES>::ActivateCache();
    EnableMetadataEmitting();
    const auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    AssertClassPresented(program.get(), "MyClass");
    AssertMethodPresented(program.get(), "MyClass", "getFirst");
    AssertMethodPresented(program.get(), "MyClass", "getSecond");
    AssertMethodPresented(program.get(), "MyClass", "getNumber");

    // Verify string literal return types
    AssertMethodReturnTypeStringLiteral(program.get(), "MyClass", "getFirst", "first");
    AssertMethodReturnTypeStringLiteral(program.get(), "MyClass", "getSecond", "second");
}

TEST_F(MetadataTestSerialization, metadata_enum_basic)
{
    std::string_view text = R"(
    export enum MyEnum {
        FIRST,
        SECOND,
        THIRD
    }
    )";

    parser::ImportCache<parser::CacheType::SOURCES>::ActivateCache();
    EnableMetadataEmitting();
    const auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    auto root = GetRoot(program->metadata.data());
    ASSERT_NE(root, nullptr);
    ASSERT_NE(root->enums(), nullptr);

    auto enumDecl = root->enums()->Get(0);
    ASSERT_NE(enumDecl, nullptr);
    ASSERT_EQ(enumDecl->name()->str(), "MyEnum");
    constexpr auto numberOfEnums = 3;
    ASSERT_EQ(enumDecl->entries()->size(), numberOfEnums);

    std::set<std::string> entries;
    for (const auto entry : *enumDecl->entries()) {
        entries.insert(entry->str());
    }

    ASSERT_EQ(entries.count("FIRST"), 1);
    ASSERT_EQ(entries.count("SECOND"), 1);
    ASSERT_EQ(entries.count("THIRD"), 1);
}

}  // namespace ark::es2panda::compiler::test