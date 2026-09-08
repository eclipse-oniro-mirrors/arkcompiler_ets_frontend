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
#include <cstddef>
#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/inlay_hints.h"
#include "lsp/include/cancellation_token.h"
#include "public/es2panda_lib.h"

namespace {
using ark::es2panda::lsp::CancellationToken;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::UserPreferences;

constexpr size_t DEFAULT_THROTTLE = 20;

struct ExpectedHint {
    std::string text;
    size_t number;
    InlayHintKind kind;
    bool whitespaceBefore;
    bool whitespaceAfter;
};

class LSPInlayHints1Tests : public LSPAPITests {
public:
    static InlayHintList ProvideHints(es2panda_Context *context, const TextSpan &span, UserPreferences &preferences)
    {
        CancellationToken token(DEFAULT_THROTTLE, nullptr);
        return ark::es2panda::lsp::ProvideInlayHintsImpl(context, &span, token, preferences);
    }

    static InlayHintList ProvideHintsWithAllPreferences(es2panda_Context *context, const TextSpan &span)
    {
        UserPreferences preferences = UserPreferences::GetDefaultUserPreferences();
        preferences.SetIncludeInlayParameterNameHints(UserPreferences::IncludeInlayParameterNameHints::ALL);
        preferences.SetIncludeInlayVariableTypeHints(true);
        preferences.SetIncludeInlayFunctionParameterTypeHints(true);
        preferences.SetIncludeInlayFunctionLikeReturnTypeHints(true);
        return ProvideHints(context, span, preferences);
    }

    static void ExpectHint(const InlayHint &hint, const ExpectedHint &expected)
    {
        EXPECT_EQ(hint.text, expected.text);
        EXPECT_EQ(hint.number, static_cast<int>(expected.number));
        EXPECT_EQ(hint.kind, expected.kind);
        EXPECT_EQ(hint.whitespaceBefore, expected.whitespaceBefore);
        EXPECT_EQ(hint.whitespaceAfter, expected.whitespaceAfter);
    }

    static void ExpectHints(const InlayHintList &result, const std::vector<ExpectedHint> &expected)
    {
        ASSERT_EQ(result.hints.size(), expected.size());
        for (size_t index = 0; index < expected.size(); ++index) {
            ExpectHint(result.hints[index], expected[index]);
        }
    }

    class CancelledHost final : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            wasQueried_ = true;
            return true;
        }

        bool WasQueried() const
        {
            return wasQueried_;
        }

    private:
        bool wasQueried_ = false;
    };
};

TEST_F(LSPInlayHints1Tests, ParameterNameHintsViaPublicApi)
{
    std::vector<std::string> files = {"param_hints_public.ets"};
    std::vector<std::string> texts = {R"(function foo(param1: number, param2: number) {
}
foo(10, 20);
)"};
    const auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    const std::string &source = texts[0];
    const auto arg1 = source.find("10");
    const auto arg2 = source.find("20");
    ASSERT_NE(arg1, std::string::npos);
    ASSERT_NE(arg2, std::string::npos);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    TextSpan span {0, source.size()};
    auto result = lspApi->provideInlayHints(ctx, &span);
    initializer.DestroyContext(ctx);

    ExpectHints(result, {{"param1", arg1, InlayHintKind::PARAMETER, false, true},
                         {"param2", arg2, InlayHintKind::PARAMETER, false, true}});
}

TEST_F(LSPInlayHints1Tests, ParameterNameHintsDisabledByDefaultPreferences)
{
    std::vector<std::string> files = {"param_hints_none.ets"};
    std::vector<std::string> texts = {R"(function foo(param1: number, param2: number) {
}
foo(10, 20);
)"};
    const auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    TextSpan span {0, texts[0].size()};

    UserPreferences nonePreferences = UserPreferences::GetDefaultUserPreferences();
    auto resultNone = ProvideHints(ctx, span, nonePreferences);

    UserPreferences literalsPreferences = UserPreferences::GetDefaultUserPreferences();
    literalsPreferences.SetIncludeInlayParameterNameHints(UserPreferences::IncludeInlayParameterNameHints::LITERALS);
    auto resultLiterals = ProvideHints(ctx, span, literalsPreferences);

    UserPreferences allPreferences = UserPreferences::GetDefaultUserPreferences();
    allPreferences.SetIncludeInlayParameterNameHints(UserPreferences::IncludeInlayParameterNameHints::ALL);
    auto resultAll = ProvideHints(ctx, span, allPreferences);
    initializer.DestroyContext(ctx);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t paramHintCount = 2;
    ASSERT_TRUE(resultNone.hints.empty());
    ASSERT_EQ(resultLiterals.hints.size(), paramHintCount);
    ASSERT_EQ(resultAll.hints.size(), paramHintCount);
}

