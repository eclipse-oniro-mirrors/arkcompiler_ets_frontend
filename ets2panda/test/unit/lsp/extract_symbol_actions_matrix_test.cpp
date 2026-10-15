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
#include <memory>
#include <string>
#include <vector>
#include "lsp/include/refactors/extract_symbol.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/get_edits_for_refactor.h"
#include "lsp/include/types.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/user_preferences.h"
#include "lsp/include/internal_api.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "public/public.h"

namespace {

using ark::es2panda::lsp::ApplicableRefactorInfo;
using ark::es2panda::lsp::FormatCodeSettings;
using ark::es2panda::lsp::FormatContext;
using ark::es2panda::lsp::GetDefaultFormatCodeSettings;
using ark::es2panda::lsp::GetFormatContext;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::RefactorContext;
using ark::es2panda::lsp::RefactorEditInfo;
using ark::es2panda::lsp::UserPreferences;

std::string ApplyEdits(const std::string &original, const std::vector<::TextChange> &edits)
{
    std::vector<const ::TextChange *> ordered;
    ordered.reserve(edits.size());
    for (const auto &change : edits) {
        ordered.push_back(&change);
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const ::TextChange *lhs, const ::TextChange *rhs) { return lhs->span.start < rhs->span.start; });

    std::string result;
    result.reserve(original.size());
    size_t cursor = 0;
    for (const auto *change : ordered) {
        size_t start = std::min(change->span.start, original.size());
        if (start < cursor) {
            start = cursor;
        }
        size_t end = std::min(start + change->span.length, static_cast<size_t>(original.size()));
        if (cursor < start) {
            result.append(original, cursor, start - cursor);
        }
        result.append(change->newText);
        cursor = end;
    }
    if (cursor < original.size()) {
        result.append(original, cursor, original.size() - cursor);
    }
    return result;
}

class ExtractSymbolActionsMatrixTests : public LSPAPITests {
public:
    RefactorContext *MakeContext(Initializer *initializer, const std::string &fileName, const std::string &code,
                                 size_t start, size_t end)
    {
        auto filePaths = CreateTempFile({fileName}, {code});
        auto *ctx = initializer->CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
        if (ctx == nullptr) {
            return nullptr;
        }
        UserPreferences prefs = UserPreferences::GetDefaultUserPreferences();
        FormatCodeSettings settings = GetDefaultFormatCodeSettings("\n");
        FormatContext fmt = GetFormatContext(settings);
        ::LanguageServiceHost host;
        auto *textChangesContext = new ::TextChangesContext {host, fmt, prefs};

        auto *refactorContext = new RefactorContext;
        refactorContext->context = ctx;
        refactorContext->textChangesContext = textChangesContext;
        refactorContext->span.pos = start;
        refactorContext->span.end = end;
        return refactorContext;
    }

    static const ApplicableRefactorInfo *FindAction(const std::vector<ApplicableRefactorInfo> &applicable,
                                                    const std::string &actionName)
    {
        for (const auto &info : applicable) {
            if (info.action.name == actionName) {
                return &info;
            }
        }
        return nullptr;
    }

    static bool HasAction(const std::vector<ApplicableRefactorInfo> &applicable, const std::string &actionName)
    {
        return FindAction(applicable, actionName) != nullptr;
    }

    static void ExpectAction(const std::vector<ApplicableRefactorInfo> &applicable, const std::string &actionName,
                             const std::string &description, const std::string &kind)
    {
        const ApplicableRefactorInfo *info = FindAction(applicable, actionName);
        ASSERT_NE(info, nullptr) << "missing action " << actionName;
        EXPECT_EQ(info->action.description, description);
        EXPECT_EQ(info->action.kind, kind);
    }

    static void ExpectContains(const std::string &text, const std::string &marker, const std::string &context)
    {
        EXPECT_NE(text.find(marker), std::string::npos) << context << "\nactual: " << text;
    }

