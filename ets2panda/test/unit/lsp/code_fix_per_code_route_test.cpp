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
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "generated/code_fix_register.h"
#include "lsp/include/internal_api.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::lsp::Initializer;
// FixIds/supported codes of the per-error-code routes targeted by this file.
namespace codefixes = ark::es2panda::lsp::codefixes;

constexpr int DEFAULT_THROTTLE = 20;

class CodeFixPerCodeRouteTests : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static ark::es2panda::lsp::CancellationToken CreateCancellingToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetRequestingHost());
    }

    static size_t LineColToPos(es2panda_Context *context, const size_t line, const size_t col)
    {
        // line/column are 1-based
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        return index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
    }

    static std::vector<CodeFixActionInfo> GetFixesAt(es2panda_Context *ctx, size_t start, size_t end,
                                                     const std::vector<int> &errorCodes)
    {
        CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
        std::vector<int> codes = errorCodes;
        return ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, start, end, codes, options);
    }

private:
    class NullCancellationToken : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return false;
        }
    };

    class RequestingCancellationToken : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return true;
        }
    };

    static NullCancellationToken &GetNullHost()
    {
        static NullCancellationToken instance;
        return instance;
    }

    static RequestingCancellationToken &GetRequestingHost()
    {
        static RequestingCancellationToken instance;
        return instance;
    }
};

// PD-016/017/018/019 — ForgottenThisPropertyAccess per-error-code route: one query clears
// GetCodeActions (forgotten_this_property_access.cpp:73) -> GetCodeActionsToFix (:65) ->
// DoChanges (:44) -> GetInfoThisProp (:30). The source carries a real PROP_ACCESS_WITHOUT_THIS
// diagnostic (checker/ets/helpers.cpp:247); querying the bare property identifier in the
// assignment with the registered SEMANTIC 145 int code yields the "this." insertion fix.
TEST_F(CodeFixPerCodeRouteTests, ForgottenThisPropertyAccessPerCodeRoute)
{
    const std::string source =
        "class C {\n"
        "    p: number = 0;\n"
        "    m(): void {\n"
        "        p = 1;\n"
        "    }\n"
        "}";
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("PerCodeForgottenThis.ets", ES2PANDA_STATE_CHECKED, source.c_str());

    // Bare `p` on line 4, column 9 (1-based); span covers exactly the identifier.
    const size_t start = LineColToPos(ctx, 4, 9);
    const size_t length = 1;
    auto actions = GetFixesAt(ctx, start, start + length,
                              {codefixes::FORGOTTEN_THIS_PROPERTY_ACCESS.GetSupportedCodeNumbers().at(0)});

    // Measured: exactly one action from ForgottenThisPropertyAccess with its exact metadata.
    ASSERT_EQ(actions.size(), 1U);
    EXPECT_EQ(actions[0].fixName_, "ForgottenThisPropertyAccess");
    EXPECT_EQ(actions[0].fixId_, "ForgottenThisPropertyAccess");
    EXPECT_EQ(actions[0].description_, "Add 'this.' to property access");
    EXPECT_EQ(actions[0].fixAllDescription_, "Add 'this.' to all property accesses in the file");
    EXPECT_EQ(actions[0].additionalMessage_, "");
    EXPECT_TRUE(actions[0].commands_.empty());
    ASSERT_EQ(actions[0].changes_.size(), 1U);
    EXPECT_EQ(actions[0].changes_[0].fileName, "PerCodeForgottenThis.ets");
    ASSERT_EQ(actions[0].changes_[0].textChanges.size(), 1U);
    // Measured current edit shape: span {0,0} newText "this.p". Product finding: DoChanges
    // re-parents the `p` identifier onto a detached synthetic member expression
    // (forgotten_this_property_access.cpp:57-58) before ReplaceNode -> MakeEditRange
    // (change_tracker.cpp:101-108) computes the range, so the detached node yields {0,0}:
    // the edit inserts "this.p" at the file start instead of replacing the identifier span.
    EXPECT_EQ(actions[0].changes_[0].textChanges[0].span.start, 0U);
    EXPECT_EQ(actions[0].changes_[0].textChanges[0].span.length, 0U);
    EXPECT_EQ(actions[0].changes_[0].textChanges[0].newText, "this.p");

    initializer.DestroyContext(ctx);
}