TEST_F(LSPInlayHints1Tests, ProvideInlayHintsImplStopsAtCancelledCancellableNode)
{
    const std::string source = R"(function foo(value: number): void {
    value;
}
)";
    Initializer initializer;
    auto *context = initializer.CreateContext("inlay_hints_cancelled.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);
    TextSpan span {0, source.size()};
    UserPreferences preferences = UserPreferences::GetDefaultUserPreferences();
    CancelledHost host;
    CancellationToken token(0, &host);

    // Implementation API: LSPAPI::provideInlayHints creates a non-cancellable token in api.cpp.
    const auto result = ark::es2panda::lsp::ProvideInlayHintsImpl(context, &span, token, preferences);
    initializer.DestroyContext(context);

    EXPECT_TRUE(host.WasQueried());
    EXPECT_TRUE(result.hints.empty());
}

TEST_F(LSPInlayHints1Tests, TypeHintsForVariableParameterAndReturn)
{
    std::vector<std::string> files = {"type_hints.ets"};
    std::vector<std::string> texts = {R"(function add(a: number, b: number): number {
    return a + b;
}
)"};
    const auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    const std::string &source = texts[0];
    const auto paramAEnd = source.find(',');
    const auto paramBEnd = source.find(')');
    ASSERT_NE(paramAEnd, std::string::npos);
    ASSERT_NE(paramBEnd, std::string::npos);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    const auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ASSERT_NE(astContext, nullptr);
    auto *parent = reinterpret_cast<ark::es2panda::ir::AstNode *>(astContext->parserProgram->Ast());
    ASSERT_NE(parent, nullptr);

    // The Visitor only reaches GetFunctionParameterTypeForHints for function declarations,
    // but ShouldProcessNode() skips typed function-like nodes first, so ProvideInlayHintsImpl
    // cannot produce parameter type hints either; invoke the helper directly.
    InlayHintList result;
    auto *funcDecl = parent->FindChild([](ark::es2panda::ir::AstNode *node) {
        return node->IsMethodDefinition() && ark::es2panda::lsp::GetFullWidth(node) != 0;
    });
    ASSERT_NE(funcDecl, nullptr);
    ark::es2panda::lsp::GetFunctionParameterTypeForHints(funcDecl, &result);
    initializer.DestroyContext(ctx);

    ExpectHints(result, {{"number", paramAEnd, InlayHintKind::TYPE, true, false},
                         {"number", paramBEnd, InlayHintKind::TYPE, true, false}});
}

TEST_F(LSPInlayHints1Tests, ReturnTypeHintsViaDirectHelpers)
{
    std::vector<std::string> files = {"return_type_hints.ets"};
    std::vector<std::string> texts = {R"(function add(a: number, b: number): number {
    return a + b;
}
const mul = (a: number, b: number): number => {
    return a * b;
};
)"};
    const auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    const std::string &source = texts[0];
    const auto funcEnd = source.find("\n}\n") + 2;
    const auto arrowEnd = source.find("\n};") + 2;
    ASSERT_NE(funcEnd, std::string::npos);
    ASSERT_NE(arrowEnd, std::string::npos);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    const auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ASSERT_NE(astContext, nullptr);
    auto *parent = reinterpret_cast<ark::es2panda::ir::AstNode *>(astContext->parserProgram->Ast());
    ASSERT_NE(parent, nullptr);

    // The Visitor never reaches GetFunctionReturnTypeForHints because ShouldProcessNode() skips
    // typed function-like nodes; invoke the helper directly to verify the return type hints.
    InlayHintList result;
    ark::es2panda::ir::AstNode *funcDecl = nullptr;
    ark::es2panda::ir::AstNode *arrowFunc = nullptr;
    parent->FindChild([&funcDecl](ark::es2panda::ir::AstNode *node) {
        if (ark::es2panda::lsp::GetFullWidth(node) != 0 && node->IsMethodDefinition()) {
            funcDecl = node;
            return true;
        }
        return false;
    });
    parent->FindChild([&arrowFunc](ark::es2panda::ir::AstNode *node) {
        if (ark::es2panda::lsp::GetFullWidth(node) != 0 && node->IsArrowFunctionExpression()) {
            arrowFunc = node;
            return true;
        }
        return false;
    });
    ASSERT_NE(funcDecl, nullptr);
    ASSERT_NE(arrowFunc, nullptr);
    ark::es2panda::lsp::GetFunctionReturnTypeForHints(funcDecl, &result);
    ark::es2panda::lsp::GetFunctionReturnTypeForHints(arrowFunc, &result);
    initializer.DestroyContext(ctx);

    ExpectHints(result, {{"Double", funcEnd, InlayHintKind::TYPE, true, false},
                         {"Double", arrowEnd, InlayHintKind::TYPE, true, false}});
}

