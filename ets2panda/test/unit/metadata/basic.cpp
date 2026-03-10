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
#include "assembly-ins.h"
#include "assembly-parser.h"
#include "assembly-program.h"
#include "abc2program/abc2program_driver.h"
#include "test/utils/checker_test.h"
#include "flatbuffers/flatbuffers.h"
#include "compiler/metadata/metadata_generated.h"
#include "util/generateBin.h"
#include "parser/program/DeclarationCache.h"

namespace ark::es2panda::compiler::test {

using Metadata::GetRoot, Metadata::BuiltinTypeKind, Metadata::TypeParamDecl;

class MetadataTest : public ::test::utils::CheckerTest {
public:
    MetadataTest() = default;

    ~MetadataTest() override = default;

    void AssertClassPresented(pandasm::Program *program, const std::string &expectedClassName)
    {
        auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);

        for (auto classDecl : *root->classes()) {
            if (classDecl->name()->str() == expectedClassName) {
                return;
            }
        }

        FAIL() << "Class " + expectedClassName + " not found in the metadata";
    }

    void AssertMethodPresented(pandasm::Program *program, const std::string &className,
                               const std::string &expectedMethodName)
    {
        auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);
        for (auto classDecl : *root->classes()) {
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

    void AssertReturnTypeForMethod(pandasm::Program *program, const std::string &className,
                                   const std::string &methodName, const BuiltinTypeKind expectedReturnType)
    {
        auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);
        for (auto classDecl : *root->classes()) {
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

    bool HasTypeParam(const ::flatbuffers::Vector<::flatbuffers::Offset<TypeParamDecl>> &typeParams,
                      const std::string &expectedTypeParamName)
    {
        for (auto typeParam : typeParams) {
            if (typeParam->name()->str() == expectedTypeParamName) {
                return true;
            }
        }
        return false;
    }

    void AssertTypeParamPresented(pandasm::Program *program, const std::string &className,
                                  const std::string &methodName, const std::string &expectedTypeParamName)
    {
        auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);
        for (auto classDecl : *root->classes()) {
            if (classDecl->name()->str() != className) {
                continue;
            }
            for (auto method : *classDecl->methods()) {
                if (method->name()->str() == methodName &&
                    HasTypeParam(*method->type_params(), expectedTypeParamName)) {
                    return;
                }
            }
        }

        // CC-OFFNXT(G.FMT.06-CPP) project code style
        FAIL() << "Type parameter " << expectedTypeParamName << " of method " << methodName
               << " not found in the metadata for class " << className << std::endl;
    }

    void AssertMetadataNotEmitted(pandasm::Program *program)
    {
        ASSERT_EQ(GetRoot(program->metadata.data()), nullptr);
    }

private:
    NO_COPY_SEMANTIC(MetadataTest);
    NO_MOVE_SEMANTIC(MetadataTest);
};

TEST_F(MetadataTest, metadata_basic_test)
{
    std::string_view text = R"(
    export class MyClass {
        x: number = 11

        constructor() {}
        constructor(x: number) {}
        constructor(p1: number, p2: string, p3: string) {}
    })";

    parser::DeclarationCache::ActivateCache();
    EnableMetadataEmitting();
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    AssertClassPresented(program.get(), "MyClass");
}

TEST_F(MetadataTest, metadata_abc_writing_reading_test)
{
    std::string_view text = R"(
    export class MyClass {
        x: number = 11

        constructor() {}
        constructor(x: number) {}
        constructor(p1: number, p2: string, p3: string) {}
    })";

    parser::DeclarationCache::ActivateCache();
    EnableMetadataEmitting();
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);
    auto diagnosticEngine = util::DiagnosticEngine();
    auto options = std::make_unique<util::Options>("", diagnosticEngine);
    auto const argumentsNumber = 2;
    std::array<const char *const, argumentsNumber> args = {{"dummy.ets", "--output=dummy.abc"}};
    options->Parse(Span(args.begin(), argumentsNumber));

    util::GenerateProgram(
        program.get(), *options,
        [&diagnosticEngine](const diagnostic::DiagnosticKind &kind, const util::DiagnosticMessageParams &params) {
            diagnosticEngine.LogDiagnostic(kind, params);
        });

    abc2program::Abc2ProgramDriver driver;
    (void)driver.Compile("dummy.abc");
    auto prog = &(driver.GetProgram());

    AssertClassPresented(prog, "MyClass");
}

TEST_F(MetadataTest, metadata_method_name)
{
    std::string_view text = R"(
    export class MyClass {
        x: number = 11
        hehehe(): void {
            console.log("hi")
        }
    })";

    parser::DeclarationCache::ActivateCache();
    EnableMetadataEmitting();
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    AssertMethodPresented(program.get(), "MyClass", "hehehe");
}