// PD-020/021 — AddMissingDeclareProperty per-error-code route:
// GetCodeActions (add_missing_declare_property.cpp:77) -> GetCodeActionsToAddMissingDeclareOnProperty
// (:66) -> MakeChangeAddMissing (:33) inserting "declare " at the touching identifier.
// Registered code: PROPERTY_MAYBE_MISSING_INIT SEMANTIC 280.
TEST_F(CodeFixPerCodeRouteTests, AddMissingDeclarePropertyPerCodeRoute)
{
    const std::string source =
        "class B {\n"
        "    a: number;\n"
        "}";
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("PerCodeAddMissingDeclare.ets", ES2PANDA_STATE_CHECKED, source.c_str());

    // Property name `a` on line 2, column 5 (1-based).
    const size_t start = LineColToPos(ctx, 2, 5);
    const size_t length = 1;
    auto actions = GetFixesAt(ctx, start, start + length,
                              {codefixes::ADD_MISSING_DECLARE_PROPERTY.GetSupportedCodeNumbers().at(0)});

    // Measured: the route accepts any touching identifier with a Statement ancestor
    // (add_missing_declare_property.cpp:41-58) and inserts "declare " at the query position.
    ASSERT_EQ(actions.size(), 1U);
    EXPECT_EQ(actions[0].fixName_, "AddMissingDeclareProperty");
    EXPECT_EQ(actions[0].fixId_, "AddMissingDeclareProperty");
    EXPECT_EQ(actions[0].description_, "Fix Description");
    EXPECT_EQ(actions[0].fixAllDescription_, "Fix All Description");
    ASSERT_EQ(actions[0].changes_.size(), 1U);
    EXPECT_EQ(actions[0].changes_[0].fileName, "PerCodeAddMissingDeclare.ets");
    ASSERT_EQ(actions[0].changes_[0].textChanges.size(), 1U);
    EXPECT_EQ(actions[0].changes_[0].textChanges[0].span.start, start);
    EXPECT_EQ(actions[0].changes_[0].textChanges[0].span.length, 0U);
    EXPECT_EQ(actions[0].changes_[0].textChanges[0].newText, "declare ");
    // Measured command payload from the InstallPackageAction built in GetCodeActions
    // (add_missing_declare_property.cpp:88-91).
    ASSERT_EQ(actions[0].commands_.size(), 1U);
    EXPECT_EQ(actions[0].commands_[0].type_, "install package");
    ASSERT_TRUE(actions[0].commands_[0].file.has_value());
    EXPECT_EQ(actions[0].commands_[0].file.value(), "addMissingDeclareProperty.ets");
    ASSERT_TRUE(actions[0].commands_[0].packageName.has_value());
    EXPECT_EQ(actions[0].commands_[0].packageName.value(), "dummy-package");

    initializer.DestroyContext(ctx);
}

// PD-022 + PD-027 — FixAddFunctionReturnStatement per-error-code route is guarded EMPTY for
// every real ETS function source. Chain of evidence (measured):
// 1. GetCodeActions (fix_add_function_return_statement.cpp:37) delegates to GetInfo (:69-91),
//    which resolves the touching identifier via GetDefinitionAtPositionImpl and walks to the
//    nearest FunctionDeclaration/FunctionExpression ancestor (FindAncessor :140-149).
// 2. For a top-level `function` the ancestor is the FunctionDeclaration wrapper; the guard at
//    fix_add_function_return_statement.cpp:77-78 (`!declaration->IsFunctionExpression()`)
//    rejects it, and ir::ScriptFunction has node type SCRIPT_FUNCTION, not FUNCTION_EXPRESSION
//    (ir/base/scriptFunction.cpp:131-132).
// 3. A persistent ir::FunctionExpression is unreachable through the ETS parser: ETS only builds
//    one transiently in object-literal parsing and immediately converts it to an
//    ArrowFunctionExpression (ETSparserExpressions.cpp:260-269).
// Therefore GetCodeActions returns empty and the ReplaceReturnType/AddReturnStatement
// ChangeTracker lambdas (PD-027, :46-50) never execute through the public route.
// The diagnostic side is healthy: the source really carries SEMANTIC 103 (int 2103).
TEST_F(CodeFixPerCodeRouteTests, AddFunctionReturnStatementGuardedEmptyPerCodeRoute)
{
    const std::string source =
        "function foo(a: number): number {\n"
        "    a;\n"
        "}";
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("PerCodeAddFunctionReturn.ets", ES2PANDA_STATE_CHECKED, source.c_str());

    LSPAPI const *lspApi = GetImpl();
    auto diagnostics = lspApi->getSemanticDiagnostics(ctx);
    const int missingReturnCode = codefixes::FIX_ADD_FUNCTION_RETURN_STATEMENT.GetSupportedCodeNumbers().at(1);
    ASSERT_TRUE(std::any_of(diagnostics.diagnostic.begin(), diagnostics.diagnostic.end(),
                            [code = missingReturnCode](const Diagnostic &diag) {
                                return std::holds_alternative<int>(diag.code_) && std::get<int>(diag.code_) == code;
                            }))
        << "Expected missing-return diagnostic with int code " << missingReturnCode;

    // Function name `foo`, line 1, column 10 (1-based).
    const size_t start = LineColToPos(ctx, 1, 10);
    const size_t length = 3;
    auto actions = GetFixesAt(ctx, start, start + length, {missingReturnCode});

    EXPECT_TRUE(actions.empty());

    initializer.DestroyContext(ctx);
}

