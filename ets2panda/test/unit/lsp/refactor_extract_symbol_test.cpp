/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
#include <string_view>
#include <algorithm>
#include <cctype>
#include <memory>
#include <utility>
#include "lsp/include/refactors/extract_symbol.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/get_edits_for_refactor.h"
#include "lsp/include/types.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/user_preferences.h"
#include "public/es2panda_lib.h"
#include "lsp_api_test.h"
#include "public/public.h"

namespace {
using ark::es2panda::lsp::Initializer;
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
    std::stable_sort(ordered.begin(), ordered.end(), [](const ::TextChange *lhs, const ::TextChange *rhs) {
        return lhs->span.start < rhs->span.start;
    });

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

enum class StripScanState { CODE, SINGLE_QUOTE, DOUBLE_QUOTE, TEMPLATE, LINE_COMMENT, BLOCK_COMMENT };

void EnterStringState(StripScanState nextState, StripScanState &state, bool &escaped, std::string &normalized, char ch)
{
    state = nextState;
    escaped = false;
    normalized.push_back(ch);
}

void HandleCodeState(StripScanState &state, bool &escaped, std::string &normalized, char ch, char next)
{
    if (ch == '/' && next == '/') {
        state = StripScanState::LINE_COMMENT;
        return;
    }
    if (ch == '/' && next == '*') {
        state = StripScanState::BLOCK_COMMENT;
        return;
    }
    if (ch == '\'') {
        EnterStringState(StripScanState::SINGLE_QUOTE, state, escaped, normalized, ch);
        return;
    }
    if (ch == '"') {
        EnterStringState(StripScanState::DOUBLE_QUOTE, state, escaped, normalized, ch);
        return;
    }
    if (ch == '`') {
        EnterStringState(StripScanState::TEMPLATE, state, escaped, normalized, ch);
        return;
    }
    if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
        return;
    }
    normalized.push_back(ch);
}

void HandleStringState(StripScanState &state, bool &escaped, std::string &normalized, char ch, char endChar)
{
    normalized.push_back(ch);
    if (!escaped && ch == endChar) {
        state = StripScanState::CODE;
    }
    escaped = (!escaped && ch == '\\');
}

void HandleLineCommentState(StripScanState &state, char ch)
{
    if (ch == '\n') {
        state = StripScanState::CODE;
    }
}

void HandleBlockCommentState(StripScanState &state, bool &skipNext, char ch, char next)
{
    if (ch == '*' && next == '/') {
        state = StripScanState::CODE;
        skipNext = true;
    }
}

std::string StripFormattingWhitespace(const std::string &source)
{
    std::string normalized;
    normalized.reserve(source.size());

    StripScanState state = StripScanState::CODE;
    bool escaped = false;
    bool skipNext = false;
    for (size_t i = 0; i < source.size(); i++) {
        if (skipNext) {
            skipNext = false;
            continue;
        }
        const char ch = source[i];
        const char next = (i + 1 < source.size()) ? source[i + 1] : '\0';

        switch (state) {
            case StripScanState::CODE:
                HandleCodeState(state, escaped, normalized, ch, next);
                break;
            case StripScanState::SINGLE_QUOTE:
                HandleStringState(state, escaped, normalized, ch, '\'');
                break;
            case StripScanState::DOUBLE_QUOTE:
                HandleStringState(state, escaped, normalized, ch, '"');
                break;
            case StripScanState::TEMPLATE:
                HandleStringState(state, escaped, normalized, ch, '`');
                break;
            case StripScanState::LINE_COMMENT:
                HandleLineCommentState(state, ch);
                break;
            case StripScanState::BLOCK_COMMENT:
                HandleBlockCommentState(state, skipNext, ch, next);
                break;
            default:
                break;
        }
    }
    return normalized;
}

void ExpectRenameLocExact(const std::string &expected, const ark::es2panda::lsp::RefactorEditInfo &edits,
                          std::string_view token)
{
    ASSERT_TRUE(edits.GetRenameLocation().has_value());
    if (token.rfind("this.", 0) == 0) {
        const size_t base = expected.find(token);
        ASSERT_NE(base, std::string::npos);
        const size_t expectedLoc = base + std::string("this.").size() + 1;
        EXPECT_EQ(edits.GetRenameLocation().value(), expectedLoc);
        return;
    }
    const size_t first = expected.find(token);
    ASSERT_NE(first, std::string::npos);
    const size_t second = expected.find(token, first + 1);
    const size_t targetPos = (second == std::string::npos) ? first : second;
    const size_t expectedLoc = targetPos + (token.size() > 1 ? 1 : 0);
    EXPECT_EQ(edits.GetRenameLocation().value(), expectedLoc);
}

void ExpectVariableRenameLocOnUsage(const std::string &source, const FileTextChanges &fileEdit,
                                    const ark::es2panda::lsp::RefactorEditInfo &edits, std::string_view token)
{
    const std::string actual = ApplyEdits(source, fileEdit.textChanges);
    const size_t first = actual.find(token);
    ASSERT_NE(first, std::string::npos) << actual;
    const size_t second = actual.find(token, first + 1);
    ASSERT_NE(second, std::string::npos) << actual;
    ASSERT_TRUE(edits.GetRenameLocation().has_value()) << actual;
    const size_t renameLoc = edits.GetRenameLocation().value();
    EXPECT_GE(renameLoc, second) << actual;
    EXPECT_LT(renameLoc, second + token.size()) << actual;
}

class LspExtrSymblGetEditsTests : public LSPAPITests {
public:
    ark::es2panda::lsp::RefactorContext *CreateExtractContext(Initializer *initializer, const std::string &code,
                                                              size_t start, size_t end)
    {
        std::vector<std::string> files = {"ExtractSymbolRefactorTest.ets"};
        std::vector<std::string> texts = {code};
        auto filePaths = CreateTempFile(files, texts);
        auto ctx = initializer->CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

        ark::es2panda::lsp::UserPreferences prefs = ark::es2panda::lsp::UserPreferences::GetDefaultUserPreferences();
        ark::es2panda::lsp::FormatCodeSettings settings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
        ark::es2panda::lsp::FormatContext fmt = ark::es2panda::lsp::GetFormatContext(settings);
        LanguageServiceHost host;
        auto *textChangesContext = new TextChangesContext {host, fmt, prefs};

        auto *refactorContext = new ark::es2panda::lsp::RefactorContext;
        refactorContext->context = ctx;
        refactorContext->textChangesContext = textChangesContext;

        refactorContext->span.pos = start;
        refactorContext->span.end = end;
        return refactorContext;
    }
};

void ExpectExtractionApplies(const std::string &source, ark::es2panda::lsp::RefactorContext *refactorContext,
                             const std::string &refactorName, const std::string &actionName,
                             const std::string &expected)
{
    LSPAPI const *lspApi = GetImpl();

    auto edits = lspApi->getEditsForRefactor(*refactorContext, refactorName, actionName);

    ASSERT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().front();
    ASSERT_FALSE(fileEdit.textChanges.empty());

    const std::string actual = ApplyEdits(source, fileEdit.textChanges);
    EXPECT_EQ(StripFormattingWhitespace(actual), StripFormattingWhitespace(expected));
}

std::unique_ptr<ark::es2panda::lsp::RefactorEditInfo> GetEditsViaLspApi(
    ark::es2panda::lsp::RefactorContext *refactorContext, const std::string &refactorName,
    const std::string &actionName)
{
    LSPAPI const *lspApi = GetImpl();
    auto edits = lspApi->getEditsForRefactor(*refactorContext, refactorName, actionName);
    EXPECT_NE(edits, nullptr);
    if (edits == nullptr) {
        return nullptr;
    }
    EXPECT_EQ(edits->GetFileTextChanges().size(), 1U);
    if (!edits->GetFileTextChanges().empty()) {
        const auto &fileEdit = edits->GetFileTextChanges().front();
        EXPECT_FALSE(fileEdit.textChanges.empty());
    }
    return edits;
}

constexpr std::string_view K_EXTRACT_METHOD_GLOBAL_FOR_RENAME_CODE = R"('use static'

function newFunction(a: number, b: number) {
    let c = a + b;
    return c;
}

class MyClass {

    MyMethod(a: number, b: number) {
        let c = a + b;
        let d = c * c;
        return d;
    }
}
)";

constexpr std::string_view K_NESTED_NAMESPACE_EXTRACT_METHOD_CODE = R"(
namespace A {
  let x = 1;
  function foo() {
  }
  namespace B {
    function a() {
      let tmp = 1;
      let y = 3;
      let z = x;
      tmp = y;
      foo();
    }
  }
}
)";

constexpr std::string_view K_NESTED_NAMESPACE_EXTRACT_METHOD_WITH_COMMENTS_CODE = R"(
namespace A {
  let x = 1;
  function foo() {
  }
  namespace B {
    function a() {
      let tmp = 1;
      let y = 3; // keep-line-comment
      /* keep-block-comment */
      let z = x;
      tmp = y;
      foo();
    }
  }
}
)";

constexpr std::string_view K_NESTED_NAMESPACE_EXTRACT_EXPR_TO_OUTER_CODE = R"(
namespace A {
  namespace B {
    function a() {
      let y = 3;
      let z = y + 1;
    }
  }
}
)";