    static void ExpectSemanticEdits(RefactorEditInfo *edits, const std::string &source,
                                    const std::vector<std::string> &markers, const std::string &renameAnchor,
                                    const std::string &renameToken)
    {
        ASSERT_NE(edits, nullptr);
        ASSERT_FALSE(edits->GetFileTextChanges().empty());
        const auto &changes = edits->GetFileTextChanges().front().textChanges;
        ASSERT_FALSE(changes.empty());

        const std::string applied = ApplyEdits(source, changes);
        for (const auto &marker : markers) {
            ExpectContains(applied, marker, "extract-symbol edit");
        }

        ASSERT_TRUE(edits->GetRenameLocation().has_value());
        const size_t renameLocation = edits->GetRenameLocation().value();
        const size_t anchorLocation = applied.find(renameAnchor);
        ASSERT_NE(anchorLocation, std::string::npos) << "missing rename anchor in:\n" << applied;
        const size_t tokenLocation = applied.find(renameToken, anchorLocation);
        ASSERT_NE(tokenLocation, std::string::npos) << "missing generated token in:\n" << applied;
        ASSERT_GE(renameLocation, tokenLocation);
        const std::string message = "rename " + std::to_string(renameLocation) + " outside token at " +
                                    std::to_string(tokenLocation) + " in:\n" + applied;
        ASSERT_LT(renameLocation, tokenLocation + renameToken.size()) << message;
    }
};

// =============================================================================
// PE-001 AddExtractVariableActions (extract_symbol_impl_edits.cpp:1268)
// Targets the untaken action-kind arms: enclose-available (L1273-1280),
// namespace+class named constant (L1275-1277), namespace depth loop (L1284-1290),
// class constant named (L1291-1294), disallowGlobalConstant (L1295-1297),
// and the semantic EXTRACT_* edit shapes for each action kind.
// =============================================================================

// PE-001: selection inside a class method with an enclosing function scope.
TEST_F(ExtractSymbolActionsMatrixTests, ActionsLocalClassSelectionMatrix)
{
    const std::string code = R"(
class C {
    v: int = 0;

    compute(): int {
        return 1 + 2 + this.v;
    }
}
)";
    const std::string target = "1 + 2";
    const size_t start = code.find(target);
    ASSERT_NE(start, std::string::npos);
    Initializer initializer;
    auto *refactorContext =
        MakeContext(&initializer, "ActionsMatrixLocalClass.ets", code, start, start + target.size());
    ASSERT_NE(refactorContext, nullptr);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());
    ExpectAction(applicable, "extract_variable_scope_0", "Extract to variable in enclosing scope",
                 "refactor.extract.variable");
    ExpectAction(applicable, "extract_constant_scope_0", "Extract to constant in enclosing scope",
                 "refactor.extract.constant");
    ExpectAction(applicable, "extract_constant_scope_1", "Extract to constant in class 'C'",
                 "refactor.extract.constant");
    ExpectAction(applicable, "extract_constant_scope_2", "Extract to constant in global scope",
                 "refactor.extract.constant");
    ExpectAction(applicable, "extract_function_scope_1", "Extract to function in class 'C'",
                 "refactor.extract.function");
    // With an enclosing scope the global variable action is not offered.
    EXPECT_FALSE(HasAction(applicable, "extract_variable_scope_2"));

    initializer.DestroyContext(refactorContext->context);
}

