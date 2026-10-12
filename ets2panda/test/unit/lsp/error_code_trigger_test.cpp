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
#include "lsp_api_test.h"
#include "lsp/include/api.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

namespace {

using ::DiagnosticReferences;
using ark::es2panda::lsp::Initializer;

class LspErrorCodeTriggerTests : public LSPAPITests {};

// Helper to find a diagnostic containing the expected message substring
static bool FindDiagnosticWithMessage(const DiagnosticReferences &diags, const std::string &substr)
{
    for (const auto &diag : diags.diagnostic) {
        if (diag.message_.find(substr) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// =============================================================================
// P4-ECT-01: Trigger_UndeclaredVariable_ReportsDiagnostic
// ETS error: ESE0143 "Unresolved reference"
// =============================================================================
TEST_F(LspErrorCodeTriggerTests, Trigger_UndeclaredVariable_ReportsDiagnostic)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x = undeclaredVar;)";
    es2panda_Context *ctx = initializer.CreateContext("ect01.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    ASSERT_GT(result.diagnostic.size(), 0U);

    // ETS: ESE0143 "Unresolved reference"
    EXPECT_TRUE(FindDiagnosticWithMessage(result, "Unresolved reference"))
        << "Expected diagnostic containing 'Unresolved reference' (ESE0143)";

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P4-ECT-02: Trigger_TypeMismatch_ReportsDiagnostic
// ETS error: ESE0318 "cannot be assigned"
// =============================================================================
TEST_F(LspErrorCodeTriggerTests, Trigger_TypeMismatch_ReportsDiagnostic)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x: number = "hello";)";
    es2panda_Context *ctx = initializer.CreateContext("ect02.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    ASSERT_GT(result.diagnostic.size(), 0U);

    // ETS: ESE0318 "Type '"hello"' cannot be assigned to type 'Double'"
    EXPECT_TRUE(FindDiagnosticWithMessage(result, "cannot be assigned"))
        << "Expected diagnostic containing 'cannot be assigned' (ESE0318)";

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P4-ECT-03: Trigger_MissingReturn_ReportsDiagnostic
// ETS error: ESE0103 "must return a value"
// =============================================================================
TEST_F(LspErrorCodeTriggerTests, Trigger_MissingReturn_ReportsDiagnostic)
{
    Initializer initializer = Initializer();
    const std::string source = R"(function foo(): number {})";
    es2panda_Context *ctx = initializer.CreateContext("ect03.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    ASSERT_GT(result.diagnostic.size(), 0U);

    // ETS: ESE0103 "Function with non-void/undefined return type must return a value."
    EXPECT_TRUE(FindDiagnosticWithMessage(result, "must return a value"))
        << "Expected diagnostic containing 'must return a value' (ESE0103)";

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P4-ECT-04: Trigger_DuplicateExport_ReportsDiagnostic
// ETS error: ESE0130 "is already declared"
// =============================================================================
// get_diagnostics_2.cpp pins the exact range/codefix link for duplicate IMPORT;
// this pins the duplicate-EXPORT message (Trigger_IllegalAwait and Trigger_DuplicateImport
// were removed as subsets of fix_remove_illegal_await/get_diagnostics_2 pins).
TEST_F(LspErrorCodeTriggerTests, Trigger_DuplicateExport_ReportsDiagnostic)
{
    Initializer initializer = Initializer();
    const std::string source = R"(export function foo(): void {} export function foo(): void {})";
    es2panda_Context *ctx = initializer.CreateContext("ect04.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    ASSERT_GT(result.diagnostic.size(), 0U);

    // ETS: ESE0130 "Function foo is already declared."
    EXPECT_TRUE(FindDiagnosticWithMessage(result, "already declared"))
        << "Expected diagnostic containing 'already declared' (ESE0130)";

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P4-ECT-05: Trigger_UnreachableCode_ReportsDiagnostic
// ETS warning: W0026 "Unreachable statement"
// Note: getSemanticDiagnostics does surface warnings (verified in branch_coverage_test.cpp BC-10)
// =============================================================================
TEST_F(LspErrorCodeTriggerTests, Trigger_UnreachableCode_ReportsDiagnostic)
{
    Initializer initializer = Initializer();
    const std::string source = R"(function bar(): void { return; let x = 1; })";
    es2panda_Context *ctx = initializer.CreateContext("ect05.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    ASSERT_GT(result.diagnostic.size(), 0U);

    // ETS: W0026 "Unreachable statement."
    EXPECT_TRUE(FindDiagnosticWithMessage(result, "Unreachable statement"))
        << "Expected diagnostic containing 'Unreachable statement' (W0026)";

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P4-ECT-07: Trigger_ClassIncorrectlyImplements_ReportsDiagnostic
// ETS error: ESE0190 "does not override abstract method"
// =============================================================================
TEST_F(LspErrorCodeTriggerTests, Trigger_ClassIncorrectlyImplements_ReportsDiagnostic)
{
    Initializer initializer = Initializer();
    const std::string source = R"(interface I { foo(): void; } class C implements I {})";
    es2panda_Context *ctx = initializer.CreateContext("ect07.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    ASSERT_GT(result.diagnostic.size(), 0U);

    // ETS: ESE0190 "C is not abstract and does not override abstract method foo(): undefined in I"
    EXPECT_TRUE(FindDiagnosticWithMessage(result, "does not override abstract method"))
        << "Expected diagnostic containing 'does not override abstract method' (ESE0190)";

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P4-ECT-08: Trigger_ConstToLet_ReportsDiagnostic
// ETS error: ESE0297 "Cannot reassign constant"
// =============================================================================
TEST_F(LspErrorCodeTriggerTests, Trigger_ConstToLet_ReportsDiagnostic)
{
    Initializer initializer = Initializer();
    const std::string source = R"(const x = 1; x = 2;)";
    es2panda_Context *ctx = initializer.CreateContext("ect08.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    ASSERT_GT(result.diagnostic.size(), 0U);

    // ETS: ESE0297 "Cannot reassign constant x"
    EXPECT_TRUE(FindDiagnosticWithMessage(result, "Cannot reassign constant"))
        << "Expected diagnostic containing 'Cannot reassign constant' (ESE0297)";

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P4-ECT-09: Trigger_ExtendsToImplements_ReportsDiagnostic
// ETS error: ESE0176 "super type is not extensible" (class extends interface)
// =============================================================================
TEST_F(LspErrorCodeTriggerTests, Trigger_ExtendsToImplements_ReportsDiagnostic)
{
    Initializer initializer = Initializer();
    const std::string source = R"(interface I {} class C extends I {})";
    es2panda_Context *ctx = initializer.CreateContext("ect09.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    ASSERT_GT(result.diagnostic.size(), 0U);

    // ETS: ESE0176 "The super type of 'C' class is not extensible."
    EXPECT_TRUE(FindDiagnosticWithMessage(result, "not extensible"))
        << "Expected diagnostic containing 'not extensible' (ESE0176)";

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P4-ECT-10: Trigger_AbstractMember_ReportsDiagnostic
// ETS error: ESE0190 "does not override abstract method"
// (Same as ECT-07 but via abstract class instead of interface)
// =============================================================================
TEST_F(LspErrorCodeTriggerTests, Trigger_AbstractMember_ReportsDiagnostic)
{
    Initializer initializer = Initializer();
    const std::string source = R"(abstract class C { abstract foo(): void; } class D extends C {})";
    es2panda_Context *ctx = initializer.CreateContext("ect10.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    ASSERT_GT(result.diagnostic.size(), 0U);

    // ETS: ESE0190 "D is not abstract and does not override abstract method foo(): undefined in C"
    EXPECT_TRUE(FindDiagnosticWithMessage(result, "does not override abstract method"))
        << "Expected diagnostic containing 'does not override abstract method' (ESE0190)";

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P4-ECT-12: Trigger_ForbiddenAnyType_ReportsDiagnostic
// ETS error: ESE153545 "'any' type annotation is forbidden"
// =============================================================================
TEST_F(LspErrorCodeTriggerTests, Trigger_ForbiddenAnyType_ReportsDiagnostic)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x: any = 1;)";
    es2panda_Context *ctx = initializer.CreateContext("ect12.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    ASSERT_GT(result.diagnostic.size(), 0U);

    // ETS: ESE153545 "'any' type annotation is forbidden"
    EXPECT_TRUE(FindDiagnosticWithMessage(result, "'any' type annotation is forbidden"))
        << "Expected diagnostic containing ''any' type annotation is forbidden' (ESE153545)";

    initializer.DestroyContext(ctx);
}

}  // namespace