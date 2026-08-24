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
#include <array>
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <memory>
#include <vector>
#include "lsp/include/refactors/extract_symbol.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/get_edits_for_refactor.h"
#include "lsp/include/types.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/user_preferences.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "lsp_api_test.h"
#include "public/public.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::RefactorContext;

std::string ApplyEdits(const std::string &original, const std::vector<::TextChange> &edits)
{
    if (edits.empty()) {
        return original;
    }

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

std::string StripWs(std::string s)
{
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
    return s;
}

class LspExtractSymbolBaseTests : public LSPAPITests {
public:
    RefactorContext *CreateExtractContext(Initializer *initializer, const std::string &code, size_t start, size_t end)
    {
        std::vector<std::string> files = {"ExtractSymbolRefactorTest3.ets"};
        std::vector<std::string> texts = {code};
        auto filePaths = CreateTempFile(files, texts);
        auto ctx = initializer->CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

        ark::es2panda::lsp::UserPreferences prefs = ark::es2panda::lsp::UserPreferences::GetDefaultUserPreferences();
        ark::es2panda::lsp::FormatCodeSettings settings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
        ark::es2panda::lsp::FormatContext fmt = ark::es2panda::lsp::GetFormatContext(settings);
        LanguageServiceHost host;
        auto *textChangesContext = new TextChangesContext {host, fmt, prefs};

        auto *refactorContext = new RefactorContext;
        refactorContext->context = ctx;
        refactorContext->textChangesContext = textChangesContext;
        refactorContext->span.pos = start;
        refactorContext->span.end = end;
        return refactorContext;
    }
};

bool HasApplicableAction(const std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> &applicable,
                         const std::string &actionName)
{
    return std::any_of(applicable.begin(), applicable.end(),
                       [&](const auto &info) { return info.action.name == actionName; });
}

void ExpectExtractionApplies(const std::string &source, RefactorContext *refactorContext,
                             const std::string &refactorName, const std::string &actionName,
                             const std::string &expected)
{
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().front();
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(source, fileEdit.textChanges);
    EXPECT_EQ(StripWs(result), StripWs(expected));
}

struct ActionExpectation {
    const char *name;
    const char *code;
    const char *target;
    bool variableEnclose;
    bool variableGlobal;
    bool constantEnclose;
    bool constantGlobal;
};

// Asserts the exact extract variable/constant action set offered for a
// selection. This documents the current GetAvailableActions behavior for
// selections that are not complete expressions/statements: the selection is
// expanded to the enclosing initializer or statement node, so some actions
// may still be offered.
void ExpectValueActionSet(RefactorContext *refactorContext, const ActionExpectation &expectation)
{
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string variableEnclose = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const std::string variableGlobal = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name);
    const std::string constantEnclose = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const std::string constantGlobal = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_EQ(HasApplicableAction(applicable, variableEnclose), expectation.variableEnclose) << expectation.name;
    EXPECT_EQ(HasApplicableAction(applicable, variableGlobal), expectation.variableGlobal) << expectation.name;
    EXPECT_EQ(HasApplicableAction(applicable, constantEnclose), expectation.constantEnclose) << expectation.name;
    EXPECT_EQ(HasApplicableAction(applicable, constantGlobal), expectation.constantGlobal) << expectation.name;
}

// -- Fixture 1: selection must form complete expressions/statements ----------

class LspExtractSymbolBoundaryTests : public LspExtractSymbolBaseTests {};

// NOLINTNEXTLINE(readability-identifier-naming)
const char *BOUNDARY_PARTIAL_BINARY_CODE = R"(
let result = 1 + 2 * 3;
)";

// NOLINTNEXTLINE(readability-identifier-naming)
const char *BOUNDARY_DECLARATION_CODE = R"(
let count = 41 + 1;
)";

// NOLINTNEXTLINE(readability-identifier-naming)
const char *BOUNDARY_RETURN_PREFIX_CODE = R"(
function compute(base: number): number {
    return base + 10;
}
)";