// PE-001: namespace scope + class scope -> named constant enclose description
// (impl_edits.cpp:1275-1277 BuildNamedScopeDescription arm).
TEST_F(ExtractSymbolActionsMatrixTests, ActionsNamespaceClassNamedConstant)
{
    const std::string code = R"(
namespace N {
    class C {
        compute(): int {
            return 3 * 4;
        }
    }
}
)";
    const std::string target = "3 * 4";
    const size_t start = code.find(target);
    ASSERT_NE(start, std::string::npos);
    Initializer initializer;
    auto *refactorContext =
        MakeContext(&initializer, "ActionsMatrixNamespaceClass.ets", code, start, start + target.size());
    ASSERT_NE(refactorContext, nullptr);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());
    ExpectAction(applicable, "extract_constant_scope_0", "Extract to constant in namespace 'N'",
                 "refactor.extract.constant");
    ExpectAction(applicable, "extract_constant_scope_1", "Extract to constant in class 'C'",
                 "refactor.extract.constant");
    ExpectAction(applicable, "extract_variable_scope_0", "Extract to variable in enclosing scope",
                 "refactor.extract.variable");
    ExpectAction(applicable, "extract_function_scope_0", "Extract to function in namespace 'N'",
                 "refactor.extract.function");

    initializer.DestroyContext(refactorContext->context);
}

// PE-001: nested namespaces drive the namespace depth loop adding ns_1 actions
// (impl_edits.cpp:1284-1290) with BuildNamespaceScopedActionName names.
TEST_F(ExtractSymbolActionsMatrixTests, ActionsNestedNamespaceDepthLoop)
{
    const std::string code = R"(
namespace Outer {
    namespace Inner {
        export function compute(): int {
            return 5 + 6;
        }
    }
}
)";
    const std::string target = "5 + 6";
    const size_t start = code.find(target);
    ASSERT_NE(start, std::string::npos);
    Initializer initializer;
    auto *refactorContext = MakeContext(&initializer, "ActionsMatrixNestedNs.ets", code, start, start + target.size());
    ASSERT_NE(refactorContext, nullptr);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());
    ExpectAction(applicable, "extract_constant_scope_ns_1", "Extract to constant in namespace 'Outer'",
                 "refactor.extract.constant");
    ExpectAction(applicable, "extract_constant_scope_0", "Extract to constant in enclosing scope",
                 "refactor.extract.constant");
    ExpectAction(applicable, "extract_function_scope_ns_1", "Extract to function in namespace 'Outer'",
                 "refactor.extract.function");
    // Only one enclosing namespace beyond the innermost one -> no ns_2 action.
    EXPECT_FALSE(HasAction(applicable, "extract_constant_scope_ns_2"));

    initializer.DestroyContext(refactorContext->context);
}

// PE-001: object literal selection without 'use static' -> global constant action
// suppressed (impl_edits.cpp:1295-1297 disallowGlobalConstant + L1447-1453 flags).
TEST_F(ExtractSymbolActionsMatrixTests, ActionsObjectLiteralDisallowsGlobalConstant)
{
    const std::string code = R"(
class C {
    make(): int {
        let p = {x: 1};
        return p.x;
    }
}
)";
    const std::string target = "{x: 1}";
    const size_t start = code.find(target);
    ASSERT_NE(start, std::string::npos);
    Initializer initializer;
    auto *refactorContext =
        MakeContext(&initializer, "ActionsMatrixObjLiteral.ets", code, start, start + target.size());
    ASSERT_NE(refactorContext, nullptr);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());
    EXPECT_FALSE(HasAction(applicable, "extract_constant_scope_2"));
    ExpectAction(applicable, "extract_constant_scope_0", "Extract to constant in enclosing scope",
                 "refactor.extract.constant");
    ExpectAction(applicable, "extract_constant_scope_1", "Extract to constant in class 'C'",
                 "refactor.extract.constant");

    initializer.DestroyContext(refactorContext->context);
}

// PE-001: let with an explicit type annotation, extracted with the variable
// enclose action kind; the generated declaration carries the annotation.
TEST_F(ExtractSymbolActionsMatrixTests, ActionsVariableKindExactEditsLetAnnotated)
{
    const std::string code = R"(
function f(): int {
    let base: int = 7 * 6;
    return base;
}
)";
    const std::string target = "7 * 6";
    const size_t start = code.find(target);
    ASSERT_NE(start, std::string::npos);
    Initializer initializer;
    auto *refactorContext =
        MakeContext(&initializer, "ActionsMatrixVarAnnotated.ets", code, start, start + target.size());
    ASSERT_NE(refactorContext, nullptr);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasAction(applicable, "extract_variable_scope_0"));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, "extract_variable_scope_0");
    ASSERT_NE(edits, nullptr);
    ExpectSemanticEdits(edits.get(), code, {"let newLocal: int = 7 * 6;", "let base: int = newLocal;", "return base;"},
                        "let base: int = ", "newLocal");

    initializer.DestroyContext(refactorContext->context);
}

