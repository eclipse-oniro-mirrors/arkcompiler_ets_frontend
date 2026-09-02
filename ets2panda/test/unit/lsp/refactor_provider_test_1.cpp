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
#include <algorithm>
#include <string>
#include <vector>
#include "lsp/include/api.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp_api_test.h"

namespace {

using ark::es2panda::lsp::ApplicableRefactorInfo;
using ark::es2panda::lsp::Initializer;

constexpr std::string_view EXTRACT_SYMBOL_REFACTOR = "ExtractSymbolRefactor";
constexpr std::string_view EXTRACT_SYMBOL_DESC = "Extract Symbol";
constexpr std::string_view EXTRACT_TYPE_REFACTOR = "ExtractTypeRefactor";
constexpr std::string_view EXTRACT_TYPE_DESC = "Extract selected type";
constexpr std::string_view MOVE_TO_NEW_FILE_REFACTOR = "Move to a new file";
constexpr std::string_view GENERATE_CONSTRUCTOR_REFACTOR = "Generate Constructor";
constexpr std::string_view GENERATE_GETTERS_SETTERS_REFACTOR = "GenerateGettersAndSettersRefactor";
constexpr std::string_view GENERATE_GETTERS_SETTERS_DESC = "Generate getters and setters";
constexpr std::string_view CONVERT_TEMPLATE_REFACTOR = "Convert to template string";
constexpr std::string_view INFER_RETURN_TYPE_REFACTOR = "Infer function return type";
constexpr std::string_view CONVERT_PARAMS_TO_OBJECT_REFACTOR = "Convert parameters to object and introduce interface";

constexpr std::string_view ACTION_EXTRACT_FUNCTION_GLOBAL = "extract_function_scope_2";
constexpr std::string_view ACTION_EXTRACT_FUNCTION_CLASS = "extract_function_scope_1";
constexpr std::string_view ACTION_EXTRACT_VARIABLE_ENCLOSE = "extract_variable_scope_0";
constexpr std::string_view ACTION_EXTRACT_CONSTANT_ENCLOSE = "extract_constant_scope_0";
constexpr std::string_view ACTION_EXTRACT_TYPE = "extract_type";
constexpr std::string_view KIND_EXTRACT_FUNCTION = "refactor.extract.function";
constexpr std::string_view KIND_EXTRACT_VARIABLE = "refactor.extract.variable";
constexpr std::string_view KIND_EXTRACT_CONSTANT = "refactor.extract.constant";
constexpr std::string_view KIND_EXTRACT_TYPE = "refactor.extract.type";
constexpr std::string_view KIND_MOVE_TO_NEW_FILE = "refactor.move.newFile";
constexpr std::string_view KIND_GENERATE_CONSTRUCTOR = "refactor.rewrite.property.generateConstructor";

class LspRefactorProviderApplicableTests : public LSPAPITests {};

size_t FindToken(const std::string &source, const std::string &token, size_t from = 0)
{
    const size_t pos = source.find(token, from);
    EXPECT_NE(pos, std::string::npos) << "marker not found: " << token;
    return pos;
}

bool HasAction(const std::vector<ApplicableRefactorInfo> &infos, std::string_view name, std::string_view actionName)
{
    return std::any_of(infos.begin(), infos.end(), [&](const ApplicableRefactorInfo &info) {
        return info.name == name && info.action.name == actionName;
    });
}

bool HasActionKind(const std::vector<ApplicableRefactorInfo> &infos, std::string_view kindPrefix)
{
    return std::any_of(infos.begin(), infos.end(), [&](const ApplicableRefactorInfo &info) {
        return info.action.kind.rfind(std::string(kindPrefix), 0) == 0;
    });
}

TEST_F(LspRefactorProviderApplicableTests, ExpressionSelectionActionSet)
{
    const std::string source = R"(
const pi = 3.14;
function area(radius: number): number {
    const value = pi * radius * radius;
    return value;
}
)";