constexpr std::string_view K_NESTED_NAMESPACE_EXTRACT_EXPR_TO_OUTER_EXPECTED = R"(
namespace A {
  function newFunction(y: Int) {
    return y + 1;
  }

  namespace B {
    function a() {
      let y = 3;
      let z = newFunction(y);
    }
  }
}
)";

constexpr std::string_view K_EXPORTED_NESTED_NAMESPACE_EXTRACT_EXPR_GLOBAL_CODE = R"(
namespace B {
  export namespace A {
    function a() {
      let y = 3;
      let z = y + 1;
    }
  }
}
)";

const ark::es2panda::lsp::ApplicableRefactorInfo *FindApplicableAction(
    const std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> &applicable, std::string_view actionName)
{
    auto iter = std::find_if(applicable.begin(), applicable.end(),
                             [actionName](const auto &info) { return info.action.name == actionName; });
    if (iter == applicable.end()) {
        return nullptr;
    }
    return &(*iter);
}

bool HasExecutableRefactorAction(const std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> &applicable)
{
    return std::any_of(applicable.begin(), applicable.end(),
                       [](const auto &info) { return !info.action.name.empty(); });
}

bool HasApplicableAction(const std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> &applicable,
                         std::string_view actionName)
{
    return FindApplicableAction(applicable, actionName) != nullptr;
}

std::string FormatApplicableActions(const std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> &applicable)
{
    std::string formatted;
    for (const auto &item : applicable) {
        if (!formatted.empty()) {
            formatted.append(" | ");
        }
        formatted.append(item.action.name);
        formatted.append(" => ");
        formatted.append(item.action.description);
    }
    return formatted;
}

void ExpectNestedNamespaceActionDescriptions(const std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> &applicable)
{
    const auto *constInNamespace =
        FindApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    ASSERT_NE(constInNamespace, nullptr);
    EXPECT_EQ(constInNamespace->action.description, "Extract to constant in enclosing scope");

    const auto *constInOuterNamespace = FindApplicableAction(applicable, "extract_constant_scope_ns_1");
    ASSERT_NE(constInOuterNamespace, nullptr);
    EXPECT_EQ(constInOuterNamespace->action.description, "Extract to constant in namespace 'A'");

    const auto *funcInNamespace =
        FindApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    ASSERT_NE(funcInNamespace, nullptr);
    EXPECT_EQ(funcInNamespace->action.description, "Extract to function in namespace 'B'");

    const auto *funcInOuterNamespace = FindApplicableAction(applicable, "extract_function_scope_ns_1");
    ASSERT_NE(funcInOuterNamespace, nullptr);
    EXPECT_EQ(funcInOuterNamespace->action.description, "Extract to function in namespace 'A'");

    const auto *funcGlobal = FindApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    ASSERT_NE(funcGlobal, nullptr);
}

std::unique_ptr<ark::es2panda::lsp::RefactorEditInfo> ExpectExtractResultIgnoringWhitespace(
    ark::es2panda::lsp::RefactorContext *refactorContext, const std::string &code, const std::string &expected,
    const std::string &refactorName, const std::string &actionName)
{
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    if (!edits) {
        ADD_FAILURE() << "GetEditsForRefactorsImpl returned null edits";
        return nullptr;
    }
    if (edits->GetFileTextChanges().size() != 1U) {
        ADD_FAILURE() << "Expected exactly one file text change";
        return edits;
    }
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    if (fileEdit.textChanges.empty()) {
        ADD_FAILURE() << "Expected non-empty text changes";
        return edits;
    }
    const std::string actual = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(StripFormattingWhitespace(actual), StripFormattingWhitespace(expected));
    return edits;
}

std::pair<size_t, size_t> FindRangeByTokens(const std::string &code, std::string_view startToken,
                                            std::string_view endToken)
{
    const size_t spanStart = code.find(startToken);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = code.find(endToken);
    EXPECT_NE(spanEnd, std::string::npos);
    if (spanStart == std::string::npos || spanEnd == std::string::npos) {
        return {0, 0};
    }
    return {spanStart, spanEnd + endToken.size()};
}

void FindFunctionInsertAndReplaceTextChange(const FileTextChanges &fileEdit, const TextChange *&insertChange,
                                            const TextChange *&replaceChange)
{
    insertChange = nullptr;
    replaceChange = nullptr;
    for (const auto &change : fileEdit.textChanges) {
        if (change.span.length == 0 && change.newText.find("function newFunction") != std::string::npos) {
            insertChange = &change;
        }
        if (change.span.length > 0 && change.newText.find("newFunction(") != std::string::npos) {
            replaceChange = &change;
        }
    }
}

void ExpectFunctionInsideNamespaceAOutsideB(const std::string &actual)
{
    const size_t namespaceBPos = actual.find("namespace B");
    ASSERT_NE(namespaceBPos, std::string::npos) << actual;
    const size_t namespaceBClosePos = actual.find("\n  }\n", namespaceBPos);
    ASSERT_NE(namespaceBClosePos, std::string::npos) << actual;
    const size_t namespaceAPos = actual.find("namespace A");
    ASSERT_NE(namespaceAPos, std::string::npos) << actual;
    const size_t namespaceAClosePos = actual.rfind("\n}\n");
    ASSERT_NE(namespaceAClosePos, std::string::npos) << actual;
    const size_t extractedFuncPos = actual.find("function newFunction");
    ASSERT_NE(extractedFuncPos, std::string::npos) << actual;
    EXPECT_GT(extractedFuncPos, namespaceAPos);
    EXPECT_LT(extractedFuncPos, namespaceAClosePos);
    EXPECT_TRUE(extractedFuncPos < namespaceBPos || extractedFuncPos > namespaceBClosePos);
}

void ExpectGlobalExtractMethodInsertChange(const TextChange *insertChange)
{
    ASSERT_NE(insertChange, nullptr);
    EXPECT_NE(insertChange->newText.find("function newFunction("), std::string::npos);
    EXPECT_NE(insertChange->newText.find("x:"), std::string::npos);
    EXPECT_NE(insertChange->newText.find("tmp:"), std::string::npos);
    EXPECT_NE(insertChange->newText.find("foo:"), std::string::npos);
    EXPECT_NE(insertChange->newText.find("return tmp;"), std::string::npos);
}

void ExpectGlobalExtractMethodReplaceChange(const TextChange *replaceChange)
{
    ASSERT_NE(replaceChange, nullptr);
    EXPECT_NE(replaceChange->newText.find("tmp = newFunction(x, tmp, foo);"), std::string::npos);
}

void ExpectRenameLocOnExtractedCallName(const FileTextChanges &fileEdit, const TextChange *replaceChange,
                                        const ark::es2panda::lsp::RefactorEditInfo &edits)
{
    ASSERT_NE(replaceChange, nullptr);
    ASSERT_TRUE(edits.GetRenameLocation().has_value());
    size_t calleePos = replaceChange->newText.find("newFunction(");
    if (calleePos == std::string::npos) {
        calleePos = replaceChange->newText.find("newMethod(");
    }
    ASSERT_NE(calleePos, std::string::npos) << replaceChange->newText;
    std::vector<const TextChange *> ordered;
    ordered.reserve(fileEdit.textChanges.size());
    for (const auto &change : fileEdit.textChanges) {
        ordered.push_back(&change);
    }
    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const TextChange *lhs, const TextChange *rhs) { return lhs->span.start < rhs->span.start; });
    size_t shiftBeforeReplace = 0;
    for (const auto *change : ordered) {
        if (change == replaceChange) {
            break;
        }
        if (change->span.start + change->span.length <= replaceChange->span.start) {
            shiftBeforeReplace += change->newText.length() - change->span.length;
        }
    }
    const size_t expectedLoc = replaceChange->span.start + shiftBeforeReplace + calleePos + 1;
    EXPECT_EQ(edits.GetRenameLocation().value(), expectedLoc);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractVariableProhibited)
{
    const std::string code = R"(
const a = 1 + 1 * 3
)";
    const size_t spanStart = 11;
    const size_t spanEnd = 16;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;

    bool found = std::any_of(applicable.begin(), applicable.end(),
                             [&target](const auto &info) { return info.action.name == target; });
    EXPECT_FALSE(found);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractVariableTrig)
{
    const std::string code = R"(
const b = 1;
const a = 1;
)";
    const std::string expected = R"(
const b = 1;
const newLocal = 1;
const a = newLocal;
)";
    const size_t spanStart = 24;
    const size_t spanEnd = 25;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractVariableTrig1)
{
    const std::string code = R"(

const a = 1;
)";
    const std::string expected = R"(
const newLocal = 1;
const a = newLocal;
)";
    const size_t spanStart = 12;
    const size_t spanEnd = 13;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantAfterAnnotation)
{
    const std::string code = R"(
// testAnnotation
const a = 1 + 1;
)";
    const size_t spanStart = 29;
    const size_t spanEnd = 34;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, target));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    EXPECT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    EXPECT_FALSE(fileEdit.textChanges.empty());

    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "const newLocal = 1 + 1;";
    auto startPos1 = fileEdit.textChanges.at(0).span.start;
    int insertPos = 0;
    EXPECT_EQ(startPos1, insertPos);
    EXPECT_EQ(newText, expect);
    const std::string expected = R"(const newLocal = 1 + 1;
