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

class ExtractSymbolSelectionShapesTests : public LSPAPITests {
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

    static bool HasAction(const std::vector<ApplicableRefactorInfo> &applicable, const std::string &actionName)
    {
        return std::any_of(applicable.begin(), applicable.end(),
                           [&](const ApplicableRefactorInfo &info) { return info.action.name == actionName; });
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
        ASSERT_LT(renameLocation, tokenLocation + renameToken.size());
    }
};

// =============================================================================
// PE-018 TrimSlashWrappedCommentBoundary (extract_symbol.cpp:440-455) +
// GetTrimmedSelectionSpan (L457-486) and the block-comment trim helpers
// TrimLeadingBlockComment (L411) / TrimTrailingBlockComment (L425).
// GetTrimmedSelectionSpan normalizes every extraction selection before use, so
// each shape below is exercised through the public refactor API.
// =============================================================================

// PE-018: selection "/1/" wrapping the commented number across the comment
// boundaries of the assignment: TrimSlashWrappedCommentBoundary ++start arm
// (L443-446), --end arm (L447-450) and the changed whitespace retrim
// (L451-453). Both selections produce the same stable extraction shape.
TEST_F(ExtractSymbolSelectionShapesTests, SlashWrappedSelectionNormalizesToExpression)
{
    const std::string code = "let v = /*start*/1/*end*/;\nfunction use(): int { return v; }\n";
    const std::string slashWrapped = "/1/";
    const size_t wrappedStart = code.find(slashWrapped);
    ASSERT_NE(wrappedStart, std::string::npos);
    const std::string plain = "1";
    const size_t plainStart = code.find(plain);
    ASSERT_NE(plainStart, std::string::npos);

    Initializer initializer;
    auto *wrappedContext =
        MakeContext(&initializer, "SlashWrapped.ets", code, wrappedStart, wrappedStart + slashWrapped.size());
    ASSERT_NE(wrappedContext, nullptr);
    auto *plainContext = MakeContext(&initializer, "SlashPlain.ets", code, plainStart, plainStart + plain.size());
    ASSERT_NE(plainContext, nullptr);

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME);
    auto wrappedApplicable = GetApplicableRefactorsImpl(wrappedContext);
    auto plainApplicable = GetApplicableRefactorsImpl(plainContext);
    ASSERT_TRUE(HasAction(wrappedApplicable, "extract_variable_scope_2"));
    ASSERT_TRUE(HasAction(plainApplicable, "extract_variable_scope_2"));

    auto wrappedEdits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*wrappedContext, refactorName, "extract_variable_scope_2");
    auto plainEdits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*plainContext, refactorName, "extract_variable_scope_2");
    ASSERT_NE(wrappedEdits, nullptr);
    ASSERT_NE(plainEdits, nullptr);
    const std::vector<std::string> markers = {"let newLocal: Int = 1;", "/*start*/newLocal/*end*/",
                                              "function use(): int"};
    ExpectSemanticEdits(wrappedEdits.get(), code, markers, "/*start*/", "newLocal");
    ExpectSemanticEdits(plainEdits.get(), code, markers, "/*start*/", "newLocal");

    initializer.DestroyContext(wrappedContext->context);
    initializer.DestroyContext(plainContext->context);
}

// PE-018: selection starting inside a block comment opener whose closer lies
// beyond the selection -> TrimLeadingBlockComment "closer not found inside
// selection" arm (extract_symbol.cpp:417, commentEnd + 2 > end) returns false
// and the selection keeps the comment text.
TEST_F(ExtractSymbolSelectionShapesTests, LeadingCommentWithoutCloserInsideSelectionStays)
{
    const std::string code = "let v = /*comment tail*/ 5;\nfunction use(): int { return v; }\n";
    // Select from the '/' of the opener up to (excluding) the closing '*'.
    const size_t openPos = code.find("/*");
    ASSERT_NE(openPos, std::string::npos);
    const size_t closerStar = code.find("*/", openPos);
    ASSERT_NE(closerStar, std::string::npos);
    Initializer initializer;
    auto *refactorContext = MakeContext(&initializer, "LeadingNoCloser.ets", code, openPos, closerStar);
    ASSERT_NE(refactorContext, nullptr);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    // The untrimmed span still normalizes onto the surrounding initializer via
    // the touch-token fallback, so the extraction stays applicable.
    ASSERT_TRUE(HasAction(applicable, "extract_variable_scope_2"));

    initializer.DestroyContext(refactorContext->context);
}