// NOLINTNEXTLINE(readability-identifier-naming)
const char *BOUNDARY_INCOMPLETE_CODE = R"(
function demo(value: number): number {
    let doubled = value * ;
    return doubled;
}
)";

// Selections that are not complete expressions: partial binary operands,
// initializers with a trailing semicolon, the return keyword prefix and an
// incomplete (broken) binary expression. The columns record the exact
// variable/constant action availability of the current implementation.
const std::array<ActionExpectation, 4> BOUNDARY_SELECTION_CASES = {{
    // name, code, target, varEnclose, varGlobal, constEnclose, constGlobal
    {"PartialBinaryOperand", BOUNDARY_PARTIAL_BINARY_CODE, "1 + 2", false, true, false, true},
    {"InitializerWithTrailingSemicolon", BOUNDARY_DECLARATION_CODE, "41 + 1;", false, false, false, false},
    {"ReturnKeywordPrefix", BOUNDARY_RETURN_PREFIX_CODE, "return base", true, false, true, false},
    {"IncompleteBinaryOperand", BOUNDARY_INCOMPLETE_CODE, "value *", true, false, true, false},
}};

TEST_F(LspExtractSymbolBoundaryTests, NonCompleteExpressionSelectionsActionSet)
{
    for (const auto &testCase : BOUNDARY_SELECTION_CASES) {
        const std::string code = testCase.code;
        const std::string target = testCase.target;
        const size_t spanStart = code.find(target);
        ASSERT_NE(spanStart, std::string::npos) << testCase.name;
        const size_t spanEnd = spanStart + target.size();

        auto initializer = std::make_unique<Initializer>();
        auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

        ExpectValueActionSet(refactorContext, testCase);

        initializer->DestroyContext(refactorContext->context);
    }
}

TEST_F(LspExtractSymbolBoundaryTests, VariableDeclarationStatementIsNotAnExpression)
{
    const std::string code = BOUNDARY_DECLARATION_CODE;
    const std::string target = "let count = 41 + 1;";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string variableAction = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const std::string constantAction = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string functionAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_FALSE(HasApplicableAction(applicable, variableAction));
    EXPECT_FALSE(HasApplicableAction(applicable, constantAction));
    EXPECT_TRUE(HasApplicableAction(applicable, functionAction));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolBoundaryTests, WholeInitializerExtractsVariable)
{
    const std::string code = BOUNDARY_DECLARATION_CODE;
    const std::string expected = R"(
let newLocal: Int = 41 + 1;
let count = newLocal;
)";
    const std::string target = "41 + 1";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string variableAction = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, variableAction));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    ExpectExtractionApplies(code, refactorContext, refactorName, variableAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolBoundaryTests, WholeReturnStatementExtractsFunction)
{
    const std::string code = BOUNDARY_RETURN_PREFIX_CODE;
    const std::string expected = R"(
function newFunction(base: number): number {
    return base + 10;
}
function compute(base: number): number {
    return newFunction(base);
}
)";
    const std::string target = "return base + 10;";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, globalAction));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    ExpectExtractionApplies(code, refactorContext, refactorName, globalAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

// -- Fixture 2: capture of outer variables and writes ------------------------

class LspExtractSymbolCaptureTests : public LspExtractSymbolBaseTests {};

// NOLINTNEXTLINE(readability-identifier-naming)
const char *CAPTURE_MULTIPLE_READS_CODE = R"(
function compute(base: number, factor: number): number {
    let unused = 0;
    return (base + factor) * 2;
}
)";

// NOLINTNEXTLINE(readability-identifier-naming)
const char *CAPTURE_WRITE_AND_READ_CODE = R"(
function accumulate(seed: number): number {
    let total = seed;
    total = total + 1;
    return total * 2;
}
)";