// testAnnotation
const a = newLocal;
)";
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantInSinglelineMultipleDeclarations)
{
    const std::string code = R"(
const a = 1, b = a + 1;
)";
    const std::string expr = "a + 1";
    const size_t spanStart = code.find(expr);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + expr.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, target));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const TextChange *insertChange = nullptr;
    for (const auto &change : fileEdit.textChanges) {
        if (change.span.length == 0 && change.newText.find("newLocal") != std::string::npos) {
            insertChange = &change;
            break;
        }
    }
    ASSERT_NE(insertChange, nullptr);
    EXPECT_EQ(insertChange->newText.rfind("newLocal = a + 1, ", 0), 0U);
    const std::string expected = R"(
const a = 1, newLocal = a + 1, b = newLocal;
)";
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}
TEST_F(LspExtrSymblGetEditsTests, ExtractConstantInMultipleDeclarations)
{
    std::string code = R"(
const a = 1,
  b = /*start*/a + 1/*end*/;
)";

    constexpr std::string_view START_MARKER = "/*start*/";
    constexpr std::string_view END_MARKER = "/*end*/";
    const size_t spanStart = code.find(START_MARKER);
    ASSERT_NE(spanStart, std::string::npos);
    code.erase(spanStart, START_MARKER.length());
    const size_t spanEnd = code.find(END_MARKER);
    ASSERT_NE(spanEnd, std::string::npos);
    code.erase(spanEnd, END_MARKER.length());

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, target));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const TextChange *insertChange = nullptr;
    for (const auto &change : fileEdit.textChanges) {
        if (change.span.length == 0 && change.newText.find("newLocal") != std::string::npos) {
            insertChange = &change;
            break;
        }
    }
    ASSERT_NE(insertChange, nullptr);
    EXPECT_EQ(insertChange->newText.rfind("newLocal = a + 1, ", 0), 0U);
    const std::string expected = R"(
const a = 1,
  newLocal = a + 1, b = newLocal;
)";
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantInMultipleDeclarationsWithLeadingComment)
{
    std::string code = R"(
const a = 1,
  /*aboutB*/b = /*start*/a + 1/*end*/;
)";

    constexpr std::string_view START_MARKER = "/*start*/";
    constexpr std::string_view END_MARKER = "/*end*/";
    const size_t spanStart = code.find(START_MARKER);
    ASSERT_NE(spanStart, std::string::npos);
    code.erase(spanStart, START_MARKER.length());
    const size_t spanEnd = code.find(END_MARKER);
    ASSERT_NE(spanEnd, std::string::npos);
    code.erase(spanEnd, END_MARKER.length());

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string expected = R"(
const a = 1,
  newLocal = a + 1, /*aboutB*/b = newLocal;
)";
    ExpectExtractionApplies(code, refactorContext, refactorName, actionName, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantInMultipleDeclarationsWithLeadingMultilineComment)
{
    std::string code = R"(
const a = 1,
/*about
B*/
b = /*start*/a + 1/*end*/;
)";

    constexpr std::string_view START_MARKER = "/*start*/";
    constexpr std::string_view END_MARKER = "/*end*/";
    const size_t spanStart = code.find(START_MARKER);
    ASSERT_NE(spanStart, std::string::npos);
    code.erase(spanStart, START_MARKER.length());
    const size_t spanEnd = code.find(END_MARKER);
    ASSERT_NE(spanEnd, std::string::npos);
    code.erase(spanEnd, END_MARKER.length());

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string expected = R"(
const a = 1,
  newLocal = a + 1, /*about
B*/
b = newLocal;
)";
    ExpectExtractionApplies(code, refactorContext, refactorName, actionName, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantInMultipleDeclarationsWithDetachedMultilineComment)
{
    std::string code = R"(
const a = 1,
/*about
B*/

b = /*start*/a + 1/*end*/;
)";

    constexpr std::string_view START_MARKER = "/*start*/";
    constexpr std::string_view END_MARKER = "/*end*/";
    const size_t spanStart = code.find(START_MARKER);
    ASSERT_NE(spanStart, std::string::npos);
    code.erase(spanStart, START_MARKER.length());
    const size_t spanEnd = code.find(END_MARKER);
    ASSERT_NE(spanEnd, std::string::npos);
    code.erase(spanEnd, END_MARKER.length());

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string expected = R"(
const a = 1,
/*about
B*/

newLocal = a + 1, b = newLocal;
)";
    ExpectExtractionApplies(code, refactorContext, refactorName, actionName, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantInMultipleDeclarationsWithLeadingSinglelineComment)
{
    std::string code = R"(
const a = 1,
// aboutB
b = /*start*/a + 1/*end*/;
)";

    constexpr std::string_view START_MARKER = "/*start*/";
    constexpr std::string_view END_MARKER = "/*end*/";
    const size_t spanStart = code.find(START_MARKER);
    ASSERT_NE(spanStart, std::string::npos);
    code.erase(spanStart, START_MARKER.length());
    const size_t spanEnd = code.find(END_MARKER);
    ASSERT_NE(spanEnd, std::string::npos);
    code.erase(spanEnd, END_MARKER.length());

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string expected = R"(
const a = 1,
  newLocal = a + 1, // aboutB
b = newLocal;
)";
    ExpectExtractionApplies(code, refactorContext, refactorName, actionName, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantWithSingleLineBlockCommentBetweenStatements)
{
    std::string code = R"(
const x = 1;
/* aboutX */
const y = /*start*/x + 1/*end*/;
)";

    constexpr std::string_view START_MARKER = "/*start*/";
    constexpr std::string_view END_MARKER = "/*end*/";
    const size_t spanStart = code.find(START_MARKER);
    ASSERT_NE(spanStart, std::string::npos);
    code.erase(spanStart, START_MARKER.length());
    const size_t spanEnd = code.find(END_MARKER);
    ASSERT_NE(spanEnd, std::string::npos);
    code.erase(spanEnd, END_MARKER.length());

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    const std::string expected = R"(
const x = 1;
const newLocal = x + 1;
/* aboutX */
const y = newLocal;
)";
    ExpectExtractionApplies(code, refactorContext, refactorName, actionName, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantInNamespace)
{
    const std::string code = R"(
namespace X {
  export const j = 10;
  export const y = j * j;
}
)";
    const std::string expected = R"(
namespace X {
  export const j = 10;

  const newLocal = j * j;
  export const y = newLocal;
}
)";
    const size_t spanStart = code.find("j * j");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("j * j").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits = ExpectExtractResultIgnoringWhitespace(refactorContext, code, expected, std::string(refactorName),
                                                       std::string(target));
    ASSERT_NE(edits, nullptr);
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantClassScopeDescriptionUsesClassName)
{
    const std::string code = R"(
class AccountingDepartment {
  amount = 1 + 1;
}
)";
    const size_t spanStart = code.find("1 + 1");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("1 + 1").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));

    const auto *classConst = FindApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name);
    ASSERT_NE(classConst, nullptr);
    EXPECT_EQ(classConst->action.description, "Extract to constant in class 'AccountingDepartment'");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractNamespaceActionsUseEnclosingScopeAndNamespaceDescription)
{
    const std::string code = R"(
namespace X {
  export const j = 10;
  export const y = j * j;
}
)";
    const size_t spanStart = code.find("j * j");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("j * j").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));

    const auto *constInNamespace =
        FindApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    ASSERT_NE(constInNamespace, nullptr);
    EXPECT_EQ(constInNamespace->action.description, "Extract to constant in enclosing scope");

    const auto *funcEnclose =
        FindApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    ASSERT_NE(funcEnclose, nullptr);
    EXPECT_EQ(funcEnclose->action.description, "Extract to function in namespace 'X'");

    const auto *funcClass = FindApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);
    EXPECT_EQ(funcClass, nullptr);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantInNamespaceClassShowsClassNamespaceGlobalActions)
{
    const std::string code = R"(
namespace N {
  class C {
    a = 1 + 1;
  }
}
)";
    const std::string expected = R"(