TEST_F(LSPInlayHints1Tests, DefaultAndVariableOnlyPreferencesYieldNoHints)
{
    std::vector<std::string> files = {"preferences_control.ets"};
    std::vector<std::string> texts = {R"(function foo(param1: number, param2: number): number {
    return param1 + param2;
}
foo(10, 20);
)"};
    const auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    const std::string &source = texts[0];
    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    TextSpan span {0, source.size()};

    UserPreferences offPreferences = UserPreferences::GetDefaultUserPreferences();
    auto resultOff = ProvideHints(ctx, span, offPreferences);

    UserPreferences variablePreferences = UserPreferences::GetDefaultUserPreferences();
    variablePreferences.SetIncludeInlayVariableTypeHints(true);
    auto resultVariable = ProvideHints(ctx, span, variablePreferences);
    initializer.DestroyContext(ctx);

    // Every type-hint preference is off by default; variable type hints alone
    // produce nothing because the source has no variable declaration.
    ASSERT_TRUE(resultOff.hints.empty());
    ASSERT_TRUE(resultVariable.hints.empty());
}

TEST_F(LSPInlayHints1Tests, DISABLED_IndividualKindPreferencesProduceEachHintKind)
{
    std::vector<std::string> files = {"preferences_kinds.ets"};
    std::vector<std::string> texts = {R"(function foo(param1: number, param2: number): number {
    return param1 + param2;
}
foo(10, 20);
)"};
    const auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    const std::string &source = texts[0];
    const auto paramAEnd = source.find(',');
    const auto paramBEnd = source.find(')');
    const auto funcEnd = source.find("\n}\n") + 2;
    const auto arg1 = source.find("10");
    const auto arg2 = source.find("20");
    ASSERT_NE(paramAEnd, std::string::npos);
    ASSERT_NE(paramBEnd, std::string::npos);
    ASSERT_NE(arg1, std::string::npos);
    ASSERT_NE(arg2, std::string::npos);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    TextSpan span {0, source.size()};

    UserPreferences paramTypePreferences = UserPreferences::GetDefaultUserPreferences();
    paramTypePreferences.SetIncludeInlayFunctionParameterTypeHints(true);
    auto resultParamType = ProvideHints(ctx, span, paramTypePreferences);

    UserPreferences returnTypePreferences = UserPreferences::GetDefaultUserPreferences();
    returnTypePreferences.SetIncludeInlayFunctionLikeReturnTypeHints(true);
    auto resultReturnType = ProvideHints(ctx, span, returnTypePreferences);

    UserPreferences paramNamePreferences = UserPreferences::GetDefaultUserPreferences();
    paramNamePreferences.SetIncludeInlayParameterNameHints(UserPreferences::IncludeInlayParameterNameHints::ALL);
    auto resultParamName = ProvideHints(ctx, span, paramNamePreferences);

    auto resultAllKinds = ProvideHintsWithAllPreferences(ctx, span);
    initializer.DestroyContext(ctx);

    // Parameter type and return type hints now reach the public API.
    ExpectHints(resultParamType, {{"number", paramAEnd, InlayHintKind::TYPE, true, false},
                                  {"number", paramBEnd, InlayHintKind::TYPE, true, false}});

    ExpectHints(resultReturnType, {{"Double", funcEnd, InlayHintKind::TYPE, true, false}});

    ExpectHints(resultParamName, {{"param1", arg1, InlayHintKind::PARAMETER, false, true},
                                  {"param2", arg2, InlayHintKind::PARAMETER, false, true}});

    // With all preferences enabled, the traversal order is: param type hints
    // (from MethodDefinition), then return type hint, then parameter name hints
    // (from the call expression).
    ExpectHints(resultAllKinds, {{"number", paramAEnd, InlayHintKind::TYPE, true, false},
                                 {"number", paramBEnd, InlayHintKind::TYPE, true, false},
                                 {"Double", funcEnd, InlayHintKind::TYPE, true, false},
                                 {"param1", arg1, InlayHintKind::PARAMETER, false, true},
                                 {"param2", arg2, InlayHintKind::PARAMETER, false, true}});
}