// PE-001: const with an inferred type at global scope, extracted with the
// constant global action kind; insertion text uses const + checker-inferred Int.
TEST_F(ExtractSymbolActionsMatrixTests, ActionsVariableKindExactEditsConstGlobal)
{
    const std::string code = "const g = 42;\nfunction use(): int { return g; }\n";
    const std::string target = "42";
    const size_t start = code.find(target);
    ASSERT_NE(start, std::string::npos);
    Initializer initializer;
    auto *refactorContext = MakeContext(&initializer, "ActionsMatrixConstGlobal.ets", code, start, start + 2);
    ASSERT_NE(refactorContext, nullptr);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasAction(applicable, "extract_constant_scope_2"));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, "extract_constant_scope_2");
    ASSERT_NE(edits, nullptr);
    ExpectSemanticEdits(edits.get(), code,
                        {"const newLocal: Int = 42;", "const g = newLocal;", "function use(): int { return g; }"},
                        "const g = ", "newLocal");

    initializer.DestroyContext(refactorContext->context);
}

// =============================================================================
// PE-006 ComputeVariableUsageRenameLocFromEdits (extract_symbol_impl.cpp:1407)
// Targets the untaken arms: replace-change scan (L1414-1419), sort lambda
// (L1429-1430), shiftBeforeReplace loop (L1433-1440), renameOffset 1 (L1450)
// and the this. prefix renameOffset (L1451-1453).
// =============================================================================

// PE-006: enclose extraction inserts the declaration before the usage so the
// sorted loop and shiftBeforeReplace arms run; rename lands inside newLocal.
TEST_F(ExtractSymbolActionsMatrixTests, RenameLocVariableEncloseSingleUsage)
{
    const std::string code = R"(
function f(): int {
    let base: int = 7 * 6;
    return base;
}
)";
    const std::string target = "7 * 6";
    const size_t start = code.find(target);
    ASSERT_NE(start, std::string::npos);
    Initializer initializer;
    auto *refactorContext = MakeContext(&initializer, "RenameLocVarEnclose.ets", code, start, start + target.size());
    ASSERT_NE(refactorContext, nullptr);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, "extract_variable_scope_0");
    ASSERT_NE(edits, nullptr);
    ExpectSemanticEdits(edits.get(), code, {"let newLocal: int = 7 * 6;", "let base: int = newLocal;", "return base;"},
                        "let base: int = ", "newLocal");

    initializer.DestroyContext(refactorContext->context);
}

// PE-006: global extraction inserts the inferred-type declaration at the top of
// the file; the out-of-order insert/replace pair exercises the sort lambda and
// the shift accumulation (impl.cpp:1429-1440).
TEST_F(ExtractSymbolActionsMatrixTests, RenameLocVariableGlobalWithInsertShift)
{
    const std::string code = R"(
function f(): int {
    return 11 + 13;
}
)";
    const std::string target = "11 + 13";
    const size_t start = code.find(target);
    ASSERT_NE(start, std::string::npos);
    Initializer initializer;
    auto *refactorContext = MakeContext(&initializer, "RenameLocVarGlobal.ets", code, start, start + target.size());
    ASSERT_NE(refactorContext, nullptr);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, "extract_variable_scope_2");
    ASSERT_NE(edits, nullptr);
    ExpectSemanticEdits(edits.get(), code, {"let newLocal: Int = 11 + 13;", "return newLocal;"}, "return ", "newLocal");

    initializer.DestroyContext(refactorContext->context);
}