    Initializer initializer;
    auto *context = initializer.CreateContext("applicable_expr.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    const std::string expr = "pi * radius * radius";
    const size_t start = FindToken(source, expr);
    auto infos = lspApi->getApplicableRefactors(context, "", start, start + expr.size());

    initializer.DestroyContext(context);

    ASSERT_EQ(infos.size(), 6U);
    EXPECT_TRUE(HasAction(infos, EXTRACT_SYMBOL_REFACTOR, ACTION_EXTRACT_FUNCTION_GLOBAL));
    EXPECT_TRUE(HasAction(infos, EXTRACT_SYMBOL_REFACTOR, ACTION_EXTRACT_VARIABLE_ENCLOSE));
    EXPECT_TRUE(HasAction(infos, EXTRACT_SYMBOL_REFACTOR, ACTION_EXTRACT_CONSTANT_ENCLOSE));
    EXPECT_TRUE(HasAction(infos, INFER_RETURN_TYPE_REFACTOR, INFER_RETURN_TYPE_REFACTOR));
    EXPECT_TRUE(HasAction(infos, CONVERT_PARAMS_TO_OBJECT_REFACTOR, CONVERT_PARAMS_TO_OBJECT_REFACTOR));
    EXPECT_TRUE(HasAction(infos, CONVERT_TEMPLATE_REFACTOR, CONVERT_TEMPLATE_REFACTOR));
    for (const auto &info : infos) {
        if (info.name == EXTRACT_SYMBOL_REFACTOR) {
            EXPECT_EQ(info.description, EXTRACT_SYMBOL_DESC);
            EXPECT_TRUE(info.action.kind == KIND_EXTRACT_FUNCTION || info.action.kind == KIND_EXTRACT_VARIABLE ||
                        info.action.kind == KIND_EXTRACT_CONSTANT)
                << "unexpected extract kind: " << info.action.kind;
        }
    }
}

TEST_F(LspRefactorProviderApplicableTests, StatementSelectionActionSet)
{
    const std::string source = R"(
function compute(a: number, b: number): number {
    const sum = a + b;
    return sum;
}
)";

    Initializer initializer;
    auto *context = initializer.CreateContext("applicable_stmt.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    const size_t start = FindToken(source, "const sum");
    const size_t end = FindToken(source, "return sum;");
    auto infos = lspApi->getApplicableRefactors(context, "", start, end);

    initializer.DestroyContext(context);

    ASSERT_EQ(infos.size(), 5U);
    EXPECT_TRUE(HasAction(infos, EXTRACT_SYMBOL_REFACTOR, ACTION_EXTRACT_FUNCTION_GLOBAL));
    EXPECT_TRUE(HasAction(infos, EXTRACT_SYMBOL_REFACTOR, ACTION_EXTRACT_VARIABLE_ENCLOSE));
    EXPECT_TRUE(HasAction(infos, EXTRACT_SYMBOL_REFACTOR, ACTION_EXTRACT_CONSTANT_ENCLOSE));
    EXPECT_TRUE(HasAction(infos, EXTRACT_TYPE_REFACTOR, ACTION_EXTRACT_TYPE));
    EXPECT_TRUE(HasAction(infos, CONVERT_PARAMS_TO_OBJECT_REFACTOR, CONVERT_PARAMS_TO_OBJECT_REFACTOR));
    for (const auto &info : infos) {
        if (info.name == EXTRACT_TYPE_REFACTOR) {
            EXPECT_EQ(info.description, EXTRACT_TYPE_DESC);
            EXPECT_EQ(info.action.kind, KIND_EXTRACT_TYPE);
        }
    }
}

TEST_F(LspRefactorProviderApplicableTests, TypeSelectionActionSet)
{
    const std::string source = R"(
let pair: [number, string] = [1, "one"];
)";