// PE-018: '/*' appearing inside a string literal has no closing '*/' anywhere ->
// TrimLeadingBlockComment npos arm (extract_symbol.cpp:416-418) returns false.
TEST_F(ExtractSymbolSelectionShapesTests, StringLiteralCommentOpenerNeverTrims)
{
    const std::string code = "let s = \"/*not a comment\";\nfunction use(): String { return s; }\n";
    const size_t openPos = code.find("/*");
    ASSERT_NE(openPos, std::string::npos);
    // Select from the '/' inside the string literal through the closing quote.
    const size_t selectLen = code.find('"', openPos) + 1 - openPos;
    Initializer initializer;
    auto *refactorContext = MakeContext(&initializer, "StringOpener.ets", code, openPos, openPos + selectLen);
    ASSERT_NE(refactorContext, nullptr);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasAction(applicable, "extract_variable_scope_2"));

    initializer.DestroyContext(refactorContext->context);
}

// PE-018: selection ending at a '*/' whose '/*' lies before the selection start
// -> TrimTrailingBlockComment "commentStart < start" arm (extract_symbol.cpp
// :432-433) returns false.
TEST_F(ExtractSymbolSelectionShapesTests, TrailingCloserBeforeSelectionStartStays)
{
    const std::string code = "let v = 1; /* note */\nfunction use(): int { return v; }\n";
    // Select the comment interior "note */": the '/*' opener is before the start.
    const size_t notePos = code.find("note");
    ASSERT_NE(notePos, std::string::npos);
    const size_t endPos = code.find("*/", notePos);
    ASSERT_NE(endPos, std::string::npos);
    const size_t selectEnd = endPos + 2;
    Initializer initializer;
    auto *refactorContext = MakeContext(&initializer, "TrailingCloser.ets", code, notePos, selectEnd);
    ASSERT_NE(refactorContext, nullptr);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_TRUE(applicable.empty()) << "comment-interior selection offers no extraction";

    initializer.DestroyContext(refactorContext->context);
}

// PE-018: comment-only selection trims to an empty span -> GetTrimmedSelectionSpan
// start == end arm (extract_symbol.cpp:482-484) returns the original span and
// no extraction is offered.
TEST_F(ExtractSymbolSelectionShapesTests, CommentOnlySelectionOffersNothing)
{
    const std::string code = "/* header */\nlet v = 1;\nfunction use(): int { return v; }\n";
    const size_t commentStart = code.find("/*");
    ASSERT_NE(commentStart, std::string::npos);
    const size_t commentEnd = code.find("*/", commentStart);
    ASSERT_NE(commentEnd, std::string::npos);
    const size_t selectEnd = commentEnd + 2;
    Initializer initializer;
    auto *refactorContext = MakeContext(&initializer, "CommentOnly.ets", code, commentStart, selectEnd);
    ASSERT_NE(refactorContext, nullptr);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    // The trimmed span degenerates to the original span, so Extract Symbol
    // offers no action while the unrelated move-to-new-file refactor remains.
    ASSERT_FALSE(applicable.empty());
    EXPECT_TRUE(HasAction(applicable, "Move to a new file"));
    EXPECT_FALSE(HasAction(applicable, "extract_variable_scope_2"));
    EXPECT_FALSE(HasAction(applicable, "extract_constant_scope_2"));
    EXPECT_FALSE(HasAction(applicable, "extract_function_scope_2"));

    initializer.DestroyContext(refactorContext->context);
}

// =============================================================================
// PE-019 zero-hit re-evaluation probe: IsNodeInParamList (extract_symbol.cpp
// :4422) + the GetParamsText loop (L4437-4446). The only filler of
// FunctionExtraction::parameters is CollectFunctionParameters (L4399) via
// candidates.front() of GetPossibleFunctionExtractions (L4369-4396); the front
// candidate is the first statement in AST pre-order. This probe selects an
// extraction in a file whose first top-level statement is a function
// declaration, the only shape that could reach the param-list membership arms.
// =============================================================================

// PE-019 probe: extraction behind a leading top-level function declaration.
TEST_F(ExtractSymbolSelectionShapesTests, ParamsTextProbeFirstStatementIsFunctionDeclaration)
{
    const std::string code =
        "function first(a: int): int {\n    return a;\n}\nfunction use(): int {\n    let v = 2 * 8;\n    return "
        "v;\n}\n";
    const std::string target = "2 * 8";
    const size_t start = code.find(target);
    ASSERT_NE(start, std::string::npos);
    Initializer initializer;
    auto *refactorContext = MakeContext(&initializer, "ParamsProbe.ets", code, start, start + target.size());
    ASSERT_NE(refactorContext, nullptr);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_TRUE(HasAction(applicable, "extract_function_scope_2"));

    const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, "extract_function_scope_2");
    ASSERT_NE(edits, nullptr);
    ExpectSemanticEdits(edits.get(), code,
                        {"function first(a: int): int {", "function newFunction(): Int {", "return 2 * 8;",
                         "let v = newFunction();", "return v;"},
                        "let v = newFunction", "newFunction");

    initializer.DestroyContext(refactorContext->context);
}
}  // namespace