// PE-006: class-scope constant rewrites the usage to this.newProperty, driving
// the this. prefix renameOffset arm (impl.cpp:1451-1453).
TEST_F(ExtractSymbolActionsMatrixTests, RenameLocClassConstantThisOffset)
{
    const std::string code = R"(
class C {
    compute(): int {
        return 21 * 2;
    }
}
)";
    const std::string target = "21 * 2";
    const size_t start = code.find(target);
    ASSERT_NE(start, std::string::npos);
    Initializer initializer;
    auto *refactorContext = MakeContext(&initializer, "RenameLocConstClass.ets", code, start, start + target.size());
    ASSERT_NE(refactorContext, nullptr);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, "extract_constant_scope_1");
    ASSERT_NE(edits, nullptr);
    ExpectSemanticEdits(edits.get(), code, {"private readonly newProperty = 21 * 2;", "return this.newProperty;"},
                        "return this.newProperty", "newProperty");

    initializer.DestroyContext(refactorContext->context);
}

// =============================================================================
// PE-013 RebalanceVariableActionByScope (extract_symbol_impl_edits.cpp:1556)
// Targets the untaken arms: no-newline + no-enclose -> drop ENCLOSE keep/add
// GLOBAL (L1561-1566), newline/this-super early return (L1567-1570), enclose
// scope -> drop GLOBAL ensure ENCLOSE (L1571-1577).
// =============================================================================

// PE-013: single-line literal in a top-level initializer -> only the global
// variable/constant actions survive (L1561-1566 plus ApplyInitializerLiteralSelectionRules).
TEST_F(ExtractSymbolActionsMatrixTests, RebalanceTopLevelLiteralKeepsGlobalOnly)
{
    const std::string code = "const g = 42;\nfunction use(): int { return g; }\n";
    const std::string target = "42";
    const size_t start = code.find(target);
    ASSERT_NE(start, std::string::npos);
    Initializer initializer;
    auto *refactorContext = MakeContext(&initializer, "RebalanceTopLiteral.ets", code, start, start + 2);
    ASSERT_NE(refactorContext, nullptr);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());
    ExpectAction(applicable, "extract_variable_scope_2", "Extract to variable in global scope",
                 "refactor.extract.variable");
    EXPECT_FALSE(HasAction(applicable, "extract_variable_scope_0"));
    ExpectAction(applicable, "extract_constant_scope_2", "Extract to constant in global scope",
                 "refactor.extract.constant");
    EXPECT_FALSE(HasAction(applicable, "extract_constant_scope_0"));

    initializer.DestroyContext(refactorContext->context);
}

// PE-013: single-line expression inside a function -> enclose actions kept,
// global variable action dropped (L1571-1577).
TEST_F(ExtractSymbolActionsMatrixTests, RebalanceEncloseScopeKeepsEnclose)
{
    const std::string code = R"(
function f(a: int): int {
    return a * 3;
}
)";
    const std::string target = "a * 3";
    const size_t start = code.find(target);
    ASSERT_NE(start, std::string::npos);
    Initializer initializer;
    auto *refactorContext = MakeContext(&initializer, "RebalanceEnclose.ets", code, start, start + target.size());
    ASSERT_NE(refactorContext, nullptr);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());
    ExpectAction(applicable, "extract_variable_scope_0", "Extract to variable in enclosing scope",
                 "refactor.extract.variable");
    EXPECT_FALSE(HasAction(applicable, "extract_variable_scope_2"));
    ExpectAction(applicable, "extract_constant_scope_0", "Extract to constant in enclosing scope",
                 "refactor.extract.constant");

    initializer.DestroyContext(refactorContext->context);
}