TEST_F(LSPInlayHints1Tests, VariableTypeHintViaDirectHelper)
{
    std::vector<std::string> files = {"variable_type_hint.ets"};
    std::vector<std::string> texts = {R"(let inferred = "hello";
)"};
    const auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    const std::string &source = texts[0];
    const auto varEnd = source.find(';');
    ASSERT_NE(varEnd, std::string::npos);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    const auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ASSERT_NE(astContext, nullptr);
    auto *parent = reinterpret_cast<ark::es2panda::ir::AstNode *>(astContext->parserProgram->Ast());
    ASSERT_NE(parent, nullptr);

    // Top-level let declarations are wrapped into class properties, so the Visitor's
    // assignment-expression branch never fires for them either; invoke the helper directly.
    InlayHintList result;
    parent->FindChild([&result](ark::es2panda::ir::AstNode *childNode) {
        if (childNode->IsAssignmentExpression()) {
            ark::es2panda::lsp::GetVariableDeclarationTypeForHints(childNode, &result);
        }
        return false;
    });
    initializer.DestroyContext(ctx);

    ASSERT_EQ(result.hints.size(), 1U);
    ExpectHint(result.hints[0], {"string", varEnd, InlayHintKind::TYPE, true, false});
}

TEST_F(LSPInlayHints1Tests, DISABLED_RangeLimitsHintsToSecondCallOnly)
{
    std::vector<std::string> files = {"range_limit.ets"};
    std::vector<std::string> texts = {R"(function bar(a: number, b: string): void {
}
bar(1, "x");
bar(2, "y");
)"};
    const auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    const std::string &source = texts[0];
    const auto paramAEnd = source.find(',');
    const auto paramBEnd = source.find(')');
    const auto funcEnd = source.find("\n}\n") + 2;
    const auto firstCallArg1 = source.find('1');
    const auto firstCallArg2 = source.find("\"x\"");
    const auto secondCall = source.find("bar(2");
    ASSERT_NE(paramAEnd, std::string::npos);
    ASSERT_NE(paramBEnd, std::string::npos);
    ASSERT_NE(firstCallArg1, std::string::npos);
    ASSERT_NE(firstCallArg2, std::string::npos);
    ASSERT_NE(secondCall, std::string::npos);
    const auto secondCallArg1 = source.find('2', secondCall);
    const auto secondCallArg2 = source.find("\"y\"", secondCall);
    ASSERT_NE(secondCallArg1, std::string::npos);
    ASSERT_NE(secondCallArg2, std::string::npos);
    const std::string secondCallText = "bar(2, \"y\");";

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    TextSpan fullSpan {0, source.size()};
    auto resultFull = ProvideHintsWithAllPreferences(ctx, fullSpan);

    TextSpan secondCallSpan {secondCall, secondCallText.size()};
    auto resultSecondCall = ProvideHintsWithAllPreferences(ctx, secondCallSpan);

    TextSpan emptySpan {secondCallArg1, 0};
    auto resultEmpty = ProvideHintsWithAllPreferences(ctx, emptySpan);
    initializer.DestroyContext(ctx);

    // Full span includes param type hints, return type hint, and both calls' param name hints.
    ExpectHints(resultFull, {{"number", paramAEnd, InlayHintKind::TYPE, true, false},
                             {"string", paramBEnd, InlayHintKind::TYPE, true, false},
                             {"undefined", funcEnd, InlayHintKind::TYPE, true, false},
                             {"a", firstCallArg1, InlayHintKind::PARAMETER, false, true},
                             {"b", firstCallArg2, InlayHintKind::PARAMETER, false, true},
                             {"a", secondCallArg1, InlayHintKind::PARAMETER, false, true},
                             {"b", secondCallArg2, InlayHintKind::PARAMETER, false, true}});

    ExpectHints(resultSecondCall, {{"a", secondCallArg1, InlayHintKind::PARAMETER, false, true},
                                   {"b", secondCallArg2, InlayHintKind::PARAMETER, false, true}});

    // A zero-length span intersects the enclosing node of its position, so the hints of the enclosing call remain.
    ExpectHints(resultEmpty, {{"a", secondCallArg1, InlayHintKind::PARAMETER, false, true},
                              {"b", secondCallArg2, InlayHintKind::PARAMETER, false, true}});
}