TEST_F(LspExtractSymbolCaptureTests, FreeVariablesBecomeParametersInDeclarationOrder)
{
    const std::string code = CAPTURE_MULTIPLE_READS_CODE;
    const std::string expected = R"(
function newFunction(base: number, factor: number): number {
    return (base + factor) * 2;
}
function compute(base: number, factor: number): number {
    let unused = 0;
    return newFunction(base, factor);
}
)";
    const std::string target = "return (base + factor) * 2;";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, globalAction));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    ExpectExtractionApplies(code, refactorContext, refactorName, globalAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolCaptureTests, WrittenVariableAssignmentBecomesReturnedExpression)
{
    const std::string code = CAPTURE_WRITE_AND_READ_CODE;
    const std::string expected = R"(
function newFunction(total: number): number {
    return total + 1;
}
function accumulate(seed: number): number {
    let total = seed;
    total = newFunction(total);
    return total * 2;
}
)";
    const std::string target = "total = total + 1;";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, globalAction));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    ExpectExtractionApplies(code, refactorContext, refactorName, globalAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

// -- Fixture 3: control-flow boundaries (await / return / break / continue) --

class LspExtractSymbolControlFlowTests : public LspExtractSymbolBaseTests {};

// NOLINTNEXTLINE(readability-identifier-naming)
const char *ASYNC_SELECTION_CODE = R"(
async function fetchDouble(raw: number): Promise<number> {
    let prepared = await Promise.resolve(raw);
    return prepared * 2;
}
)";

// NOLINTNEXTLINE(readability-identifier-naming)
const char *LOOP_BREAK_CONTINUE_CODE = R"(
function collect(limit: number): number {
    let total = 0;
    for (let i = 0; i < limit; i++) {
        if (i == 3) {
            continue;
        }
        if (i > 4) {
            break;
        }
        total += i;
    }
    return total;
}
)";

TEST_F(LspExtractSymbolControlFlowTests, AwaitInsideSelectionIsMovedIntoExtractedFunction)
{
    const std::string code = ASYNC_SELECTION_CODE;
    const std::string target = R"(let prepared = await Promise.resolve(raw);
    return prepared * 2;)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, globalAction));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, globalAction);
    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().front();
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    // The extracted function keeps the await expression inside its own body.
    EXPECT_NE(result.find("function newFunction"), std::string::npos);
    EXPECT_NE(result.find("await Promise.resolve(raw)"), std::string::npos);
    EXPECT_NE(result.find("return prepared;"), std::string::npos);
    EXPECT_NE(result.find("let prepared = newFunction();"), std::string::npos);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolControlFlowTests, ReturnStatementWithoutValueExtractsFunction)
{
    const std::string code = R"(
function logValue(value: number): void {
    console.log(value);
    return;
}
)";
    const std::string expected = R"(
function logValue(value: number): void {
    return newFunction(value);
}

function newFunction(value: number): void {
    console.log(value);
    return;
}
)";
    const std::string target = R"(console.log(value);
    return;)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, globalAction));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    ExpectExtractionApplies(code, refactorContext, refactorName, globalAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolControlFlowTests, BreakAndContinueInsideFullySelectedLoopAreAllowed)
{
    const std::string code = LOOP_BREAK_CONTINUE_CODE;
    const std::string target = R"(for (let i = 0; i < limit; i++) {
        if (i == 3) {
            continue;
        }
        if (i > 4) {
            break;
        }
        total += i;
    })";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, globalAction));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, globalAction);
    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().front();
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_NE(result.find("newFunction(limit"), std::string::npos);
    EXPECT_NE(result.find("total = newFunction"), std::string::npos);
    EXPECT_NE(result.find("return total;"), std::string::npos);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolControlFlowTests, BreakContinueStatementsWithoutEnclosingLoopActionSet)
{
    const std::array<ActionExpectation, 2> cases = {{
        // name, code, target, varEnclose, varGlobal, constEnclose, constGlobal
        {"BreakAlone", LOOP_BREAK_CONTINUE_CODE, "break;", true, false, false, false},
        {"ContinueAlone", LOOP_BREAK_CONTINUE_CODE, "continue;", true, false, false, false},
    }};
    for (const auto &testCase : cases) {
        const std::string code = testCase.code;
        const std::string target = testCase.target;
        const size_t spanStart = code.find(target);
        ASSERT_NE(spanStart, std::string::npos) << testCase.name;
        const size_t spanEnd = spanStart + target.size();

        auto initializer = std::make_unique<Initializer>();
        auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

        ExpectValueActionSet(refactorContext, testCase);

        initializer->DestroyContext(refactorContext->context);
    }
}