    Initializer initializer;
    auto *context = initializer.CreateContext("applicable_type.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    const std::string typeExpr = "[number, string]";
    const size_t start = FindToken(source, typeExpr);
    auto infos = lspApi->getApplicableRefactors(context, "", start, start + typeExpr.size());

    initializer.DestroyContext(context);

    ASSERT_EQ(infos.size(), 2U);
    EXPECT_TRUE(HasAction(infos, EXTRACT_TYPE_REFACTOR, ACTION_EXTRACT_TYPE));
    EXPECT_TRUE(HasAction(infos, GENERATE_GETTERS_SETTERS_REFACTOR, GENERATE_GETTERS_SETTERS_REFACTOR));
    for (const auto &info : infos) {
        if (info.name == EXTRACT_TYPE_REFACTOR) {
            EXPECT_EQ(info.description, EXTRACT_TYPE_DESC);
            EXPECT_EQ(info.action.kind, KIND_EXTRACT_TYPE);
            EXPECT_EQ(info.action.description, "Extract selected type to a type alias");
        }
        if (info.name == GENERATE_GETTERS_SETTERS_REFACTOR) {
            EXPECT_EQ(info.description, GENERATE_GETTERS_SETTERS_DESC);
        }
    }
}

TEST_F(LspRefactorProviderApplicableTests, MemberCursorActionSet)
{
    const std::string source = R"(
class Counter {
    count: number = 0;

    increment(): void {
        this.count = this.count + 1;
    }
}
)";

    Initializer initializer;
    auto *context = initializer.CreateContext("applicable_member.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    const size_t memberPos = FindToken(source, "increment");
    auto infos = lspApi->getApplicableRefactors(context, "", memberPos, memberPos);

    initializer.DestroyContext(context);

    ASSERT_EQ(infos.size(), 2U);
    EXPECT_TRUE(HasAction(infos, MOVE_TO_NEW_FILE_REFACTOR, MOVE_TO_NEW_FILE_REFACTOR));
    EXPECT_TRUE(HasAction(infos, GENERATE_CONSTRUCTOR_REFACTOR, "Generate constructor"));
    for (const auto &info : infos) {
        if (info.name == MOVE_TO_NEW_FILE_REFACTOR) {
            EXPECT_EQ(info.description, MOVE_TO_NEW_FILE_REFACTOR);
            EXPECT_EQ(info.action.kind, KIND_MOVE_TO_NEW_FILE);
        }
        if (info.name == GENERATE_CONSTRUCTOR_REFACTOR) {
            EXPECT_EQ(info.description, GENERATE_CONSTRUCTOR_REFACTOR);
            EXPECT_EQ(info.action.kind, KIND_GENERATE_CONSTRUCTOR);
        }
    }
}

TEST_F(LspRefactorProviderApplicableTests, ClassSelectionActionSet)
{
    const std::string source = R"(
class Counter {
    count: number = 0;
}

class Point {
    x: number = 0;
    y: number = 0;
}
)";

    Initializer initializer;
    auto *context = initializer.CreateContext("applicable_class.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    const size_t classStart = FindToken(source, "class Point");
    auto infos = lspApi->getApplicableRefactors(context, "", classStart, source.size());

    initializer.DestroyContext(context);

    ASSERT_EQ(infos.size(), 4U);
    EXPECT_TRUE(HasAction(infos, EXTRACT_SYMBOL_REFACTOR, ACTION_EXTRACT_FUNCTION_CLASS));
    EXPECT_TRUE(HasAction(infos, EXTRACT_SYMBOL_REFACTOR, ACTION_EXTRACT_FUNCTION_GLOBAL));
    EXPECT_TRUE(HasAction(infos, EXTRACT_TYPE_REFACTOR, ACTION_EXTRACT_TYPE));
    EXPECT_TRUE(HasAction(infos, MOVE_TO_NEW_FILE_REFACTOR, MOVE_TO_NEW_FILE_REFACTOR));
    for (const auto &info : infos) {
        if (info.name == EXTRACT_SYMBOL_REFACTOR) {
            EXPECT_EQ(info.action.kind, KIND_EXTRACT_FUNCTION);
        }
        if (info.name == MOVE_TO_NEW_FILE_REFACTOR) {
            EXPECT_EQ(info.action.kind, KIND_MOVE_TO_NEW_FILE);
        }
    }
}

TEST_F(LspRefactorProviderApplicableTests, CommentSelectionReturnsNothing)
{
    const std::string source = R"(
// comment with pi * radius
const value = 1;
)";