namespace N {
  const newLocal = 1 + 1;
  class C {
    a = newLocal;
  }
}
)";
    const size_t spanStart = code.find("1 + 1");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("1 + 1").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));

    const auto *constEnclose =
        FindApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    ASSERT_NE(constEnclose, nullptr) << FormatApplicableActions(applicable);
    EXPECT_EQ(constEnclose->action.description, "Extract to constant in namespace 'N'");

    const auto *classConst = FindApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name);
    ASSERT_NE(classConst, nullptr);
    EXPECT_EQ(classConst->action.description, "Extract to constant in class 'C'");

    const auto *globalConst = FindApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name);
    ASSERT_NE(globalConst, nullptr);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits =
        ExpectExtractResultIgnoringWhitespace(refactorContext, code, expected, refactorName,
                                              std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name));
    ASSERT_NE(edits, nullptr);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractFunctionInNamespaceClassEncloseInsertsIntoNamespace)
{
    const std::string code = R"(
namespace N {
  class C {
    a = 1 + 1;
  }
}
)";
    const size_t spanStart = code.find("1 + 1");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("1 + 1").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    const auto *funcInNamespace = FindApplicableAction(applicable, actionName);
    ASSERT_NE(funcInNamespace, nullptr) << FormatApplicableActions(applicable);
    EXPECT_EQ(funcInNamespace->action.description, "Extract to function in namespace 'N'");

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const TextChange *insertChange = nullptr;
    const TextChange *replaceChange = nullptr;
    FindFunctionInsertAndReplaceTextChange(fileEdit, insertChange, replaceChange);
    ASSERT_NE(insertChange, nullptr);
    ASSERT_NE(replaceChange, nullptr);

    const std::string actual = ApplyEdits(code, fileEdit.textChanges);
    const size_t namespacePos = actual.find("namespace N");
    ASSERT_NE(namespacePos, std::string::npos) << actual;
    const size_t classPos = actual.find("class C", namespacePos);
    ASSERT_NE(classPos, std::string::npos) << actual;
    const size_t classClosePos = actual.find("\n  }\n", classPos);
    ASSERT_NE(classClosePos, std::string::npos) << actual;
    const size_t namespaceClosePos = actual.rfind("\n}\n");
    ASSERT_NE(namespaceClosePos, std::string::npos) << actual;
    const size_t extractedFuncPos = actual.find("function newFunction", namespacePos);
    ASSERT_NE(extractedFuncPos, std::string::npos) << actual;

    EXPECT_GT(extractedFuncPos, namespacePos);
    EXPECT_LT(extractedFuncPos, namespaceClosePos);
    EXPECT_TRUE(extractedFuncPos < classPos || extractedFuncPos > classClosePos) << actual;

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractVariableInNestedNamespaceDefaultsToInnermostNamespace)
{
    const std::string code = R"(
namespace A {
  export namespace B {
    export const x = 1 + 2;
  }
}
)";
    const std::string expected = R"(
namespace A {
  export namespace B {
    let newLocal = 1 + 2;
    export const x = newLocal;
  }
}
)";
    const size_t spanStart = code.find("1 + 2");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("1 + 2").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    const auto *varEnclose = FindApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    ASSERT_NE(varEnclose, nullptr);

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractVariableInNestedNamespaceAvoidsNameCollision)
{
    const std::string code = R"(
namespace A {
  export namespace B {
    export const newLocal = 100;
    export const x = 1 + 2;
  }
}
)";
    const std::string expected = R"(
namespace A {
  export namespace B {
    export const newLocal = 100;

    let newLocal_1 = 1 + 2;
    export const x = newLocal_1;
  }
}
)";
    const size_t spanStart = code.find("1 + 2");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("1 + 2").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    auto edits = ExpectExtractResultIgnoringWhitespace(refactorContext, code, expected, refactorName, actionName);
    ASSERT_NE(edits, nullptr);
    const std::string token = "newLocal_1";
    ExpectRenameLocExact(expected, *edits, token);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantInNestedNamespaceUsesInnermostNamespaceScope)
{
    const std::string code = R"(
namespace A {
  export namespace B {
    export const x = 1 + 2;
  }
}
)";
    const std::string expected = R"(
namespace A {
  export namespace B {
    const newLocal = 1 + 2;
    export const x = newLocal;
  }
}
)";
    const size_t spanStart = code.find("1 + 2");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("1 + 2").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ExpectNestedNamespaceActionDescriptions(applicable);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name);
    auto edits = ExpectExtractResultIgnoringWhitespace(refactorContext, code, expected, refactorName, actionName);
    ASSERT_NE(edits, nullptr);
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantInNestedNamespaceSupportsOuterNamespaceScope)
{
    const std::string code = R"(
namespace A {
  export namespace B {
    export const x = 1 + 2;
  }
}
)";
    const std::string expected = R"(
namespace A {
  const newLocal = 1 + 2;
  export namespace B {
    export const x = newLocal;
  }
}
)";
    const size_t spanStart = code.find("1 + 2");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("1 + 2").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string actionName = "extract_constant_scope_ns_1";
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());

    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantInNestedNamespaceOuterScopeAfterExistingDecl)
{
    const std::string code = R"(
namespace A {
  const b = 2;
  export namespace B {
    export const x = 1 + 2;
  }
}
)";
    const std::string expected = R"(
namespace A {
  const b = 2;
  const newLocal = 1 + 2;
  export namespace B {
    export const x = newLocal;
  }
}
)";
    const size_t spanStart = code.find("1 + 2");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("1 + 2").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string actionName = "extract_constant_scope_ns_1";
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    auto edits = ExpectExtractResultIgnoringWhitespace(refactorContext, code, expected, refactorName, actionName);
    ASSERT_NE(edits, nullptr);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    const std::string actual = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_NE(actual.find("const b = 2;\n  const newLocal"), std::string::npos) << actual;
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractFunctionInNestedNamespaceSupportsOuterNamespaceScope)
{
    const std::string code = R"(
namespace A {
  export namespace B {
    export const x = 1 + 2;
  }
}
)";
    const size_t spanStart = code.find("1 + 2");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("1 + 2").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string actionName = "extract_function_scope_ns_1";
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const TextChange *insertChange = nullptr;
    const TextChange *replaceChange = nullptr;
    FindFunctionInsertAndReplaceTextChange(fileEdit, insertChange, replaceChange);
    ExpectRenameLocOnExtractedCallName(fileEdit, replaceChange, *edits);

    const std::string expected = R"(
namespace A {
  function newFunction() {
    return 1 + 2;
  }

  export namespace B {
    export const x = newFunction();
  }
}
)";
    initializer->DestroyContext(refactorContext->context);
}
TEST_F(LspExtrSymblGetEditsTests, ExtractVariableNoReference)
{
    const std::string code = R"(
class C {
  constructor() {
    this.m2()
  }
  m2() {return 1;}
}
)";
    // 1 * 3
    const size_t spanStart = 33;
    const size_t spanEnd = 42;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, target));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string expected = R"(
class C {
  constructor() {
    let newLocal = this.m2();
  }
  m2() {return 1;}
}
)";
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractVariableCrash1)
{
    const std::string code = R"(
let a = 1;
const b = a * a;
)";
    // 1 * 3
    const size_t spanStart = 22;
    const size_t spanEnd = 27;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, target));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string expected = R"(
let a = 1;
let newLocal = a * a;
const b = newLocal;
)";
    const std::string actual = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_EQ(StripFormattingWhitespace(actual), StripFormattingWhitespace(expected));
    const size_t first = actual.find("newLocal");
    ASSERT_NE(first, std::string::npos);
    const size_t second = actual.find("newLocal", first + 1);
    const size_t targetPos = (second == std::string::npos) ? first : second;
    ASSERT_TRUE(edits->GetRenameLocation().has_value());
    EXPECT_EQ(edits->GetRenameLocation().value(), targetPos + 1);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractVariableFromObjectLiteralWithUnionTypedInterface)
{
    std::string code = R"(
interface I {a:1|2|3}

let i:I = /*start*/{a:1}/*end*/;
)";
    constexpr std::string_view START_MARKER = "/*start*/";
    constexpr std::string_view END_MARKER = "/*end*/";
    const size_t spanStart = code.find(START_MARKER);
    ASSERT_NE(spanStart, std::string::npos);
    code.erase(spanStart, START_MARKER.length());
    const size_t spanEnd = code.find(END_MARKER);
    ASSERT_NE(spanEnd, std::string::npos);
    code.erase(spanEnd, END_MARKER.length());

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name);

    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string expected = R"(
interface I {a:1|2|3}

let newLocal = {a:1};
let i:I = newLocal;
)";

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantWrongRange3)
{
    const std::string code = R"(
    const a = 1 + 1 * 3;
)";
    // 1 * 3
    const size_t spanStart = 19;
    const size_t spanEnd = 24;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, target));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string expected = R"(const newLocal = 1 * 3;
    const a = 1 + newLocal;
)";
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantWrongRange2)
{
    const std::string code = R"(
    const a = 1 + 1 * 3;
)";
    // 1 + 1
    const size_t spanStart = 15;
    const size_t spanEnd = 20;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    EXPECT_EQ(edits->GetFileTextChanges().size(), 0);
    EXPECT_FALSE(edits->GetRenameLocation().has_value());

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantWrongRange1)
{
    const std::string code = R"(
    const fun = () => 1 + 2;
)";
    //  1 + 2
    const size_t spanStart = 22;
    const size_t spanEnd = 28;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, target));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string expected = R"(const newLocal = 1 + 2;
    const fun = () => newLocal;
)";
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantViaPublicAPI7)
{
    const std::string code = R"(
'use static'

const a: int = 1 + 1;
)";

    const size_t spanStart = 30;
    const size_t spanEnd = 35;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, target));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    EXPECT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    EXPECT_FALSE(fileEdit.textChanges.empty());

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "const newLocal = 1 + 1;";
    auto startPos1 = fileEdit.textChanges.at(0).span.start;
    const int insertPos = 14;
    EXPECT_EQ(startPos1, insertPos);
    EXPECT_EQ(newText, expect);
    const std::string expected = R"(
'use static'
const newLocal = 1 + 1;
const a: int = newLocal;
)";
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantViaPublicAPI6)
{
    const std::string code = R"(
'use static'

class test {
  a = 1 + 1;
}
)";

    const size_t spanStart = 34;
    const size_t spanEnd = 39;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, target));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    EXPECT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    EXPECT_FALSE(fileEdit.textChanges.empty());

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "private readonly newProperty = 1 + 1;\n";
    auto startPos1 = fileEdit.textChanges.at(0).span.start;
    const int insertPos = 27;
    EXPECT_EQ(startPos1, insertPos);
    EXPECT_EQ(newText, expect);
    const std::string expected = R"(