TEST_F(LspExtractSymbolControlFlowTests, LoopBodyWithoutBreakContinueStatementsIsExtractable)
{
    const std::string code = LOOP_BREAK_CONTINUE_CODE;
    const std::string target = "total += i;";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, globalAction));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, globalAction);
    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().front();
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_NE(result.find("function newFunction"), std::string::npos);
    EXPECT_NE(result.find("newFunction(total, i)"), std::string::npos);

    initializer->DestroyContext(refactorContext->context);
}

// -- Fixture 4: this / super, generic context, automatic naming --------------

class LspExtractSymbolScopeTests : public LspExtractSymbolBaseTests {};

TEST_F(LspExtractSymbolScopeTests, ThisUsageStaysInClassMethodWhenExtractedToClass)
{
    const std::string code = R"(
class Counter {
    value: number = 0;
    increase(step: number): void {
        this.value = this.value + step;
    }
}
)";
    const std::string expected = R"(
class Counter {
    value: number = 0;
    private newMethod(step: number) {
        this.value = this.value + step;
    }
    increase(step: number): void {
        this.newMethod(step)
    }
}
)";
    const std::string target = "this.value = this.value + step;";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string classAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);
    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, classAction));
    EXPECT_FALSE(HasApplicableAction(applicable, globalAction));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    ExpectExtractionApplies(code, refactorContext, refactorName, classAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolScopeTests, ThisExpressionCannotBeExtractedToGlobalScope)
{
    const std::string code = R"(
class Holder {
    inner: number = 1;
    read(): number {
        return this.inner;
    }
}
)";
    const std::string target = "this.inner";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string classAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);
    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, classAction));
    EXPECT_FALSE(HasApplicableAction(applicable, globalAction));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolScopeTests, SuperMethodCallIsExtractableWithinSubclass)
{
    const std::string code = R"(
class BaseGreeter {
    greet(): string {
        return "base";
    }
}

class SubGreeter extends BaseGreeter {
    describe(): string {
        return "say:" + super.greet();
    }
}
)";
    const std::string expected = R"(
class BaseGreeter {
    greet(): string {
        return "base";
    }
}

class SubGreeter extends BaseGreeter {
    private newMethod(): string {
        return "say:" + super.greet();
    }
    describe(): string {
        return this.newMethod();
    }
}
)";
    const std::string target = R"(return "say:" + super.greet();)";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string classAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);
    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, classAction));
    EXPECT_FALSE(HasApplicableAction(applicable, globalAction));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    ExpectExtractionApplies(code, refactorContext, refactorName, classAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolScopeTests, GenericTypeParameterBecomesArgumentAndReturn)
{
    const std::string code = R"(
function wrap<T>(input: T): T[] {
    let pair: T[] = [input];
    return pair;
}
)";
    const std::string target = "let pair: T[] = [input];";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, globalAction));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, globalAction);
    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().front();
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_NE(result.find("newFunction<T>(input)"), std::string::npos);
    EXPECT_NE(result.find("function newFunction<T>(input: T): T[]"), std::string::npos);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolScopeTests, NameConflictWithExistingFunctionGeneratesSuffix)
{
    const std::string code = R"(
function newFunction(): number {
    return 0;
}

function compute(a: number, b: number): number {
    let sum = a + b;
    return sum;
}
)";
    const std::string expected = R"(
function newFunction(): number {
    return 0;
}

function newFunction_1(a: number, b: number): number {
    return a + b;
}