// PD-023 — FixSpelling::GetCodeActions (fix_spelling.cpp:78) is unreachable through the public
// per-error-code route: the constructor (fix_spelling.cpp:29-32) never registers its error codes
// (SEMANTIC 356/143) nor its fixId, so CodeFixProvider::GetFixes (code_fix_provider.cpp:231-252)
// cannot find the registration for the "Unresolved reference" int code. The source carries the
// real diagnostic; the query must therefore come back WITHOUT any FixSpelling action.
TEST_F(CodeFixPerCodeRouteTests, FixSpellingNotRegisteredPerCodeRoute)
{
    const std::string source = "let cot: number = 1;\nlet y = cots;";
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("PerCodeFixSpelling.ets", ES2PANDA_STATE_CHECKED, source.c_str());

    // Misspelled `cots` on line 2, column 9 (1-based), length 4.
    const size_t start = LineColToPos(ctx, 2, 9);
    const size_t length = 4;
    // FIX_SPELLING's registered codes per the generated register are SEMANTIC 356 and 143; use
    // the int code value the checker really emits (SEMANTIC 143, measured in the all-route
    // suite) for the query.
    const int unresolvedReferenceCode = 2 * 1000 + 143;
    auto actions = GetFixesAt(ctx, start, start + length, {unresolvedReferenceCode});

    // Measured: zero actions for the unresolved-reference code at the misspelled identifier,
    // because the spelling fix provider is unregistered (its constructor registers neither its
    // error codes nor its fix id, fix_spelling.cpp:29-32 and code_fix_provider.cpp:34-43);
    // the other registrations sharing SEMANTIC 143 (FIX_IMPORT_SOURCE, ADD_LOCAL_VARIABLE) do
    // not produce an action at this position either.
    EXPECT_TRUE(actions.empty());

    initializer.DestroyContext(ctx);
}

// PD-024 — FixRemoveOverrideModifier per-error-code route reaches
// MakeChangeForRemoveOverrideModifier (fix_remove_override_modifier.cpp:40): RfindNearestKeyWordTextRange
// locates the `override` keyword range and DeleteRange removes it. NOTE: the ledger assumed
// FindNearestClass (:29) is reached here, but it has zero callers repo-wide (dead free function)
// — this test cannot cover it; recorded as a product finding instead.
TEST_F(CodeFixPerCodeRouteTests, RemoveOverrideModifierPerCodeRoute)
{
    const std::string source =
        "class D {\n"
        "    override foo(): void {}\n"
        "}";
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("PerCodeRemoveOverride.ets", ES2PANDA_STATE_CHECKED, source.c_str());

    // Method name `foo` on line 2, column 14 (1-based).
    const size_t start = LineColToPos(ctx, 2, 14);
    const size_t length = 3;
    auto actions =
        GetFixesAt(ctx, start, start + length, {codefixes::REMOVE_OVERRIDE_MODIFIER.GetSupportedCodeNumbers().at(0)});

    // Measured: one action with exact metadata and a deletion edit covering exactly the
    // 8-character `override` keyword found by RfindNearestKeyWordTextRange
    // (fix_remove_override_modifier.cpp:50-56).
    ASSERT_EQ(actions.size(), 1U);
    EXPECT_EQ(actions[0].fixName_, "RemoveOverrideModifier");
    EXPECT_EQ(actions[0].fixId_, "RemoveOverrideModifier");
    EXPECT_EQ(actions[0].description_, "Remove override modifier");
    EXPECT_EQ(actions[0].fixAllDescription_, "Remove all unnecessary override modifiers");
    ASSERT_EQ(actions[0].changes_.size(), 1U);
    EXPECT_EQ(actions[0].changes_[0].fileName, "PerCodeRemoveOverride.ets");
    ASSERT_EQ(actions[0].changes_[0].textChanges.size(), 1U);
    // `override` starts on line 2 at column 5 (1-based).
    const size_t overrideKeywordLine = 2;
    const size_t overrideKeywordColumn = 5;
    EXPECT_EQ(actions[0].changes_[0].textChanges[0].span.start,
              LineColToPos(ctx, overrideKeywordLine, overrideKeywordColumn));
    EXPECT_EQ(actions[0].changes_[0].textChanges[0].span.length, 8U);
    EXPECT_EQ(actions[0].changes_[0].textChanges[0].newText, "");

    initializer.DestroyContext(ctx);
}

// Negative control for the per-error-code dispatch loop: a cancellation token whose host reports
// a request stops the loop before any provider is consulted
// (internal_api.cpp:1676-1678), so the result is empty even for a registered code.
TEST_F(CodeFixPerCodeRouteTests, CancelledTokenPerCodeRouteReturnsEmpty)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("PerCodeCancelled.ets", ES2PANDA_STATE_CHECKED, "let x: number = 1;");
    ark::es2panda::lsp::CancellationToken cancellingToken = CreateCancellingToken();
    CodeFixOptions options = {cancellingToken, ark::es2panda::lsp::FormatCodeSettings(), {}};
    std::vector<int> codes = {codefixes::FORGOTTEN_THIS_PROPERTY_ACCESS.GetSupportedCodeNumbers().at(0)};
    auto actions = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, 0, 1, codes, options);

    EXPECT_TRUE(actions.empty());

    initializer.DestroyContext(ctx);
}

}  // namespace
