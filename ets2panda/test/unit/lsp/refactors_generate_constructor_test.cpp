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

#include "lsp/include/refactors/generate_constructor.h"
#include <gtest/gtest.h>
#include <string>
#include <optional>
#include <string_view>
#include "lsp_api_test.h"

#include "lsp/include/services/text_change/change_tracker.h"
#include "public/es2panda_lib.h"

namespace {

class LSPGenerateConstructorTests : public LSPAPITests {
protected:
    std::optional<std::string> RunGenerateConstructorRefactorAndGetText(std::string_view fileName,
                                                                        std::string_view sourceCode,
                                                                        size_t cursorPosition)
    {
        auto tempFiles = this->CreateTempFile({std::string(fileName)}, {std::string(sourceCode)});
        if (tempFiles.empty()) {
            return std::nullopt;
        }

        ark::es2panda::lsp::Initializer initializer;
        es2panda_Context *context = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

        auto *tokenNode = ark::es2panda::lsp::GetTouchingToken(context, cursorPosition, false);
        if (tokenNode == nullptr) {
            initializer.DestroyContext(context);
            return std::nullopt;
        }

        ark::es2panda::lsp::FormatCodeSettings formatSettings;
        auto formatContext = ark::es2panda::lsp::GetFormatContext(formatSettings);
        TextChangesContext textChangeContext {{}, formatContext, {}};

        ark::es2panda::lsp::RefactorContext refactorContext;
        refactorContext.context = context;
        refactorContext.textChangesContext = &textChangeContext;
        refactorContext.kind = std::string(ark::es2panda::lsp::TO_GENERATE_CONSTRUCTOR_ACTION.kind);
        refactorContext.span.pos = cursorPosition;

        ark::es2panda::lsp::ChangeTracker changeTracker =
            ark::es2panda::lsp::ChangeTracker::FromContext(textChangeContext);

        (void)ark::es2panda::lsp::GetApplicableRefactorsImpl(&refactorContext);

        auto actions = ark::es2panda::lsp::GenerateConstructorRefactor().GetEditsForAction(
            refactorContext, std::string(ark::es2panda::lsp::TO_GENERATE_CONSTRUCTOR_ACTION.name));

        std::optional<std::string> result;
        if (actions && !actions->GetFileTextChanges().empty()) {
            const auto &fileChange = actions->GetFileTextChanges().at(0);
            if (!fileChange.textChanges.empty()) {
                result = std::string(fileChange.textChanges.at(0).newText);
            }
        }

        initializer.DestroyContext(context);
        return result;
    }

    void VerifyGeneratedConstructorEqualsExpected(std::string_view fileName, std::string_view sourceCode,
                                                  size_t cursorPosition, std::string_view expectedConstructor)
    {
        auto result = RunGenerateConstructorRefactorAndGetText(fileName, sourceCode, cursorPosition);
        ASSERT_TRUE(result.has_value()) << "Generate Constructor produced no output.";
        EXPECT_EQ(*result, expectedConstructor);
    }

    void VerifyNoConstructorGenerated(std::string_view fileName, std::string_view sourceCode, size_t cursorPosition)
    {
        auto result = RunGenerateConstructorRefactorAndGetText(fileName, sourceCode, cursorPosition);
        EXPECT_FALSE(result.has_value()) << "Unexpected constructor generated:\n" << *result;
    }
};

TEST_F(LSPGenerateConstructorTests, GenerateConstructor_ForSimpleClass)
{
    const std::string sourceCode = R"(
class Point {
    x: number
    y: number
}
)";
    constexpr std::string_view EXPECTED =
        "\n    constructor(x: number, y: number) {\n"
        "        this.x = x\n"
        "        this.y = y\n"
        "    }\n";

    const size_t cursorPosition = 42;
    this->VerifyGeneratedConstructorEqualsExpected("GenerateConstructor_ForSimpleClass.ets", sourceCode, cursorPosition,
                                                   EXPECTED);
}

TEST_F(LSPGenerateConstructorTests, GenerateConstructor_ForDerivedWithBaseConstructor)
{
    const std::string sourceCode = R"(
class Point {
    x: number
    y: number
    constructor(x: number, y: number) {
        this.x = x
        this.y = y
    }
}

class ColoredPoint extends Point {
    static readonly WHITE = 0
    static readonly BLACK = 1
    color: number
}
)";
    constexpr std::string_view EXPECTED =
        "\n    constructor(x: number, y: number, color: number) {\n"
        "        super(x, y)\n"
        "        this.color = color\n"
        "    }\n";

    const size_t cursorPosition = 242;
    this->VerifyGeneratedConstructorEqualsExpected("GenerateConstructor_ForDerivedWithBaseConstructor.ets", sourceCode,
                                                   cursorPosition, EXPECTED);
}