function compute(a: number, b: number): number {
    let sum = newFunction_1(a, b);
    return sum;
}
)";
    const std::string target = "let sum = a + b;";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, globalAction));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    ExpectExtractionApplies(code, refactorContext, refactorName, globalAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

// -- Fixture 5: return-type fallback chain, declaration-leading helpers ------

class LspExtractSymbolReturnTypeFallbackTests : public LspExtractSymbolBaseTests {};

// Exercises the InferReturnTypeAnnotationFromSelectionFallback chain
// (extract_symbol_impl.cpp) when a return statement inside a finally block has
// a non-literal argument: the consumer/checker paths return empty, so the
// fallback inference (declared binding, selected node checker, literal, and
// selection-text heuristics) is used. The statement-level checker then fills
// the ": Int" annotation after the fallback chain has run.
TEST_F(LspExtractSymbolReturnTypeFallbackTests, NonLiteralReturnInsideFinallyFallsBackToSelectionText)
{
    const std::string code = R"(
function first(g: number): number {
    return g;
}
let g = 5;
function test(): number {
    try {}
    finally {
        return g + 1;
    }
}
)";
    const std::string expected = R"(
function first(g: number): number {
    return g;
}
let g = 5;
function newFunction(): number {
    return g + 1;
}

function test(): number {
    try {}
    finally {
        return newFunction();
    }
}
)";
    const std::string target = "return g + 1;";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, globalAction));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    ExpectExtractionApplies(code, refactorContext, refactorName, globalAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolReturnTypeFallbackTests, FunctionLocalDeclarationLeadingExtraction)
{
    const std::string code = "function outer(): void {\nlet a = 1; let b = a + 1;\n}\n";
    const std::string expected = R"(
function newFunction(): number {
    let a = 1; let b = a + 1;
    return a;
}

function outer(): void {
let a = newFunction();
}
)";
    const std::string target = "let a = 1; let b = a + 1;";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    ExpectExtractionApplies(code, refactorContext, refactorName, globalAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

// Covers CollectIdentifierNames (extract_symbol.cpp): a global helper
// extraction where the initializer references a module-level (global-scope)
// variable from inside a namespace. AnalyzeFunctionIO skips the global
// identifier (not local, not nonGlobal), so callArgs stays empty and
// CollectIdentifierNames collects the free variable into the helper parameter
// list. The namespace enclosing class is not global, so BuildGlobalPieces is
// reachable and BuildParamSignature runs its free-variable fallback.
TEST_F(LspExtractSymbolReturnTypeFallbackTests, GlobalReferencedInitializerCollectsIdentifierNames)
{
    const std::string code = R"(
let g = 5;
namespace N {
    function outer(): void {
        let b = g + 1;
    }
}
)";
    const std::string expected = R"(
function newFunction(g): number {
    return g + 1;
}
let g = 5;
namespace N {
    function outer(): void {

        let b = newFunction(g);
    }
}
)";
    const std::string target = "let b = g + 1;";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    ExpectExtractionApplies(code, refactorContext, refactorName, globalAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

// -- Fixture 6: 'use static' object-literal initializer extraction ------------
// Targets extract_symbol_impl_edits.cpp ApplyVariableRefactorRestrictions and
// extract_symbol.cpp GetDeclaratorIdText / GetDeclaratorTextWithoutInitializer.
// All scenarios go through the real GetApplicableRefactorsImpl /
// GetEditsForRefactorsImpl public entry points.

class LspExtractSymbolUseStaticTests : public LspExtractSymbolBaseTests {};

TEST_F(LspExtractSymbolUseStaticTests, UseStaticObjectLiteralWithLocalDependencyReaddsGlobalConstant)
{
    const std::string code = R"('use static'

function build(): void {
    let base = 1;
    let cfg = { level: base };
}
)";
    const std::string target = "{ level: base }";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string constantEnclose = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    const std::string variableEnclose = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const std::string constantGlobal = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, constantEnclose));
    EXPECT_TRUE(HasApplicableAction(applicable, variableEnclose));
    EXPECT_TRUE(HasApplicableAction(applicable, constantGlobal))
        << "use-static object literal with local dependency must re-add the global constant action";

    // The re-added action must be usable through the edits entry point.
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, constantGlobal);
    ASSERT_NE(edits, nullptr);

    initializer->DestroyContext(refactorContext->context);
}

