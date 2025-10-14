/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 */

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "ir/astNode.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/refactors/generate_override_methods.h"
#include "lsp/include/refactors/refactor_types.h"
#include "public/public.h"

using ark::es2panda::lsp::ApplicableRefactorInfo;
namespace {
class LSPGenerateOverrideMethodsTests : public LSPAPITests {};

TEST_F(LSPGenerateOverrideMethodsTests, Override_Methods_Interface_to_Interface_cmplx)
{
    const std::string src = R"(
interface Base {
kinds_of_parameters<T extends Derived, U extends Base>(
p01: Derived,
p02: (q: Base)=>Derived,
p03: number,
p04: Number,
p06: E1,
p07: Base[],
p08: [Base, Base]
): void
kinds_of_return_type(): Object
}
interface Derived1 extends Base {
}
)";
    auto files = CreateTempFile({"Override_Methods_I2I_cmplx.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    const size_t refactorPosition = 253;
    auto node = ark::es2panda::lsp::GetTouchingToken(ctx, refactorPosition, false);
    ASSERT_NE(node, nullptr);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = std::string(ark::es2panda::lsp::TO_GENERATE_OVERRIDE_METHODS_ACTION.kind);
    refactorContext.span.pos = refactorPosition;
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::GenerateOverrideMethods().GetEditsForAction(
        refactorContext, std::string(ark::es2panda::lsp::TO_GENERATE_OVERRIDE_METHODS_ACTION.name));
    std::string_view res = actions->GetFileTextChanges().at(0).textChanges.at(0).newText;
    std::string_view expect =
        "\n    kinds_of_parameters(p01: Derived, p02: ((q:Base) => Derived), p03: number, p04: Number, p06: E1, p07: "
        "Base[], p08: [Base, Base]) : void {\n\n}\n\n    kinds_of_return_type() : Object {\n\n}\n\n";
    EXPECT_EQ(res, expect);

    init.DestroyContext(ctx);
}

TEST_F(LSPGenerateOverrideMethodsTests, Override_Methods_implements)
{
    const std::string src = R"(
interface Interface {
foo (p: number): void
foo (p: string): void
}
class Class1 implements Interface {
}
)";
    auto files = CreateTempFile({"override_Methods_implements.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    const size_t refactorPosition = 104;
    auto node = ark::es2panda::lsp::GetTouchingToken(ctx, refactorPosition, false);
    ASSERT_NE(node, nullptr);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = std::string(ark::es2panda::lsp::TO_GENERATE_OVERRIDE_METHODS_ACTION.kind);
    refactorContext.span.pos = refactorPosition;
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::GenerateOverrideMethods().GetEditsForAction(
        refactorContext, std::string(ark::es2panda::lsp::TO_GENERATE_OVERRIDE_METHODS_ACTION.name));
    std::string_view res = actions->GetFileTextChanges().at(0).textChanges.at(0).newText;
    std::string_view expect = "\n    foo(p: number) : void {\n\n}\n\n    foo(p: string) : void {\n\n}\n\n";
    EXPECT_EQ(res, expect);

    init.DestroyContext(ctx);
}

TEST_F(LSPGenerateOverrideMethodsTests, Override_Methods_all_types)
{
    const std::string src = R"(
class Base {
public public_member() {}
protected protected_member() {}
private private_member() {}
}
interface Interface {
public_member()
// All members are public in interfaces
private private_member() {} // Except private methods with default implementation
}
class Derived extends Base implements Interface {
}

)";
    auto files = CreateTempFile({"override_Methods_all_types.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    const size_t refactorPosition = 313;
    auto node = ark::es2panda::lsp::GetTouchingToken(ctx, refactorPosition, false);
    ASSERT_NE(node, nullptr);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = std::string(ark::es2panda::lsp::TO_GENERATE_OVERRIDE_METHODS_ACTION.kind);
    refactorContext.span.pos = refactorPosition;
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::GenerateOverrideMethods().GetEditsForAction(
        refactorContext, std::string(ark::es2panda::lsp::TO_GENERATE_OVERRIDE_METHODS_ACTION.name));
    std::string_view res = actions->GetFileTextChanges().at(0).textChanges.at(0).newText;
    std::string_view expect =
        "\n    public override public_member() {\n        super.public_member();\n    }\n\n    public override "
        "protected_member() {\n        super.protected_member();\n    }\n\n    public_member() {\n\n}\n\n";
    EXPECT_EQ(res, expect);

    init.DestroyContext(ctx);
}

TEST_F(LSPGenerateOverrideMethodsTests, Override_Methods_getter_setter)
{
    const std::string src = R"(
    class Base {
    get value(): number { return 42; }
    set value(v: number) { console.log(v); }
}

class Child extends Base {
}
)";
    auto files = CreateTempFile({"override_Methods_getter_setter.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    const size_t refactorPosition = 131;
    auto node = ark::es2panda::lsp::GetTouchingToken(ctx, refactorPosition, false);
    ASSERT_NE(node, nullptr);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = std::string(ark::es2panda::lsp::TO_GENERATE_OVERRIDE_METHODS_ACTION.kind);
    refactorContext.span.pos = refactorPosition;
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::GenerateOverrideMethods().GetEditsForAction(
        refactorContext, std::string(ark::es2panda::lsp::TO_GENERATE_OVERRIDE_METHODS_ACTION.name));
    std::string_view res = actions->GetFileTextChanges().at(0).textChanges.at(0).newText;
    std::string_view expect =
        "\n    public override get value() : number {\n        return super.value();\n    }\n\n    public override set "
        "value(v: number) {\n        super.value(v: number);\n    }\n\n";
    EXPECT_EQ(res, expect);

    init.DestroyContext(ctx);
}

TEST_F(LSPGenerateOverrideMethodsTests, Override_Methods_Basic1)
{
    const std::string src = R"(
class Base {
greet(name: string): string {
return `Hello ${name}`;
}

protected calc(x: number): number {
return x * 2;
}
}

class Child extends Base {
}
)";
    auto files = CreateTempFile({"generate_override_methods_1.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    const size_t refactorPosition = 152;
    auto node = ark::es2panda::lsp::GetTouchingToken(ctx, refactorPosition, false);
    ASSERT_NE(node, nullptr);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = std::string(ark::es2panda::lsp::TO_GENERATE_OVERRIDE_METHODS_ACTION.kind);
    refactorContext.span.pos = refactorPosition;
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::GenerateOverrideMethods().GetEditsForAction(
        refactorContext, std::string(ark::es2panda::lsp::TO_GENERATE_OVERRIDE_METHODS_ACTION.name));
    std::string_view res = actions->GetFileTextChanges().at(0).textChanges.at(0).newText;
    std::string_view expect =
        "\n    public override greet(name: string) : string {\n        return super.greet(name: string);\n    }\n\n    "
        "public override calc(x: number) : number {\n        return super.calc(x: number);\n    }\n\n";
    EXPECT_EQ(res, expect);

    init.DestroyContext(ctx);
}
TEST_F(LSPGenerateOverrideMethodsTests, Override_Methods_Interface_to_Interface)
{
    const std::string src = R"(
interface Base {
param(p: Derived): void
ret(): Base
}
interface Derived extends Base {
}
)";
    auto files = CreateTempFile({"override_Methods_I2I.ets"}, {src});
    ASSERT_FALSE(files.empty());
    const size_t refactorPosition = 87;

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    const auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto node = ark::es2panda::lsp::GetTouchingToken(ctx, refactorPosition, false);
    ASSERT_NE(node, nullptr);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = std::string(ark::es2panda::lsp::TO_GENERATE_OVERRIDE_METHODS_ACTION.kind);
    refactorContext.span.pos = refactorPosition;
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::GenerateOverrideMethods().GetEditsForAction(
        refactorContext, std::string(ark::es2panda::lsp::TO_GENERATE_OVERRIDE_METHODS_ACTION.name));
    std::string_view res = actions->GetFileTextChanges().at(0).textChanges.at(0).newText;
    std::string_view expect = "\n    param(p: Derived) : void {\n\n}\n\n    ret() : Base {\n\n}\n\n";
    EXPECT_EQ(res, expect);

    init.DestroyContext(ctx);
}

TEST_F(LSPGenerateOverrideMethodsTests, Override_Methods_Interface_to_Interface_2)
{
    const std::string src = R"(
interface I1 {
  foo(): void
}

interface I2 extends I1 {
}
)";
    auto files = CreateTempFile({"override_Methods_I2I_2.ets"}, {src});
    ASSERT_FALSE(files.empty());
    const size_t refactorPosition = 58;

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    const auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto node = ark::es2panda::lsp::GetTouchingToken(ctx, refactorPosition, false);
    ASSERT_NE(node, nullptr);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &changeText;
    refactorContext.kind = std::string(ark::es2panda::lsp::TO_GENERATE_OVERRIDE_METHODS_ACTION.kind);
    refactorContext.span.pos = refactorPosition;
    ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    auto actions = ark::es2panda::lsp::GenerateOverrideMethods().GetEditsForAction(
        refactorContext, std::string(ark::es2panda::lsp::TO_GENERATE_OVERRIDE_METHODS_ACTION.name));
    std::string_view res = actions->GetFileTextChanges().at(0).textChanges.at(0).newText;
    std::string_view expect = "\n    foo() : void {\n\n}\n\n";
    EXPECT_EQ(res, expect);

    init.DestroyContext(ctx);
}

}  // namespace