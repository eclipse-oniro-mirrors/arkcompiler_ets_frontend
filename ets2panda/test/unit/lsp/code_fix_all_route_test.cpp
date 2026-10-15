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

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "generated/code_fix_register.h"
#include "lsp/include/internal_api.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::lsp::Initializer;
// FixIds/supported codes of every fixer targeted by this file come from the generated register.
namespace codefixes = ark::es2panda::lsp::codefixes;

constexpr int DEFAULT_THROTTLE = 20;

// Minimal valid ETS source without any diagnostic: the fix-all route re-creates a CHECKED context
// from the context's own file (CodeFixProvider::GetDiagnostics, code_fix_provider.cpp:126-148) and
// only fires the provider lambda for diagnostics whose int code is registered. With no diagnostics
// at all every lambda is skipped and the combined result stays empty.
const char *MINIMAL_SOURCE = "let x: number = 1;";

class CodeFixAllRouteTests : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static CodeFixOptions MakeOptions()
    {
        return {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    }

    // Runs the fix-all entry for one fixer over `source` and returns the combined result.
    static CombinedCodeActionsInfo GetAllFor(es2panda_Context *ctx, const std::string &fixId)
    {
        CodeFixOptions options = MakeOptions();
        return ark::es2panda::lsp::GetCombinedCodeFixImpl(ctx, fixId, options);
    }

    static bool HasDiagnosticWithIntCode(const DiagnosticReferences &diags, int code)
    {
        for (const auto &diag : diags.diagnostic) {
            if (std::holds_alternative<int>(diag.code_) && std::get<int>(diag.code_) == code) {
                return true;
            }
        }
        return false;
    }

private:
    class NullCancellationToken : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return false;
        }
    };

    static NullCancellationToken &GetNullHost()
    {
        static NullCancellationToken instance;
        return instance;
    }
};

// PD-001 — RemoveCatchTypeAnnotation::GetAllCodeActions (remove_catch_type_annotation.cpp:99)
// via GetCombinedCodeFixImpl -> CodeFixProvider::GetAllFixes -> registration->GetAllCodeActions.
// The minimal source has no SEMANTIC 33781 diagnostic, so the CodeFixAll lambda never fires and
// the combined result is pinned empty.
TEST_F(CodeFixAllRouteTests, RemoveCatchTypeAnnotationAllCodeActionsRoute)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("FixAllRemoveCatchTypeAnnotation.ets", ES2PANDA_STATE_CHECKED, MINIMAL_SOURCE);
    auto combined = GetAllFor(ctx, codefixes::REMOVE_CATCH_TYPE_ANNOTATION.GetFixId().data());

    EXPECT_TRUE(combined.changes_.empty());
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

// PD-002 — FixRemoveAccidentalCallParentheses::GetAllCodeActions
// (remove_accidental_call_parentheses.cpp:124). Minimal source => no matching diagnostic => empty.
TEST_F(CodeFixAllRouteTests, RemoveAccidentalCallParenthesesAllCodeActionsRoute)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("FixAllRemoveAccidentalCallParentheses.ets", ES2PANDA_STATE_CHECKED, MINIMAL_SOURCE);
    auto combined = GetAllFor(ctx, codefixes::REMOVE_ACCIDENTAL_CALL_PARENTHESES.GetFixId().data());

    EXPECT_TRUE(combined.changes_.empty());
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