'use static'

class test {
  private readonly newProperty = 1 + 1;

  a = this.newProperty;
}
)";
    const std::string token = "this.newProperty";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantViaPublicAPI5)
{
    const std::string code = R"(
'use static'

class test {
  a = 1 + 1;
}
)";

    const size_t spanStart = 34;
    const size_t spanEnd = 39;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, target));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    EXPECT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    EXPECT_FALSE(fileEdit.textChanges.empty());

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "const newLocal = 1 + 1;";
    auto startPos1 = fileEdit.textChanges.at(0).span.start;
    const int insertPos = 14;
    EXPECT_EQ(startPos1, insertPos);
    EXPECT_EQ(newText, expect);
    const std::string expected = R"(
'use static'
const newLocal = 1 + 1;
class test {
  a = newLocal;
}
)";
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantViaPublicAPI4)
{
    const std::string code = R"(
'use static'

class test {
  a: string = "dfs";
  f = () => { return 1 + 1; };
}
)";

    const size_t spanStart = 70;
    const size_t spanEnd = 75;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, target));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    EXPECT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    EXPECT_FALSE(fileEdit.textChanges.empty());

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "const newLocal = 1 + 1;";
    auto startPos1 = fileEdit.textChanges.at(0).span.start;
    const int insertPos = 14;
    EXPECT_EQ(startPos1, insertPos);
    EXPECT_EQ(newText, expect);
    const std::string expected = R"(
'use static'
const newLocal = 1 + 1;
class test {
  a: string = "dfs";
  f = () => { return newLocal; };
}
)";
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantViaPublicAPI3)
{
    const std::string code = R"(
'use static'

class test {
  a: string = "dfs";
  f = () => { return 1 + 1; };
}
)";

    const size_t spanStart = 70;
    const size_t spanEnd = 75;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, target));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    EXPECT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    EXPECT_FALSE(fileEdit.textChanges.empty());

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "private readonly newProperty = 1 + 1;\n";
    auto startPos1 = fileEdit.textChanges.at(0).span.start;
    const int insertPos = 48;
    EXPECT_EQ(startPos1, insertPos);
    EXPECT_EQ(newText, expect);
    const std::string expected = R"(
'use static'

class test {
  a: string = "dfs";
  private readonly newProperty = 1 + 1;

  f = () => { return this.newProperty; };
}
)";
    const std::string token = "this.newProperty";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantViaPublicAPI2)
{
    const std::string code = R"(
'use static'

const f = () => {
  return 1 + 1;
};
)";

    const size_t spanStart = 41;
    const size_t spanEnd = 47;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    EXPECT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    EXPECT_FALSE(fileEdit.textChanges.empty());

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "const newLocal = 1 + 1;";
    auto startPos1 = fileEdit.textChanges.at(0).span.start;
    const int insertPos = 32;
    EXPECT_EQ(startPos1, insertPos);
    EXPECT_EQ(newText, expect);
    const std::string expected = R"(
'use static'

const f = () => {
  const newLocal = 1 + 1;
  return newLocal;
};
)";
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

// -----------------------------------------------------------------------------
// TEST 1: GetEditsForRefactorsImpl - Extract Constant 1
// -----------------------------------------------------------------------------
TEST_F(LspExtrSymblGetEditsTests, ExtractConstantViaPublicAPI1)
{
    const std::string code = R"(
    const kkmm = 1 + 1;

    const kks = kkmm + 1;
)";

    const size_t spanStart = 18;
    const size_t spanEnd = 23;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, target));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    EXPECT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    EXPECT_FALSE(fileEdit.textChanges.empty());

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "const newLocal = 1 + 1;";
    auto startPos1 = fileEdit.textChanges.at(0).span.start;
    EXPECT_EQ(startPos1, 0);
    EXPECT_EQ(newText, expect);
    const std::string expected = R"(const newLocal = 1 + 1;
    const kkmm = newLocal;

    const kks = kkmm + 1;
)";
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

// -----------------------------------------------------------------------------
// TEST 1: GetEditsForRefactorsImpl - Extract Constant
// -----------------------------------------------------------------------------
TEST_F(LspExtrSymblGetEditsTests, ExtractConstantViaPublicAPI)
{
    const std::string code = R"(function main() {
  let x = 10;
  let y = 20;
  console.log("x + y = " + (x + y));
})";

    const size_t spanStart = code.find("x + y");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("x + y").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name;
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;

    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    EXPECT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    EXPECT_FALSE(fileEdit.textChanges.empty());

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "const newLocal = \"x + y = \";";
    auto startPos1 = fileEdit.textChanges.at(0).span.start;
    // NOLINTNEXTLINE(readability-identifier-naming)
    const size_t expectedInsertPos = code.find("\n  console.log");
    ASSERT_NE(expectedInsertPos, std::string::npos);
    EXPECT_EQ(startPos1, expectedInsertPos);
    EXPECT_EQ(newText, expect);
    const std::string expected = R"(function main() {
  let x = 10;
  let y = 20;
  const newLocal = "x + y = ";
  console.log(newLocal + (x + y));
}))";
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractConstantViaGlobalPublicAPI)
{
    const std::string code = R"(
    import hilog from '@ohos.hilog'
    const a = 42;
    function main() {
    let x = 10;
    let y = 20;
    console.log("x + y = " + (x + y));
})";

    const size_t spanStart = 125;
    const size_t spanEnd = 135;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    const std::string_view target = ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name;
    ASSERT_TRUE(HasApplicableAction(applicable, target));
    const std::string_view refactorName = ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME;
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), std::string(target));

    EXPECT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    EXPECT_FALSE(fileEdit.textChanges.empty());

    // Expect generated const extraction
    std::string_view newText = fileEdit.textChanges.at(0).newText;
    std::string_view expect = "const newLocal = \"x + y = \";";
    auto startPos1 = fileEdit.textChanges.at(0).span.start;

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t expectedInsertPos = 54;
    EXPECT_EQ(startPos1, expectedInsertPos);
    EXPECT_EQ(newText, expect);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractVariableFromMethodEncloseScope)
{
    const std::string code = R"('use static'

class AccountingDepartment {
  name: string = '';

  printName(): void {
    console.log('Department name:' + this.name);
  }
}
)";
    const std::string expected = R"('use static'

class AccountingDepartment {
  name: string = '';

  printName(): void {
    let newLocal = 'Department name:';
    console.log(newLocal + this.name);
  }
}
)";
    const std::string target = "'Department name:'";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    refactorContext->kind = "refactor.extract.variable";

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    ASSERT_TRUE(HasApplicableAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string token = "newLocal";
    ExpectRenameLocExact(expected, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractVariableGlobalFromObjectLiteralAtTopLevel)
{
    const std::string code = R"(
interface I { a: 1 | 2 | 3 }
let i: I = { a: 1 }
)";
    const size_t spanStart = code.find("{ a: 1 }");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("{ a: 1 }").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name))
        << FormatApplicableActions(applicable);
    EXPECT_FALSE(HasApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name))
        << FormatApplicableActions(applicable);
    EXPECT_FALSE(HasApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name))
        << FormatApplicableActions(applicable);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());

    const std::string actual = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_NE(actual.find("let newLocal: I = { a: 1 };"), std::string::npos) << actual;
    EXPECT_NE(actual.find("let i: I = newLocal"), std::string::npos) << actual;

    const size_t stmtStart = code.find("let i: I = { a: 1 }");
    ASSERT_NE(stmtStart, std::string::npos);
    const TextChange *insertChange = nullptr;
    const TextChange *replaceChange = nullptr;
    for (const auto &change : fileEdit.textChanges) {
        if (change.span.length == 0 && change.newText.find("let newLocal: I = { a: 1 };") != std::string::npos) {
            insertChange = &change;
        }
        if (change.span.length > 0 && change.newText.find("newLocal") != std::string::npos) {
            replaceChange = &change;
        }
    }
    ASSERT_NE(insertChange, nullptr);
    ASSERT_NE(replaceChange, nullptr);
    EXPECT_EQ(insertChange->span.start, stmtStart) << actual;
    ExpectVariableRenameLocOnUsage(code, fileEdit, *edits, "newLocal");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractVariableEncloseInMethodKeepsStableInsertAndRenameLoc)
{
    const std::string code = R"(
class C {
  f() {
    let value = 1 + 2;
  }
}
)";
    const size_t spanStart = code.find("1 + 2");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("1 + 2").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());

    const TextChange *insertChange = nullptr;
    const TextChange *replaceChange = nullptr;
    for (const auto &change : fileEdit.textChanges) {
        if (change.span.length == 0 && change.newText.find("newLocal") != std::string::npos &&
            change.newText.find('=') != std::string::npos) {
            insertChange = &change;
        }
        if (change.span.length > 0 && change.newText.find("newLocal") != std::string::npos) {
            replaceChange = &change;
        }
    }
    ASSERT_NE(insertChange, nullptr);
    ASSERT_NE(replaceChange, nullptr);
    const size_t stmtStart = code.find("    let value = 1 + 2;");
    ASSERT_NE(stmtStart, std::string::npos);
    EXPECT_LE(insertChange->span.start, stmtStart);

    const std::string actual = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_NE(actual.find("newLocal"), std::string::npos) << actual;
    EXPECT_NE(actual.find("value = newLocal"), std::string::npos) << actual;
    ExpectVariableRenameLocOnUsage(code, fileEdit, *edits, "newLocal");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractVariableEncloseInNestedNamespaceKeepsStableInsertAndRenameLoc)
{
    const std::string code = R"(
namespace A {
  export namespace B {
    export const x = 1 + 2;
  }
}
)";
    const size_t spanStart = code.find("1 + 2");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("1 + 2").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());

    const TextChange *insertChange = nullptr;
    const TextChange *replaceChange = nullptr;
    for (const auto &change : fileEdit.textChanges) {
        if (change.span.length == 0 && change.newText.find("newLocal") != std::string::npos &&
            change.newText.find('=') != std::string::npos) {
            insertChange = &change;
        }
        if (change.span.length > 0 && change.newText.find("newLocal") != std::string::npos) {
            replaceChange = &change;
        }
    }
    ASSERT_NE(insertChange, nullptr);
    ASSERT_NE(replaceChange, nullptr);
    const size_t stmtStart = code.find("    export const x = 1 + 2;");
    ASSERT_NE(stmtStart, std::string::npos);
    EXPECT_LE(insertChange->span.start, stmtStart);

    const std::string actual = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_NE(actual.find("newLocal"), std::string::npos) << actual;
    EXPECT_NE(actual.find("export const x = newLocal"), std::string::npos) << actual;
    ExpectVariableRenameLocOnUsage(code, fileEdit, *edits, "newLocal");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractVariableInNestedNamespaceDoesNotOfferGlobalScope)
{
    const std::string code = R"(
namespace A {
  export namespace B {
    export const x = 1 + 2;
  }
}
)";
    const size_t spanStart = code.find("1 + 2");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("1 + 2").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    EXPECT_TRUE(HasApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name))
        << FormatApplicableActions(applicable);
    EXPECT_FALSE(HasApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name))
        << FormatApplicableActions(applicable);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractFunctionViaPublicAPI)
{
    const std::string code = R"(
const kkmm = 1 + 1;
const kks = kkmm + 1;
)";

    const std::string expected = R"(