    Initializer initializer;
    auto *context = initializer.CreateContext("applicable_comment.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    const std::string commentExpr = "pi * radius";
    const size_t start = FindToken(source, commentExpr);
    auto infos = lspApi->getApplicableRefactors(context, "", start, start + commentExpr.size());

    initializer.DestroyContext(context);

    EXPECT_TRUE(infos.empty());
}

TEST_F(LspRefactorProviderApplicableTests, StringSelectionHasNoExtractOrStringRewrite)
{
    const std::string source = R"(
const text = "pi * radius in string";
)";

    Initializer initializer;
    auto *context = initializer.CreateContext("applicable_string.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    const std::string inner = "pi * radius";
    const size_t start = FindToken(source, inner);
    auto infos = lspApi->getApplicableRefactors(context, "", start, start + inner.size());

    initializer.DestroyContext(context);

    // The selection targets the contents of a string literal. The only
    // extract actions offered are whole-literal extractions to the global
    // scope (they extract the string literal expression itself); no
    // enclosing-scope extraction and no string-rewrite action is produced.
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr std::string_view actionExtractVariableGlobal = "extract_variable_scope_2";
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr std::string_view actionExtractConstantGlobal = "extract_constant_scope_2";
    ASSERT_EQ(infos.size(), 4U);
    EXPECT_TRUE(HasAction(infos, EXTRACT_SYMBOL_REFACTOR, ACTION_EXTRACT_FUNCTION_GLOBAL));
    EXPECT_TRUE(HasAction(infos, EXTRACT_SYMBOL_REFACTOR, actionExtractVariableGlobal));
    EXPECT_TRUE(HasAction(infos, EXTRACT_SYMBOL_REFACTOR, actionExtractConstantGlobal));
    EXPECT_TRUE(HasAction(infos, GENERATE_GETTERS_SETTERS_REFACTOR, GENERATE_GETTERS_SETTERS_REFACTOR));
    for (const auto &info : infos) {
        if (info.name == EXTRACT_SYMBOL_REFACTOR) {
            EXPECT_EQ(info.description, EXTRACT_SYMBOL_DESC);
            EXPECT_TRUE(info.action.kind == KIND_EXTRACT_FUNCTION || info.action.kind == KIND_EXTRACT_VARIABLE ||
                        info.action.kind == KIND_EXTRACT_CONSTANT)
                << "unexpected extract kind inside string: " << info.action.kind;
        }
    }
    EXPECT_FALSE(HasActionKind(infos, "refactor.rewrite.string"));
    EXPECT_FALSE(HasActionKind(infos, KIND_EXTRACT_TYPE));
}

TEST_F(LspRefactorProviderApplicableTests, EmptySelectionHasNoExtractActions)
{
    const std::string source = R"(
const value = 1 + 2;
)";

    Initializer initializer;
    auto *context = initializer.CreateContext("applicable_empty.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    const size_t pos = FindToken(source, "1 + 2");
    auto infos = lspApi->getApplicableRefactors(context, "", pos, pos);

    initializer.DestroyContext(context);

    EXPECT_FALSE(HasActionKind(infos, "refactor.extract"));
    EXPECT_FALSE(HasActionKind(infos, KIND_MOVE_TO_NEW_FILE));
}

TEST_F(LspRefactorProviderApplicableTests, InvalidSelectionHasNoExtractActions)
{
    const std::string source = R"(
const value = 1 + 2;
)";