TEST_F(MetadataTest, metadata_several_methods_name)
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

    parser::DeclarationCache::ActivateCache();
    EnableMetadataEmitting();
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    AssertMethodPresented(program.get(), "MyClass", "hehehe");
    AssertMethodPresented(program.get(), "MyClass", "hohoho");
    AssertMethodPresented(program.get(), "MyClass", "hahaha");
}

TEST_F(MetadataTest, metadata_method_return_type_builtin)
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

    parser::DeclarationCache::ActivateCache();
    EnableMetadataEmitting();
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    AssertReturnTypeForMethod(program.get(), "MyClass", "hehehe", BuiltinTypeKind::BUILTIN_TYPE_KIND_VOID);
    AssertReturnTypeForMethod(program.get(), "MyClass", "hohoho", BuiltinTypeKind::BUILTIN_TYPE_KIND_NULL);
    AssertReturnTypeForMethod(program.get(), "MyClass", "hahaha", BuiltinTypeKind::BUILTIN_TYPE_KIND_UNDEFINED);
    AssertReturnTypeForMethod(program.get(), "MyClass", "foo", BuiltinTypeKind::BUILTIN_TYPE_KIND_ANY);
    AssertReturnTypeForMethod(program.get(), "MyClass", "fee", BuiltinTypeKind::BUILTIN_TYPE_KIND_NEVER);
    AssertReturnTypeForMethod(program.get(), "MyClass", "faa", BuiltinTypeKind::BUILTIN_TYPE_KIND_STRING);
    AssertReturnTypeForMethod(program.get(), "MyClass", "lol", BuiltinTypeKind::BUILTIN_TYPE_KIND_BIGINT);
    AssertReturnTypeForMethod(program.get(), "MyClass", "lel", BuiltinTypeKind::BUILTIN_TYPE_KIND_OBJECT);
}

TEST_F(MetadataTest, metadata_method_return_type_builtin_primitive)
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

    parser::DeclarationCache::ActivateCache();
    EnableMetadataEmitting();
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    AssertReturnTypeForMethod(program.get(), "MyClass", "hehehe", BuiltinTypeKind::BUILTIN_TYPE_KIND_INT);
    AssertReturnTypeForMethod(program.get(), "MyClass", "hohoho", BuiltinTypeKind::BUILTIN_TYPE_KIND_LONG);
    AssertReturnTypeForMethod(program.get(), "MyClass", "hahaha", BuiltinTypeKind::BUILTIN_TYPE_KIND_DOUBLE);
    AssertReturnTypeForMethod(program.get(), "MyClass", "foo", BuiltinTypeKind::BUILTIN_TYPE_KIND_BOOLEAN);
    AssertReturnTypeForMethod(program.get(), "MyClass", "fee", BuiltinTypeKind::BUILTIN_TYPE_KIND_BYTE);
    AssertReturnTypeForMethod(program.get(), "MyClass", "faa", BuiltinTypeKind::BUILTIN_TYPE_KIND_SHORT);
    AssertReturnTypeForMethod(program.get(), "MyClass", "lol", BuiltinTypeKind::BUILTIN_TYPE_KIND_CHAR);
    AssertReturnTypeForMethod(program.get(), "MyClass", "lel", BuiltinTypeKind::BUILTIN_TYPE_KIND_FLOAT);
}

TEST_F(MetadataTest, metadata_method_type_param_decls)
{
    std::string_view text = R"(
    export class MyClass {
        x: number = 11
        hehehe<T, K>(): void {
            console.log("aaa");
        }
    })";

    parser::DeclarationCache::ActivateCache();
    EnableMetadataEmitting();
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);

    AssertTypeParamPresented(program.get(), "MyClass", "hehehe", "T");
    AssertTypeParamPresented(program.get(), "MyClass", "hehehe", "K");
}

TEST_F(MetadataTest, metadata_disabled)
{
    std::string_view text = R"(
    export class MyClass {
        x: number = 11

        constructor() {}
        constructor(x: number) {}
        constructor(p1: number, p2: string, p3: string) {}
    })";

    parser::DeclarationCache::ActivateCache();
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, []([[maybe_unused]] ir::AstNode *ast) {});
    ASSERT_NE(program, nullptr);
    auto diagnosticEngine = util::DiagnosticEngine();
    auto options = std::make_unique<util::Options>("", diagnosticEngine);
    auto argsNumber = 2;
    std::array<const char *const, 2> args = {{"dummy.ets", "--output=dummy.abc"}};
    options->Parse(Span(args.begin(), argsNumber));

    util::GenerateProgram(
        program.get(), *options,
        [&diagnosticEngine](const diagnostic::DiagnosticKind &kind, const util::DiagnosticMessageParams &params) {
            diagnosticEngine.LogDiagnostic(kind, params);
        });

    abc2program::Abc2ProgramDriver driver;
    (void)driver.Compile("dummy.abc");
    auto prog = &(driver.GetProgram());

    AssertMetadataNotEmitted(prog);
}

}  // namespace ark::es2panda::compiler::test