const kkmm = 1 + 1;
function newFunction() {
  return kkmm + 1;
}

const kks = newFunction();
)";

    const size_t spanStart = 33;
    const size_t spanEnd = 41;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    // Step 1: get applicable refactors
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    ASSERT_TRUE(HasApplicableAction(applicable, globalScopeAction));
    // Step 2: run GetEditsForRefactorsImpl
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, std::string(refactorName), globalScopeAction);

    EXPECT_EQ(edits->GetFileTextChanges().size(), 1);

    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    EXPECT_FALSE(fileEdit.textChanges.empty());
    const std::string token = "newFunction";
    ExpectRenameLocExact(expected, *edits, token);
    ExpectExtractionApplies(code, refactorContext, refactorName, globalScopeAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodGlobal)
{
    const std::string code = R"('use static'

function newFunction(a: number, b: number) {
    let c = a + b;
    return c;
}

class MyClass {

    MyMethod(a: number, b: number) {
        let c = a + b;
        let d = c * c;
        return d;
    }
}
)";
    const std::string expected = R"('use static'

function newFunction_1(a: number, b: number) {
    return a + b;
}

function newFunction(a: number, b: number) {
    let c = a + b;
    return c;
}

class MyClass {

    MyMethod(a: number, b: number) {

        let c = newFunction_1(a, b);
        let d = c * c;
        return d;
    }
}
)";

    const std::string target = "let c = a + b;";
    const size_t spanStart = code.find(target, code.find("MyMethod"));
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, globalScopeAction));

    ExpectExtractionApplies(code, refactorContext, refactorName, globalScopeAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodGlobalMultiLineRenameLoc)
{
    std::string code = R"('use static'

class MyClass {

  MyMethod(a: number, b: number) {
    let c = a + b;
    let d = c * c;
  }
}
)";

    const auto [spanStart, spanEnd] = FindRangeByTokens(code, "let c = a + b;", "let d = c * c;");

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, globalScopeAction));

    auto edits = GetEditsViaLspApi(refactorContext, refactorName, globalScopeAction);
    ASSERT_NE(edits, nullptr);
    const auto &fileEdit = edits->GetFileTextChanges().front();
    const std::string actual = ApplyEdits(code, fileEdit.textChanges);
    const std::string token = "newFunction";
    const std::string defPrefix = "function " + token;
    const size_t defPos = actual.find(defPrefix);
    ASSERT_NE(defPos, std::string::npos) << actual;
    const size_t defNamePos = defPos + std::string("function ").size();
    const size_t first = actual.find(token);
    ASSERT_NE(first, std::string::npos) << actual;
    const size_t second = actual.find(token, first + 1);
    ASSERT_NE(second, std::string::npos) << actual;
    const size_t callPos = (first == defNamePos) ? second : first;
    ASSERT_TRUE(edits->GetRenameLocation().has_value());
    const size_t expectedRenameLoc = callPos + (token.size() > 1 ? 1 : 0);
    EXPECT_EQ(edits->GetRenameLocation().value(), expectedRenameLoc) << actual;

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodHelperGlobalRenameLoc)
{
    const std::string code = R"('use static'

class MyClass {

  MyMethod(a: number, b: number) {
    let sum = a + b;
    return sum;
  }
}
)";

    const auto [spanStart, spanEnd] = FindRangeByTokens(code, "let sum = a + b;", "let sum = a + b;");

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, globalScopeAction));

    auto edits = GetEditsViaLspApi(refactorContext, refactorName, globalScopeAction);
    ASSERT_NE(edits, nullptr);
    const auto &fileEdit = edits->GetFileTextChanges().front();
    const TextChange *insertChange = nullptr;
    const TextChange *replaceChange = nullptr;
    FindFunctionInsertAndReplaceTextChange(fileEdit, insertChange, replaceChange);
    ASSERT_NE(insertChange, nullptr);
    ASSERT_NE(replaceChange, nullptr);
    EXPECT_NE(insertChange->newText.find("function newFunction("), std::string::npos);
    EXPECT_NE(replaceChange->newText.find("newFunction("), std::string::npos);
    ExpectRenameLocOnExtractedCallName(fileEdit, replaceChange, *edits);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodHelperClassRenameLoc)
{
    const std::string code = R"('use static'

class MyClass {

  MyMethod(a: number, b: number) {
    let prod = a * b;
    return prod;
  }
}
)";

    const auto [spanStart, spanEnd] = FindRangeByTokens(code, "let prod = a * b;", "let prod = a * b;");

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string classScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, classScopeAction));

    auto edits = GetEditsViaLspApi(refactorContext, refactorName, classScopeAction);
    ASSERT_NE(edits, nullptr);
    const auto &fileEdit = edits->GetFileTextChanges().front();
    const TextChange *replaceChange = nullptr;
    for (const auto &change : fileEdit.textChanges) {
        if (change.span.length > 0 && change.newText.find("newMethod(") != std::string::npos) {
            replaceChange = &change;
            break;
        }
    }
    ASSERT_NE(replaceChange, nullptr);
    ExpectRenameLocOnExtractedCallName(fileEdit, replaceChange, *edits);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodGlobalForRename)
{
    const std::string code = std::string(K_EXTRACT_METHOD_GLOBAL_FOR_RENAME_CODE);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr std::string_view target = "let c = a + b;";
    const size_t spanStart = code.find(target, code.find("MyMethod"));
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string globalScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, globalScopeAction));

    auto edits = GetEditsViaLspApi(refactorContext, refactorName, globalScopeAction);
    ASSERT_NE(edits, nullptr);
    const auto &fileEdit = edits->GetFileTextChanges().front();
    const std::string token = "newFunction_1";
    ExpectVariableRenameLocOnUsage(code, fileEdit, *edits, token);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodPartialOverlapSelectionHasNoApplicableActions)
{
    const std::string code = R"(
class A {
  func(a: number, b: number) {
    let c = a + b;
    let d = c * c;
    return d;
  }
}
)";
    const size_t spanStart = code.find("c = a + b;");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t dStmtStart = code.find("let d = c * c;");
    ASSERT_NE(dStmtStart, std::string::npos);
    const size_t spanEnd = dStmtStart + std::string("let d = c * c").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(HasApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name))
        << FormatApplicableActions(applicable);
    EXPECT_FALSE(HasApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name))
        << FormatApplicableActions(applicable);
    EXPECT_FALSE(HasApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name))
        << FormatApplicableActions(applicable);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodMidTokenToNextStatementHasNoApplicableActions)
{
    const std::string code = R"(
class A {
  func(a: number, b: number) {
    let c = a + b;
    let d = c * c;
    return d;
  }
}
)";
    const size_t spanStart = code.find("b;");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t dStmtStart = code.find("let d = c * c;");
    ASSERT_NE(dStmtStart, std::string::npos);
    const size_t spanEnd = dStmtStart + std::string("let d = c * c").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(HasApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name))
        << FormatApplicableActions(applicable);
    EXPECT_FALSE(HasApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name))
        << FormatApplicableActions(applicable);
    EXPECT_FALSE(HasApplicableAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name))
        << FormatApplicableActions(applicable);
    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodClass)
{
    const std::string code = R"('use static'

class MyClass {

    MyMethod(a: number, b: number) {
        let c = a + b;
        let d = c * c;
        return d;
    }
}
)";
    const std::string expected = R"('use static'