// PE-013: multiline expression selection -> early return (L1567-1568) keeps the
// base action set unchanged (enclose present, global variable absent).
TEST_F(ExtractSymbolActionsMatrixTests, RebalanceMultilineSelectionEarlyReturn)
{
    const std::string code = R"(
function f(a: int): int {
    return a +
        100;
}
)";
    const std::string target = "a +\n        100";
    const size_t start = code.find(target);
    ASSERT_NE(start, std::string::npos);
    Initializer initializer;
    auto *refactorContext = MakeContext(&initializer, "RebalanceMultiline.ets", code, start, start + target.size());
    ASSERT_NE(refactorContext, nullptr);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());
    ExpectAction(applicable, "extract_variable_scope_0", "Extract to variable in enclosing scope",
                 "refactor.extract.variable");
    EXPECT_FALSE(HasAction(applicable, "extract_variable_scope_2"));
    ExpectAction(applicable, "extract_constant_scope_0", "Extract to constant in enclosing scope",
                 "refactor.extract.constant");

    initializer.DestroyContext(refactorContext->context);
}

// PE-013: this-containing selection -> containsThisOrSuper early return
// (L1567); variable enclose survives while the global constant action is
// suppressed via ResolveVariableRefactorRestrictions (impl_edits.cpp:750).
TEST_F(ExtractSymbolActionsMatrixTests, RebalanceThisSelectionConstantClassOnly)
{
    const std::string code = R"(
class C {
    v: int = 4;

    compute(): int {
        return this.v * 9;
    }
}
)";
    const std::string target = "this.v * 9";
    const size_t start = code.find(target);
    ASSERT_NE(start, std::string::npos);
    Initializer initializer;
    auto *refactorContext = MakeContext(&initializer, "RebalanceThis.ets", code, start, start + target.size());
    ASSERT_NE(refactorContext, nullptr);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_FALSE(applicable.empty());
    ExpectAction(applicable, "extract_variable_scope_0", "Extract to variable in enclosing scope",
                 "refactor.extract.variable");
    ExpectAction(applicable, "extract_constant_scope_1", "Extract to constant in class 'C'",
                 "refactor.extract.constant");
    EXPECT_FALSE(HasAction(applicable, "extract_constant_scope_2"));

    initializer.DestroyContext(refactorContext->context);
}

// =============================================================================
// PE-016 ResolveReturnTypeAnnotationForBinding (extract_symbol.cpp:3077)
// Targets the untaken arms: annotated identifier (L3082-3091, via
// InferHelperReturnTypeAnnotationFromBinding L3153 normalize=true), checker on
// identifier (L3093-3099), literal fallback number/string/boolean
// (L3106-3116 via InferReturnTypeFromTopLevelBinding impl.cpp:2126), empty
// fallthrough (L3117).
// =============================================================================

// PE-016: annotated binding extracted through the global helper action -> the
// declared annotation text is carried into the helper signature.
TEST_F(ExtractSymbolActionsMatrixTests, ReturnTypeAnnotatedGlobalHelper)
{
    const std::string code = R"(class C {
    m(): int {
        let cfg: int = 12;
        return cfg;
    }
}
)";
    const std::string target = "12";
    const size_t start = code.find(target);
    ASSERT_NE(start, std::string::npos);
    Initializer initializer;
    auto *refactorContext = MakeContext(&initializer, "RetTypeAnnotated.ets", code, start, start + 2);
    ASSERT_NE(refactorContext, nullptr);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasAction(applicable, "extract_function_scope_2"));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, "extract_function_scope_2");
    ASSERT_NE(edits, nullptr);
    ExpectSemanticEdits(edits.get(), code,
                        {"function newFunction(): int {", "return 12;", "let cfg: int = newFunction();", "return cfg;"},
                        "let cfg: int = newFunction", "newFunction");

    initializer.DestroyContext(refactorContext->context);
}

