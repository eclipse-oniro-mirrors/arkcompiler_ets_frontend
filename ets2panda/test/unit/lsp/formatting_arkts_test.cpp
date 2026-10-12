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

#include "lsp/include/formatting/formatting.h"
#include "lsp/include/formatting/formatting_settings.h"
#include "lsp_api_test.h"
#include <gtest/gtest.h>
#include <algorithm>

namespace {

std::string ApplyChanges(const std::string &source, const std::vector<TextChange> &changes)
{
    std::string result = source;
    std::vector<TextChange> sortedChanges = changes;
    std::sort(sortedChanges.begin(), sortedChanges.end(),
              [](const TextChange &a, const TextChange &b) { return a.span.start > b.span.start; });
    for (const auto &change : sortedChanges) {
        result.replace(change.span.start, change.span.length, change.newText);
    }
    return result;
}

class LspFormattingArkTSTests : public LSPAPITests {};

// ArkTS-specific shapes (struct declarations, @decorators, rest parameters, keyof types)
// do not exist in any formatting_test*.cpp scenario; overlap with class/interface spacing
// rules is intentional and covers different AST nodes.
TEST_F(LspFormattingArkTSTests, FormatDocument_ETSStructDeclaration)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
struct Foo{x:number=0;}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
struct Foo { x: number = 0; }
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_ets_struct_decl.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LspFormattingArkTSTests, FormatDocument_ETSStructWithMethods)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
struct Foo {
method():void{}
method2():void{}
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
struct Foo {
method(): void {}
method2(): void {}
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_ets_struct_methods.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LspFormattingArkTSTests, FormatDocument_DecoratorSpacing)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
@Component
struct Foo{x:number=0;}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
@Component
struct Foo { x: number = 0; }
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_decorator_spacing.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LspFormattingArkTSTests, FormatDocument_DecoratorWithArgs)
{
    std::string testCode =
        "\n"
        // CC-OFFNXT(G.FMT.16-CPP) test logic
        R"(@Entry({count:2,label:"test"})
struct Foo{x:number=0;}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
@Entry({ count: 2, label: "test" })
struct Foo { x: number = 0; }
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_decorator_args.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LspFormattingArkTSTests, FormatDocument_ETSParameterExpression)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function foo(...args:number[]):void{}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
function foo(...args: number[]): void {}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_ets_param_expr.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LspFormattingArkTSTests, FormatDocument_ETSKeyofType)
{
    std::string testCode =
        "\n"
        // CC-OFFNXT(G.FMT.16-CPP) test logic
        R"(class A{}
type K=keyof A
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
class A { }
type K = keyof A
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_ets_keyof.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingEmptyBraces(true);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LspFormattingArkTSTests, FormatDocument_SendableClass)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
@Sendable
class Foo{x:number=0;}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
@Sendable
class Foo { x: number = 0; }
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_sendable_class.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LspFormattingArkTSTests, FormatDocument_AnnotationArray)
{
    std::string testCode =
        "\n"
        // CC-OFFNXT(G.FMT.16-CPP) test logic
        R"(@Annotation([1,2,3])
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    testCode += R"(class Foo{}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
@Annotation([1, 2, 3])
class Foo { }
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_annotation_array.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingEmptyBraces(true);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LspFormattingArkTSTests, FormatDocument_MultipleDecorators)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
@A @B
class Foo{x:number=0;}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
@A @B
class Foo { x: number = 0; }
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_multi_decorator.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

TEST_F(LspFormattingArkTSTests, FormatDocument_StructImplements)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
interface I{}
struct Foo implements I{x:number=0;}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
interface I { }
struct Foo implements I { x: number = 0; }
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_struct_implements.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingEmptyBraces(true);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);

    initializer.DestroyContext(ctx);
}

}  // namespace