// Covers the opposite side of the any_of branch in
// ApplyVariableRefactorRestrictions: a top-level 'use static' object literal
// has no local-value dependency, so the global constant is added by the regular
// AddExtractVariableActions pass and the any_of rescue branch observes it
// already present (no duplicate action).
TEST_F(LspExtractSymbolUseStaticTests, UseStaticObjectLiteralWithoutLocalDependencyKeepsGlobalConstant)
{
    const std::string code = R"('use static'

let cfg = { level: 1 };
)";
    const std::string target = "{ level: 1 }";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(applicable.empty());

    const std::string constantGlobal = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string variableGlobal = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, constantGlobal));
    EXPECT_TRUE(HasApplicableAction(applicable, variableGlobal));

    initializer->DestroyContext(refactorContext->context);
}

// Attempt for GetDeclaratorIdText (extract_symbol.cpp): a whole single-line
// declaration at module top level is extracted to a global function through the
// real GetEditsForRefactorsImpl entry point. BuildGlobalPieces /
// BuildAssignmentLine run with a real source declarator whose source text is
// non-empty, so GetDeclaratorTextWithoutInitializer returns the declarator text
// directly and the GetDeclaratorIdText fallback (only reachable when the
// declarator source text is empty, i.e. for zero-length / missing nodes) is not
// taken. This documents the reachable behavior and the unreachable fallback.
TEST_F(LspExtractSymbolUseStaticTests, WholeDeclarationFunctionExtractionUsesDeclaratorText)
{
    const std::string code = R"('use static'

interface Point {
    x: number;
    y: number;
}

let origin: Point = { x: 0, y: 0 };
)";
    const std::string target = "let origin: Point = { x: 0, y: 0 };";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string globalAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    EXPECT_TRUE(HasApplicableAction(applicable, globalAction));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, globalAction);
    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().front();
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string result = ApplyEdits(code, fileEdit.textChanges);
    // The declarator text `origin: Point` (not just the bare identifier) must be
    // preserved in the generated assignment line. The helper body is built from
    // the initializer because the whole-declaration selection is normalized to
    // the initializer expression.
    EXPECT_NE(result.find("let origin: Point = newFunction();"), std::string::npos) << "result:\n" << result;
    EXPECT_NE(result.find("return { x: 0, y: 0 };"), std::string::npos) << "result:\n" << result;

    initializer->DestroyContext(refactorContext->context);
}

// A class value referenced from inside a namespace is not extracted: moving it
// would require cross-namespace value qualification and visibility checks.
TEST_F(LspExtractSymbolReturnTypeFallbackTests, NamespaceClassValueParamIsNotExtractable)
{
    const std::string code = R"(
namespace N {
    class Foo {}
    function make(): void {
        let c = Foo;
    }
}
)";
    const std::string declaration = "let c = ";
    const size_t declarationStart = code.find(declaration);
    ASSERT_NE(declarationStart, std::string::npos);
    const std::string target = "Foo";
    const size_t spanStart = code.find(target, declarationStart + declaration.size());
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(HasApplicableAction(applicable, std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name)));
    EXPECT_FALSE(
        HasApplicableAction(applicable, std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name)));
    EXPECT_FALSE(HasApplicableAction(applicable, std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name)));
    EXPECT_FALSE(
        HasApplicableAction(applicable, std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name)));
    EXPECT_FALSE(HasApplicableAction(applicable, std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name)));
    EXPECT_FALSE(
        HasApplicableAction(applicable, std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name)));

    initializer->DestroyContext(refactorContext->context);
}

}  // namespace