    Initializer initializer;
    auto *context = initializer.CreateContext("applicable_invalid.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    const size_t start = FindToken(source, "1 + 2");
    const size_t end = start + std::string("1 + 2").size();
    // end before start is an invalid selection
    auto infos = lspApi->getApplicableRefactors(context, "", end, start);

    initializer.DestroyContext(context);

    EXPECT_FALSE(HasActionKind(infos, "refactor.extract"));
    EXPECT_FALSE(HasActionKind(infos, KIND_MOVE_TO_NEW_FILE));
}

TEST_F(LspRefactorProviderApplicableTests, IncompleteSyntaxDoesNotCrash)
{
    const std::string source = "function broken( {\n    return 1 + ;\n}\n";

    Initializer initializer;
    auto *context = initializer.CreateContext("applicable_incomplete.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto wholeInfos = lspApi->getApplicableRefactors(context, "", 0, source.size());
    const size_t mid = FindToken(source, "return");
    auto midInfos = lspApi->getApplicableRefactors(context, "", mid, source.size());
    auto emptyInfos = lspApi->getApplicableRefactors(context, "", mid, mid);

    initializer.DestroyContext(context);

    // The parser recovers from the incomplete function; queries must not
    // crash and must not report extract-function actions for the broken body.
    EXPECT_FALSE(HasActionKind(wholeInfos, KIND_EXTRACT_FUNCTION));
    EXPECT_FALSE(HasActionKind(midInfos, KIND_EXTRACT_FUNCTION));
    EXPECT_FALSE(HasActionKind(emptyInfos, KIND_EXTRACT_FUNCTION));
    // The cursor sits on the recovered 'return' statement; the offered
    // actions come from statement-level refactors (extract-type and
    // move-to-new-file), none of which is a broken-function extraction.
    ASSERT_EQ(midInfos.size(), 2U);
    EXPECT_TRUE(HasAction(midInfos, EXTRACT_TYPE_REFACTOR, ACTION_EXTRACT_TYPE));
    EXPECT_TRUE(HasAction(midInfos, MOVE_TO_NEW_FILE_REFACTOR, MOVE_TO_NEW_FILE_REFACTOR));
    ASSERT_EQ(emptyInfos.size(), 1U);
    EXPECT_EQ(emptyInfos[0].name, MOVE_TO_NEW_FILE_REFACTOR);
    EXPECT_EQ(emptyInfos[0].action.kind, KIND_MOVE_TO_NEW_FILE);
}

TEST_F(LspRefactorProviderApplicableTests, KindFilterNarrowsResult)
{
    const std::string source = R"(
class Point {
    x: number = 0;
    y: number = 0;
}
)";

    Initializer initializer;
    auto *context = initializer.CreateContext("applicable_kind.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(context, nullptr);

    LSPAPI const *lspApi = GetImpl();
    const size_t classStart = FindToken(source, "class Point");

    auto allInfos = lspApi->getApplicableRefactors(context, "", classStart, source.size());
    auto moveInfos = lspApi->getApplicableRefactors(context, "refactor.move.newFile", classStart, source.size());
    auto unknownInfos = lspApi->getApplicableRefactors(context, "refactor.nonexistent.kind", classStart, source.size());

    initializer.DestroyContext(context);

    ASSERT_EQ(allInfos.size(), 4U);

    ASSERT_EQ(moveInfos.size(), 1U);
    EXPECT_EQ(moveInfos[0].name, MOVE_TO_NEW_FILE_REFACTOR);
    EXPECT_EQ(moveInfos[0].description, MOVE_TO_NEW_FILE_REFACTOR);
    EXPECT_EQ(moveInfos[0].action.name, MOVE_TO_NEW_FILE_REFACTOR);
    EXPECT_EQ(moveInfos[0].action.kind, KIND_MOVE_TO_NEW_FILE);

    EXPECT_TRUE(unknownInfos.empty());
}

}  // namespace