// PD-003 + PD-025 — ForgottenThisPropertyAccess::GetAllCodeActions
// (forgotten_this_property_access.cpp:89). The source carries a real PROP_ACCESS_WITHOUT_THIS
// diagnostic (SEMANTIC 145 -> int 2145, thrown at checker/ets/helpers.cpp:247) whose int code is
// registered by the fixer, so the fix-all lambda fires and rewrites `p` to `this.p`.
TEST_F(CodeFixAllRouteTests, ForgottenThisPropertyAccessAllCodeActionsRoute)
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
        initializer.CreateContext("FixAllForgottenThis.ets", ES2PANDA_STATE_CHECKED, source.c_str());

    // Precondition probe: the re-created fix-all context runs the same CHECKED pipeline over the
    // same source, so the registered int code must be visible through getSemanticDiagnostics.
    LSPAPI const *lspApi = GetImpl();
    auto diagnostics = lspApi->getSemanticDiagnostics(ctx);
    // Probe result (measured): the source carries exactly one semantic diagnostic for `p`
    // (1-based line 4, char 9..10) with int code SEMANTIC*1000+145 and message
    // "Property 'p' must be accessed through 'this'". EachDiagnostic feeds the same 1-based
    // line/character through lexer::SourceLocation, so the fix-all lambda receives the real
    // `p` offset and fires (covers the PD-025 lambda body).
    const int registeredCode = codefixes::FORGOTTEN_THIS_PROPERTY_ACCESS.GetSupportedCodeNumbers().at(0);
    ASSERT_TRUE(HasDiagnosticWithIntCode(diagnostics, registeredCode))
        << "Expected a diagnostic with int code " << registeredCode << " for the forgotten-this source";

    auto combined = GetAllFor(ctx, codefixes::FORGOTTEN_THIS_PROPERTY_ACCESS.GetFixId().data());

    // The fix-all lambda fires and produces the "this.p" replacement. Measured current edit
    // shape: span {0,0} with newText "this.p". Root cause (product finding): DoChanges
    // re-parents the original `p` node onto a detached synthetic member expression
    // (forgotten_this_property_access.cpp:57-58) before ReplaceNode computes its edit range,
    // so ChangeTracker::MakeEditRange (change_tracker.cpp:101-108) reads Start/End indexes of
    // the detached node and yields {0,0}.
    ASSERT_EQ(combined.changes_.size(), 1U);
    EXPECT_EQ(combined.changes_[0].fileName, "FixAllForgottenThis.ets");
    ASSERT_EQ(combined.changes_[0].textChanges.size(), 1U);
    EXPECT_EQ(combined.changes_[0].textChanges[0].span.start, 0U);
    EXPECT_EQ(combined.changes_[0].textChanges[0].span.length, 0U);
    EXPECT_EQ(combined.changes_[0].textChanges[0].newText, "this.p");
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