TEST_F(LSPInlayHints1Tests, DISABLED_ReturnTypeHintsViaPublicApi)
{
    std::vector<std::string> files = {"public_return_type_hints.ets"};
    std::vector<std::string> texts = {R"(function add(a: number, b: number): number {
    return a + b;
}
const mul = (a: number, b: number): number => {
    return a * b;
};
)"};
    const auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    const std::string &source = texts[0];
    const auto funcEnd = source.find("\n}\n") + 2;
    const auto arrowEnd = source.find("\n};") + 2;
    ASSERT_NE(funcEnd, std::string::npos);
    ASSERT_NE(arrowEnd, std::string::npos);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    TextSpan span {0, source.size()};

    UserPreferences preferences = UserPreferences::GetDefaultUserPreferences();
    preferences.SetIncludeInlayFunctionLikeReturnTypeHints(true);
    auto result = ProvideHints(ctx, span, preferences);
    initializer.DestroyContext(ctx);

    // The tree traversal (FindChild) can visit the same logical node through
    // multiple parent paths (e.g., ArrowFunctionExpression inside a const
    // declaration). The production path deduplicates hints with identical
    // position/text/kind/whitespace, so only one hint per position is expected.
    ExpectHints(result, {{"Double", funcEnd, InlayHintKind::TYPE, true, false},
                         {"Double", arrowEnd, InlayHintKind::TYPE, true, false}});
}

TEST_F(LSPInlayHints1Tests, DISABLED_ParameterTypeHintsViaPublicApi)
{
    std::vector<std::string> files = {"public_param_type_hints.ets"};
    std::vector<std::string> texts = {R"(function add(a: number, b: number): number {
    return a + b;
}
)"};
    const auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    const std::string &source = texts[0];
    const auto paramAEnd = source.find(',');
    const auto paramBEnd = source.find(')');
    ASSERT_NE(paramAEnd, std::string::npos);
    ASSERT_NE(paramBEnd, std::string::npos);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    TextSpan span {0, source.size()};

    UserPreferences preferences = UserPreferences::GetDefaultUserPreferences();
    preferences.SetIncludeInlayFunctionParameterTypeHints(true);
    auto result = ProvideHints(ctx, span, preferences);
    initializer.DestroyContext(ctx);

    ExpectHints(result, {{"number", paramAEnd, InlayHintKind::TYPE, true, false},
                         {"number", paramBEnd, InlayHintKind::TYPE, true, false}});
}

TEST_F(LSPInlayHints1Tests, DISABLED_VariableTypeHintViaPublicApi)
{
    std::vector<std::string> files = {"public_var_type_hint.ets"};
    std::vector<std::string> texts = {R"(let inferred = "hello";
)"};
    const auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    const std::string &source = texts[0];
    const auto varEnd = source.find(';');
    ASSERT_NE(varEnd, std::string::npos);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    TextSpan span {0, source.size()};

    UserPreferences preferences = UserPreferences::GetDefaultUserPreferences();
    preferences.SetIncludeInlayVariableTypeHints(true);
    auto result = ProvideHints(ctx, span, preferences);
    initializer.DestroyContext(ctx);

    ExpectHints(result, {{"string", varEnd, InlayHintKind::TYPE, true, false}});
}

}  // namespace