TEST_F(LSPGenerateConstructorTests, GenerateConstructor_ForDerivedWithoutBaseConstructor)
{
    const std::string sourceCode = R"(
class Point {
    x: number
    y: number
}

class ColoredPoint extends Point {
    static readonly WHITE = 0
    static readonly BLACK = 1
    color: number
}
)";
    constexpr std::string_view EXPECTED =
        "\n    constructor(color: number) {\n"
        "        super()\n"
        "        this.color = color\n"
        "    }\n";

    const size_t cursorPosition = 158;
    this->VerifyGeneratedConstructorEqualsExpected("GenerateConstructor_ForDerivedWithoutBaseConstructor.ets",
                                                   sourceCode, cursorPosition, EXPECTED);
}

TEST_F(LSPGenerateConstructorTests, GenerateConstructor_TrimsUnderscorePrefixFromFieldAndParameterNames)
{
    const std::string sourceCode = R"(
class Point {
    constructor(_id: number) {}
}
class ColoredPoint extends Point {
    _name: number;
}
)";
    constexpr std::string_view EXPECTED =
        "\n    constructor(id: number, name: number) {\n"
        "        super(id)\n"
        "        this._name = name\n"
        "    }\n";

    const size_t cursorPosition = 102;
    this->VerifyGeneratedConstructorEqualsExpected(
        "GenerateConstructor_TrimsUnderscorePrefixFromFieldAndParameterNames.ets", sourceCode, cursorPosition,
        EXPECTED);
}

TEST_F(LSPGenerateConstructorTests, GenerateConstructor_ForDerivedClassWithOnlyStaticMembers)
{
    const std::string sourceCode = R"(
class Point {
    constructor(x: number, y: number) {}
}
class ColoredPoint extends Point {
    static WHITE = 0
    static BLACK = 1
}
)";
    constexpr std::string_view EXPECTED =
        "\n    constructor(x: number, y: number) {\n"
        "        super(x, y)\n"
        "    }\n";

    const size_t cursorPosition = 134;
    this->VerifyGeneratedConstructorEqualsExpected("GenerateConstructor_ForDerivedClassWithOnlyStaticMembers.ets",
                                                   sourceCode, cursorPosition, EXPECTED);
}

TEST_F(LSPGenerateConstructorTests, GenerateConstructor_IgnoresAccessors_UsesOnlyInstanceFields)
{
    const std::string sourceCode = R"(
class Point { constructor(n: number) {} }
class ColoredPoint extends Point {
    get value() { return 1 }
    set value(v: number) {}
    p: number
}
)";

    constexpr std::string_view EXPECTED =
        "\n    constructor(n: number, p: number) {\n"
        "        super(n)\n"
        "        this.p = p\n"
        "    }\n";

    const size_t cursorPosition = 148;
    this->VerifyGeneratedConstructorEqualsExpected("GenerateConstructor_IgnoresAccessors_UsesOnlyInstanceFields.ets",
                                                   sourceCode, cursorPosition, EXPECTED);
}

TEST_F(LSPGenerateConstructorTests, GenerateConstructor_ForBaseWithoutConstructor_AndMultipleInstanceFields)
{
    const std::string sourceCode = R"(
class A {}
class B extends A {
    u: number
    v: number
}
)";

    constexpr std::string_view EXPECTED =
        "\n    constructor(u: number, v: number) {\n"
        "        super()\n"
        "        this.u = u\n"
        "        this.v = v\n"
        "    }\n";

    const size_t cursorPosition = 59;
    this->VerifyGeneratedConstructorEqualsExpected(
        "GenerateConstructor_ForBaseWithoutConstructor_AndMultipleInstanceFields.ets", sourceCode, cursorPosition,
        EXPECTED);
}

TEST_F(LSPGenerateConstructorTests, GenerateConstructor_IncludesPrivateAndProtectedInstanceFields)
{
    const std::string sourceCode = R"(
class P {
    private a: number
    protected b: number
}
)";

    constexpr std::string_view EXPECTED =
        "\n    constructor(a: number, b: number) {\n"
        "        this.a = a\n"
        "        this.b = b\n"
        "    }\n";

    const size_t cursorPosition = 56;
    this->VerifyGeneratedConstructorEqualsExpected("GenerateConstructor_IncludesPrivateAndProtectedInstanceFields.ets",
                                                   sourceCode, cursorPosition, EXPECTED);
}

}  // namespace