// PD-004 — FixSpelling::GetAllCodeActions route. The fixer's constructor is empty
// (fix_spelling.cpp:29-32, "// #34663"): it never calls SetErrorCodes/SetFixIds, so
// AutoCodeFixRegister registers it under neither errorCodeToFixes_ nor fixIdToRegistration_
// (code_fix_provider.cpp:34-43). GetAllFixes misses the lookup and returns an empty result at
// code_fix_provider.cpp:172-175 without ever entering FixSpelling::GetAllCodeActions. The source
// below *does* carry "Unresolved reference cots" (SEMANTIC 143 -> int 2143, a FIX_SPELLING code
// per the generated register), which pins the defect as registration-side, not diagnostic-side.
TEST_F(CodeFixAllRouteTests, FixSpellingAllCodeActionsRoute)
{
    const std::string source = "let cot: number = 1;\nlet y = cots;";
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("FixAllFixSpelling.ets", ES2PANDA_STATE_CHECKED, source.c_str());

    LSPAPI const *lspApi = GetImpl();
    auto diagnostics = lspApi->getSemanticDiagnostics(ctx);
    ASSERT_TRUE(HasDiagnosticWithIntCode(diagnostics, codefixes::FIX_SPELLING.GetSupportedCodeNumbers().at(1)))
        << "Expected 'Unresolved reference' diagnostic with int code "
        << codefixes::FIX_SPELLING.GetSupportedCodeNumbers().at(1);

    auto combined = GetAllFor(ctx, codefixes::FIX_SPELLING.GetFixId().data());

    EXPECT_TRUE(combined.changes_.empty());
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

// PD-005 — FixReturnTypeInAsyncFunction::GetAllCodeActions
// (fix_return_type_in_async_function.cpp:128). Minimal source => empty.
TEST_F(CodeFixAllRouteTests, FixReturnTypeInAsyncFunctionAllCodeActionsRoute)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("FixAllReturnTypeAsync.ets", ES2PANDA_STATE_CHECKED, MINIMAL_SOURCE);
    auto combined = GetAllFor(ctx, codefixes::FIX_RETURN_TYPE_IN_ASYNC_FUNCTION.GetFixId().data());

    EXPECT_TRUE(combined.changes_.empty());
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

// PD-006 — FixRemoveOverrideModifier::GetAllCodeActions (fix_remove_override_modifier.cpp:94).
// Minimal source => empty.
TEST_F(CodeFixAllRouteTests, RemoveOverrideModifierAllCodeActionsRoute)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("FixAllRemoveOverrideModifier.ets", ES2PANDA_STATE_CHECKED, MINIMAL_SOURCE);
    auto combined = GetAllFor(ctx, codefixes::REMOVE_OVERRIDE_MODIFIER.GetFixId().data());

    EXPECT_TRUE(combined.changes_.empty());
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

// PD-007 — FixImportNonExportedMember::GetAllCodeActions
// (fix_import_non_exported_member.cpp:241). Minimal source => empty.
TEST_F(CodeFixAllRouteTests, FixImportNonExportedMemberAllCodeActionsRoute)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("FixAllImportNonExportedMember.ets", ES2PANDA_STATE_CHECKED, MINIMAL_SOURCE);
    auto combined = GetAllFor(ctx, codefixes::FIX_IMPORT_NON_EXPORTED_MEMBER.GetFixId().data());

    EXPECT_TRUE(combined.changes_.empty());
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

// PD-008 — FixForbiddenAnyType::GetAllCodeActions (fix_forbidden_any_type.cpp:76).
// Minimal source => empty.
TEST_F(CodeFixAllRouteTests, FixForbiddenAnyTypeAllCodeActionsRoute)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("FixAllForbiddenAnyType.ets", ES2PANDA_STATE_CHECKED, MINIMAL_SOURCE);
    auto combined = GetAllFor(ctx, codefixes::FIX_FORBIDDEN_ANY_TYPE.GetFixId().data());

    EXPECT_TRUE(combined.changes_.empty());
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

// PD-009 — FixExtendsInterfaceBecomesImplements::GetAllCodeActions
// (fix_extends_interface_becomes_implements.cpp:100). Minimal source => empty.
TEST_F(CodeFixAllRouteTests, ExtendsInterfaceBecomesImplementsAllCodeActionsRoute)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("FixAllExtendsInterfaceBecomesImplements.ets",
                                                      ES2PANDA_STATE_CHECKED, MINIMAL_SOURCE);
    auto combined = GetAllFor(ctx, codefixes::EXTENDS_INTERFACE_BECOMES_IMPLEMENTS.GetFixId().data());

    EXPECT_TRUE(combined.changes_.empty());
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

// PD-010 — FixClassSuperMustPrecedeThisAccess::GetAllCodeActions
// (fix_class_super_must_precede_this_access.cpp:131). Minimal source => empty.
TEST_F(CodeFixAllRouteTests, ClassSuperMustPrecedeThisAccessAllCodeActionsRoute)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("FixAllSuperMustPrecedeThis.ets", ES2PANDA_STATE_CHECKED, MINIMAL_SOURCE);
    auto combined = GetAllFor(ctx, codefixes::CLASS_SUPER_MUST_PRECEDE_THIS_ACCESS.GetFixId().data());

    EXPECT_TRUE(combined.changes_.empty());
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

// PD-011 — FixAddFunctionReturnStatement::GetAllCodeActions is a stub that returns an empty
// CombinedCodeActions unconditionally (fix_add_function_return_statement.cpp:63-67), even when
// the source really carries the registered diagnostic: `function foo(): number {}` produces
// "Function with non-void/undefined return type must return a value." (SEMANTIC 103 -> int 2103).
// The test pins the stub behavior with the diagnostic precondition asserted.
TEST_F(CodeFixAllRouteTests, AddFunctionReturnStatementAllCodeActionsRoute)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("FixAllAddFunctionReturn.ets", ES2PANDA_STATE_CHECKED, "function foo(): number {}");

    LSPAPI const *lspApi = GetImpl();
    auto diagnostics = lspApi->getSemanticDiagnostics(ctx);
    ASSERT_TRUE(HasDiagnosticWithIntCode(diagnostics,
                                         codefixes::FIX_ADD_FUNCTION_RETURN_STATEMENT.GetSupportedCodeNumbers().at(1)))
        << "Expected missing-return diagnostic with int code "
        << codefixes::FIX_ADD_FUNCTION_RETURN_STATEMENT.GetSupportedCodeNumbers().at(1);

    auto combined = GetAllFor(ctx, codefixes::FIX_ADD_FUNCTION_RETURN_STATEMENT.GetFixId().data());

    EXPECT_TRUE(combined.changes_.empty());
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

// PD-012 — ConstructorDerivedNeedSuper::GetAllCodeActions is a stub returning {}
// (constructor_for_derived_need_super_call.cpp:181-183). Minimal source => empty.
TEST_F(CodeFixAllRouteTests, ConstructorDerivedNeedSuperAllCodeActionsRoute)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("FixAllConstructorDerivedNeedSuper.ets", ES2PANDA_STATE_CHECKED, MINIMAL_SOURCE);
    auto combined = GetAllFor(ctx, codefixes::CONSTRUCTOR_DERIVED_NEED_SUPER.GetFixId().data());

    EXPECT_TRUE(combined.changes_.empty());
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

// PD-013 — AddNameToNamelessParameter::GetAllCodeActions
// (add_name_to_nameless_parameter.cpp:201). Minimal source => empty.
TEST_F(CodeFixAllRouteTests, AddNameToNamelessParameterAllCodeActionsRoute)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("FixAllAddNameToNamelessParameter.ets", ES2PANDA_STATE_CHECKED, MINIMAL_SOURCE);
    auto combined = GetAllFor(ctx, codefixes::ADD_NAME_TO_NAMELESS_PARAMETER.GetFixId().data());

    EXPECT_TRUE(combined.changes_.empty());
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

// PD-014 — FixAddMissingNewOperator::GetAllCodeActions
// (add_missing_new_operator.cpp:124). Minimal source => empty.
TEST_F(CodeFixAllRouteTests, AddMissingNewOperatorAllCodeActionsRoute)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("FixAllAddMissingNewOperator.ets", ES2PANDA_STATE_CHECKED, MINIMAL_SOURCE);
    auto combined = GetAllFor(ctx, codefixes::FIX_ADD_MISSING_NEW_OPERATOR.GetFixId().data());

    EXPECT_TRUE(combined.changes_.empty());
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

// PD-015 — AddMissingDeclareProperty::GetAllCodeActions (add_missing_declare_property.cpp:97).
// Minimal source has no PROPERTY_MAYBE_MISSING_INIT diagnostic, so the fix-all lambda never fires.
TEST_F(CodeFixAllRouteTests, AddMissingDeclarePropertyAllCodeActionsRoute)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("FixAllAddMissingDeclareProperty.ets", ES2PANDA_STATE_CHECKED, MINIMAL_SOURCE);
    auto combined = GetAllFor(ctx, codefixes::ADD_MISSING_DECLARE_PROPERTY.GetFixId().data());

    EXPECT_TRUE(combined.changes_.empty());
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

// Control: a fixId that no provider registered must produce an empty combined result through the
// CodeFixProvider::GetAllFixes lookup miss (code_fix_provider.cpp:172-175).
TEST_F(CodeFixAllRouteTests, UnknownFixIdReturnsEmpty)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("FixAllUnknownId.ets", ES2PANDA_STATE_CHECKED, MINIMAL_SOURCE);
    auto combined = GetAllFor(ctx, "CodeFixThatDoesNotExist");

    EXPECT_TRUE(combined.changes_.empty());
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

}  // namespace