// PE-016: unannotated binding extracted through the global helper action -> the
// checker yields an inferred primitive return type.
TEST_F(ExtractSymbolActionsMatrixTests, ReturnTypeCheckerInferredGlobalHelper)
{
    const std::string code = R"(class C {
    m(): int {
        let cfg2 = 34;
        return cfg2;
    }
}
)";
    const std::string target = "34";
    const size_t start = code.find(target);
    ASSERT_NE(start, std::string::npos);
    Initializer initializer;
    auto *refactorContext = MakeContext(&initializer, "RetTypeChecker.ets", code, start, start + 2);
    ASSERT_NE(refactorContext, nullptr);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasAction(applicable, "extract_function_scope_2"));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, "extract_function_scope_2");
    ASSERT_NE(edits, nullptr);
    ExpectSemanticEdits(edits.get(), code,
                        {"function newFunction(): Int {", "return 34;", "let cfg2 = newFunction();", "return cfg2;"},
                        "let cfg2 = newFunction", "newFunction");

    initializer.DestroyContext(refactorContext->context);
}

// PE-016: literal fallback arms (extract_symbol.cpp:3106-3116) on the top-level
// declaration-leading route: number -> ": int", string -> ": String",
// boolean -> ": boolean".
TEST_F(ExtractSymbolActionsMatrixTests, ReturnTypeLiteralFallbackNumberStringBoolean)
{
    struct LiteralCase {
        const char *decl;
        const char *varName;
        const char *annotation;
    };
    const std::vector<LiteralCase> cases = {
        {"let v =\n    9;", "v", ": int"},
        {"let s =\n    \"abc\";", "s", ": String"},
        {"let b =\n    true;", "b", ": boolean"},
    };
    for (const auto &literalCase : cases) {
        std::string code = std::string(literalCase.decl) + "\nfunction use(): int { return 0; }\n";
        const size_t start = code.find(literalCase.decl);
        ASSERT_EQ(start, 0U);
        Initializer initializer;
        auto *refactorContext =
            MakeContext(&initializer, "RetTypeLiteral.ets", code, start, start + strlen(literalCase.decl));
        ASSERT_NE(refactorContext, nullptr);
        auto applicable = GetApplicableRefactorsImpl(refactorContext);
        ASSERT_TRUE(HasAction(applicable, "extract_function_scope_2"));

        const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
        auto edits =
            ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, "extract_function_scope_2");
        ASSERT_NE(edits, nullptr);
        const std::string call = std::string("let ") + literalCase.varName + " = newFunction();";
        ExpectSemanticEdits(edits.get(), code,
                            {std::string("function newFunction()") + literalCase.annotation + " {", call},
                            " = newFunction()", "newFunction");
        initializer.DestroyContext(refactorContext->context);
    }
}

// PE-016: char/null initializers fall through every fallback (extract_symbol.cpp
// :3106-3116 miss -> L3117 empty), and the downstream literal parser
// (extract_symbol_impl.cpp:2132) also yields no annotation.
TEST_F(ExtractSymbolActionsMatrixTests, ReturnTypeCharAndNullFallThroughEmpty)
{
    const std::vector<std::string> cases = {
        "let c =\n    'x';",
        "let z =\n    null;",
    };
    for (const auto &declaration : cases) {
        std::string code = declaration + "\nfunction use(): int { return 0; }\n";
        const size_t start = code.find(declaration);
        ASSERT_EQ(start, 0U);
        Initializer initializer;
        auto *refactorContext =
            MakeContext(&initializer, "RetTypeFallthrough.ets", code, start, start + declaration.size());
        ASSERT_NE(refactorContext, nullptr);
        auto applicable = GetApplicableRefactorsImpl(refactorContext);
        ASSERT_TRUE(HasAction(applicable, "extract_function_scope_2"));

        const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
        auto edits =
            ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, "extract_function_scope_2");
        ASSERT_NE(edits, nullptr);
        ExpectSemanticEdits(edits.get(), code, {"function newFunction() {", "newFunction();"}, " = newFunction()",
                            "newFunction");
        initializer.DestroyContext(refactorContext->context);
    }
}
}  // namespace