class MyClass {

    MyMethod(a: number, b: number) {

        let c = this.newMethod(a, b);
        let d = c * c;
        return d;
    }

    private newMethod(a: number, b: number) {
        let c = a + b;
        return c;
    }
}
)";

    const std::string target = "let c = a + b;";
    const size_t spanStart = code.find(target);
    EXPECT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string classScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    ASSERT_TRUE(HasApplicableAction(applicable, classScopeAction));

    ExpectExtractionApplies(code, refactorContext, refactorName, classScopeAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodNestedNamespaceMultiStatements)
{
    const std::string code = std::string(K_NESTED_NAMESPACE_EXTRACT_METHOD_CODE);
    const auto [spanStart, spanEnd] = FindRangeByTokens(code, "let y = 3;", "foo();");

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    ASSERT_TRUE(HasApplicableAction(applicable, actionName));
    const auto *funcInNamespace = FindApplicableAction(applicable, actionName);
    ASSERT_NE(funcInNamespace, nullptr);
    EXPECT_EQ(funcInNamespace->action.description, "Extract to function in namespace 'B'")
        << FormatApplicableActions(applicable);
    const auto *funcInOuterNamespace = FindApplicableAction(applicable, "extract_function_scope_ns_1");
    ASSERT_NE(funcInOuterNamespace, nullptr) << FormatApplicableActions(applicable);
    EXPECT_EQ(funcInOuterNamespace->action.description, "Extract to function in namespace 'A'");

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const TextChange *insertChange = nullptr;
    const TextChange *replaceChange = nullptr;
    FindFunctionInsertAndReplaceTextChange(fileEdit, insertChange, replaceChange);
    ASSERT_NE(insertChange, nullptr);
    ASSERT_NE(replaceChange, nullptr);
    ExpectRenameLocOnExtractedCallName(fileEdit, replaceChange, *edits);
    const size_t namespaceBPosInCode = code.find("namespace B {");
    ASSERT_NE(namespaceBPosInCode, std::string::npos);
    const size_t namespaceBCloseLineStart = code.find("\n  }\n", namespaceBPosInCode);
    ASSERT_NE(namespaceBCloseLineStart, std::string::npos);
    EXPECT_EQ(insertChange->span.start, namespaceBCloseLineStart + 1);
    const std::string actual = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_NE(actual.find("tmp = newFunction(tmp);"), std::string::npos) << actual;
    EXPECT_EQ(actual.find("tmp = newFunction(x, tmp, foo);"), std::string::npos);
    EXPECT_NE(actual.find("return tmp;"), std::string::npos);
    const size_t namespaceBPos = actual.find("namespace B");
    ASSERT_NE(namespaceBPos, std::string::npos);
    const size_t extractedFuncPos = actual.find("function newFunction", namespaceBPos);
    ASSERT_NE(extractedFuncPos, std::string::npos);
    const size_t hostFuncPos = actual.find("function a()", namespaceBPos);
    ASSERT_NE(hostFuncPos, std::string::npos);
    EXPECT_GT(extractedFuncPos, hostFuncPos);
    const size_t namespaceBClosePos = actual.find("\n  }\n", hostFuncPos);
    ASSERT_NE(namespaceBClosePos, std::string::npos);
    EXPECT_LT(extractedFuncPos, namespaceBClosePos);
    ASSERT_TRUE(edits->GetRenameLocation().has_value());
    EXPECT_LT(edits->GetRenameLocation().value(), extractedFuncPos);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodNestedNamespaceMultiStatementsWithCommentsEnclose)
{
    const std::string code = std::string(K_NESTED_NAMESPACE_EXTRACT_METHOD_WITH_COMMENTS_CODE);
    const auto [spanStart, spanEnd] = FindRangeByTokens(code, "let y = 3;", "foo();");

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    ASSERT_TRUE(HasApplicableAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());

    const TextChange *insertChange = nullptr;
    const TextChange *replaceChange = nullptr;
    FindFunctionInsertAndReplaceTextChange(fileEdit, insertChange, replaceChange);
    ASSERT_NE(insertChange, nullptr);
    ASSERT_NE(replaceChange, nullptr);
    ExpectRenameLocOnExtractedCallName(fileEdit, replaceChange, *edits);
    EXPECT_NE(insertChange->newText.find("// keep-line-comment"), std::string::npos) << insertChange->newText;
    EXPECT_NE(insertChange->newText.find("/* keep-block-comment */"), std::string::npos) << insertChange->newText;
    EXPECT_NE(replaceChange->newText.find("tmp = newFunction(tmp);"), std::string::npos) << replaceChange->newText;

    const std::string actual = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_NE(actual.find("// keep-line-comment"), std::string::npos) << actual;
    EXPECT_NE(actual.find("/* keep-block-comment */"), std::string::npos) << actual;

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodNestedNamespaceGlobalParams)
{
    const std::string code = std::string(K_NESTED_NAMESPACE_EXTRACT_METHOD_CODE);
    const auto [spanStart, spanEnd] = FindRangeByTokens(code, "let y = 3;", "foo();");

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    ASSERT_TRUE(HasApplicableAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string actual = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_NE(actual.find("tmp = newFunction(x, tmp, foo);"), std::string::npos);
    const size_t extractedFuncPos = actual.find("function newFunction");
    ASSERT_NE(extractedFuncPos, std::string::npos);
    const size_t namespaceAPos = actual.find("namespace A");
    ASSERT_NE(namespaceAPos, std::string::npos);
    EXPECT_GT(extractedFuncPos, namespaceAPos);
    ASSERT_TRUE(edits->GetRenameLocation().has_value());
    EXPECT_LT(edits->GetRenameLocation().value(), extractedFuncPos);

    const TextChange *insertChange = nullptr;
    const TextChange *replaceChange = nullptr;
    FindFunctionInsertAndReplaceTextChange(fileEdit, insertChange, replaceChange);
    ExpectGlobalExtractMethodInsertChange(insertChange);
    ExpectGlobalExtractMethodReplaceChange(replaceChange);
    ExpectRenameLocOnExtractedCallName(fileEdit, replaceChange, *edits);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodNestedNamespaceMultiStatementsWithCommentsGlobal)
{
    const std::string code = std::string(K_NESTED_NAMESPACE_EXTRACT_METHOD_WITH_COMMENTS_CODE);
    const auto [spanStart, spanEnd] = FindRangeByTokens(code, "let y = 3;", "foo();");

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    ASSERT_TRUE(HasApplicableAction(applicable, actionName));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());

    const TextChange *insertChange = nullptr;
    const TextChange *replaceChange = nullptr;
    FindFunctionInsertAndReplaceTextChange(fileEdit, insertChange, replaceChange);
    ASSERT_NE(insertChange, nullptr);
    ASSERT_NE(replaceChange, nullptr);
    ExpectRenameLocOnExtractedCallName(fileEdit, replaceChange, *edits);

    EXPECT_NE(insertChange->newText.find("// keep-line-comment"), std::string::npos) << insertChange->newText;
    EXPECT_NE(insertChange->newText.find("/* keep-block-comment */"), std::string::npos) << insertChange->newText;
    EXPECT_NE(insertChange->newText.find("x:"), std::string::npos) << insertChange->newText;
    EXPECT_NE(insertChange->newText.find("tmp:"), std::string::npos) << insertChange->newText;
    EXPECT_NE(insertChange->newText.find("foo:"), std::string::npos) << insertChange->newText;
    EXPECT_NE(replaceChange->newText.find("tmp = newFunction(x, tmp, foo);"), std::string::npos)
        << replaceChange->newText;

    const std::string actual = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_NE(actual.find("// keep-line-comment"), std::string::npos) << actual;
    EXPECT_NE(actual.find("/* keep-block-comment */"), std::string::npos) << actual;

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodExportedNestedNamespaceExpressionGlobalParams)
{
    const std::string code = std::string(K_EXPORTED_NESTED_NAMESPACE_EXTRACT_EXPR_GLOBAL_CODE);
    const size_t spanStart = code.find("y + 1");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("y + 1").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    auto edits = GetEditsViaLspApi(refactorContext, refactorName, actionName);
    ASSERT_NE(edits, nullptr);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    const TextChange *insertChange = nullptr;
    const TextChange *replaceChange = nullptr;
    FindFunctionInsertAndReplaceTextChange(fileEdit, insertChange, replaceChange);
    ASSERT_NE(insertChange, nullptr);
    ASSERT_NE(replaceChange, nullptr);
    EXPECT_NE(insertChange->newText.find("function newFunction(y: Int)"), std::string::npos) << insertChange->newText;
    EXPECT_NE(replaceChange->newText.find("newFunction(y)"), std::string::npos) << replaceChange->newText;
    ExpectRenameLocOnExtractedCallName(fileEdit, replaceChange, *edits);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodNestedNamespaceOuterScopeParams)
{
    const std::string code = std::string(K_NESTED_NAMESPACE_EXTRACT_METHOD_CODE);
    const auto [spanStart, spanEnd] = FindRangeByTokens(code, "let y = 3;", "foo();");

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    const std::string actionName = "extract_function_scope_ns_1";
    ASSERT_TRUE(HasApplicableAction(applicable, actionName));
    const auto *outerScopeAction = FindApplicableAction(applicable, actionName);
    ASSERT_NE(outerScopeAction, nullptr);
    EXPECT_EQ(outerScopeAction->action.description, "Extract to function in namespace 'A'");

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const TextChange *insertChange = nullptr;
    const TextChange *replaceChange = nullptr;
    FindFunctionInsertAndReplaceTextChange(fileEdit, insertChange, replaceChange);
    ASSERT_NE(insertChange, nullptr);
    ASSERT_NE(replaceChange, nullptr);
    ExpectRenameLocOnExtractedCallName(fileEdit, replaceChange, *edits);
    const size_t namespaceBPosInCode = code.find("namespace B {");
    ASSERT_NE(namespaceBPosInCode, std::string::npos);
    const size_t namespaceBCloseLineStart = code.find("\n  }\n", namespaceBPosInCode);
    ASSERT_NE(namespaceBCloseLineStart, std::string::npos);
    const size_t namespaceACloseLineStart = code.find("\n}\n", namespaceBCloseLineStart + 1);
    ASSERT_NE(namespaceACloseLineStart, std::string::npos);
    EXPECT_EQ(insertChange->span.start, namespaceACloseLineStart + 1);
    const std::string actual = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_NE(actual.find("tmp = newFunction(tmp);"), std::string::npos);
    EXPECT_EQ(actual.find("tmp = newFunction(x, tmp, foo);"), std::string::npos);
    EXPECT_NE(actual.find("function newFunction(tmp: Int)"), std::string::npos) << actual;
    const size_t namespaceBPos = actual.find("namespace B");
    ASSERT_NE(namespaceBPos, std::string::npos);
    const size_t extractedFuncPos = actual.find("function newFunction", namespaceBPos);
    ASSERT_NE(extractedFuncPos, std::string::npos);
    const size_t namespaceBClosePos = actual.find("\n  }\n", namespaceBPos);
    ASSERT_NE(namespaceBClosePos, std::string::npos);
    EXPECT_GT(extractedFuncPos, namespaceBClosePos);
    ASSERT_TRUE(edits->GetRenameLocation().has_value());
    EXPECT_LT(edits->GetRenameLocation().value(), extractedFuncPos);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodNestedNamespaceExpressionToOuterNamespace)
{
    const std::string code = std::string(K_NESTED_NAMESPACE_EXTRACT_EXPR_TO_OUTER_CODE);
    const size_t spanStart = code.find("y + 1");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = spanStart + std::string("y + 1").size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    const std::string actionName = "extract_function_scope_ns_1";
    ASSERT_TRUE(HasApplicableAction(applicable, actionName));
    const auto *outerScopeAction = FindApplicableAction(applicable, actionName);
    ASSERT_NE(outerScopeAction, nullptr);
    EXPECT_EQ(outerScopeAction->action.description, "Extract to function in namespace 'A'");

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ExpectExtractResultIgnoringWhitespace(refactorContext, code,
                                                       std::string(K_NESTED_NAMESPACE_EXTRACT_EXPR_TO_OUTER_EXPECTED),
                                                       refactorName, actionName);
    ASSERT_NE(edits, nullptr);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    const std::string actual = ApplyEdits(code, fileEdit.textChanges);
    ExpectFunctionInsideNamespaceAOutsideB(actual);

    const TextChange *insertChange = nullptr;
    const TextChange *replaceChange = nullptr;
    FindFunctionInsertAndReplaceTextChange(fileEdit, insertChange, replaceChange);
    ASSERT_NE(insertChange, nullptr);
    ASSERT_NE(replaceChange, nullptr);
    ExpectRenameLocOnExtractedCallName(fileEdit, replaceChange, *edits);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodNestedNamespaceMultiStatementsLineSelection)
{
    const std::string code = std::string(K_NESTED_NAMESPACE_EXTRACT_METHOD_CODE);
    const std::string startToken = "      let y = 3;";
    const std::string endToken = "      foo();\n";
    const size_t spanStart = code.find(startToken);
    ASSERT_NE(spanStart, std::string::npos);
    const size_t endTokenPos = code.find("      foo();");
    ASSERT_NE(endTokenPos, std::string::npos);
    const size_t spanEnd = endTokenPos + endToken.size();

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasExecutableRefactorAction(applicable));
    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name);
    ASSERT_TRUE(HasApplicableAction(applicable, actionName));
    const auto *funcInNamespace = FindApplicableAction(applicable, actionName);
    ASSERT_NE(funcInNamespace, nullptr);
    EXPECT_EQ(funcInNamespace->action.description, "Extract to function in namespace 'B'");
    const auto *funcInOuterNamespace = FindApplicableAction(applicable, "extract_function_scope_ns_1");
    ASSERT_NE(funcInOuterNamespace, nullptr);
    EXPECT_EQ(funcInOuterNamespace->action.description, "Extract to function in namespace 'A'");

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    const std::string actual = ApplyEdits(code, fileEdit.textChanges);
    EXPECT_NE(actual.find("tmp = newFunction(tmp);"), std::string::npos) << actual;
    EXPECT_EQ(actual.find("tmp = newFunction(x, tmp, foo);"), std::string::npos);
    EXPECT_NE(actual.find("function newFunction(tmp: Int)"), std::string::npos) << actual;
    const size_t namespaceBPos = actual.find("namespace B");
    ASSERT_NE(namespaceBPos, std::string::npos);
    const size_t extractedFuncPos = actual.find("function newFunction", namespaceBPos);
    ASSERT_NE(extractedFuncPos, std::string::npos);
    const size_t hostFuncPos = actual.find("function a()", namespaceBPos);
    ASSERT_NE(hostFuncPos, std::string::npos);
    EXPECT_GT(extractedFuncPos, hostFuncPos);
    const size_t namespaceBClosePos = actual.find("\n  }\n", hostFuncPos);
    ASSERT_NE(namespaceBClosePos, std::string::npos);
    EXPECT_LT(extractedFuncPos, namespaceBClosePos);
    ASSERT_TRUE(edits->GetRenameLocation().has_value());
    EXPECT_LT(edits->GetRenameLocation().value(), extractedFuncPos);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractVariableForSpecialCharacters)
{
    const std::string code = R"('use static'

class AccountingDepartment {
    //中文测试
    name: string = '中文测试';
    //中文测试
    printName(): void {
        //中文测试
        console.log('中文测试' + this.name);
    }
}
)";

    const size_t spanStart = 151;
    const size_t spanEnd = 157;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    refactorContext->kind = "refactor.extract.variable";

    LSPAPI const *lspApi = GetImpl();
    auto applicable =
        lspApi->getApplicableRefactors(refactorContext->context, refactorContext->kind.c_str(), spanStart, spanEnd);

    EXPECT_FALSE(applicable.empty());

    const std::string actionName = std::string(ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name);
    const bool hasVariableEnclose = std::any_of(applicable.begin(), applicable.end(),
                                                [&](const auto &info) { return info.action.name == actionName; });
    EXPECT_TRUE(hasVariableEnclose);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto edits = lspApi->getEditsForRefactor(*refactorContext, refactorName, actionName);
    ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
    const auto &fileEdit = edits->GetFileTextChanges().at(0);
    ASSERT_FALSE(fileEdit.textChanges.empty());
    ASSERT_EQ(fileEdit.textChanges.size(), 2U);
    EXPECT_EQ(fileEdit.textChanges[0].span.start, 139U);
    EXPECT_EQ(fileEdit.textChanges[0].span.length, 0U);
    EXPECT_EQ(fileEdit.textChanges[0].newText, "let newLocal = '中文测试';");
    EXPECT_EQ(fileEdit.textChanges[1].span.start, 183U);
    EXPECT_EQ(fileEdit.textChanges[1].span.length, 14U);
    EXPECT_EQ(fileEdit.textChanges[1].newText, "newLocal");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtrSymblGetEditsTests, ExtractMethodForSpecialCharacters)
{
    const std::string code = R"('use static'

class MyClass {
    //中文测试
    MyMethod(a: string, b: number) {
        //中文测试
        let c = a + "中文测试";
        return c;
    }
}
)";
    const std::string expected = R"('use static'

class MyClass {
    //中文测试
    MyMethod(a: string, b: number) {
        //中文测试

        let c = this.newMethod(a);
        return c;
    }

    private newMethod(a: string) {
        let c = a + "中文测试";
        return c;
    }
}
)";

    const size_t spanStart = 101;
    const size_t spanEnd = 120;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);
    refactorContext->kind = "refactor.extract.function";

    LSPAPI const *lspApi = GetImpl();
    auto applicable =
        lspApi->getApplicableRefactors(refactorContext->context, refactorContext->kind.c_str(), spanStart, spanEnd);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    const std::string classScopeAction = std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name);

    const bool hasClass = std::any_of(applicable.begin(), applicable.end(),
                                      [&](const auto &info) { return info.action.name == classScopeAction; });
    EXPECT_TRUE(hasClass);

    ExpectExtractionApplies(code, refactorContext, refactorName, classScopeAction, expected);

    initializer->DestroyContext(refactorContext->context);
}

}  // namespace
