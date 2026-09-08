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
#include <cctype>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
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

std::string ApplyEditsExactly(const std::string &original, const std::vector<::TextChange> &edits)
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
        start = std::max(start, cursor);
        result.append(original, cursor, start - cursor);
        result.append(change->newText);
        cursor = std::min(start + change->span.length, original.size());
    }
    result.append(original, cursor, original.size() - cursor);
    return result;
}

class LspExtractSymbolTestsBase : public LSPAPITests {
public:
    ark::es2panda::lsp::RefactorContext *CreateExtractContext(Initializer *initializer, const std::string &code,
                                                              size_t start, size_t end)
    {
        std::vector<std::string> files = {"ExtractSymbolInlineInsertionTest.ets"};
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

    static std::string RefactorName()
    {
        return std::string(ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME);
    }

    static std::string FunctionGlobalAction()
    {
        return std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name);
    }

    bool HasAction(const std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> &applicable,
                   const std::string_view actionName)
    {
        return std::any_of(applicable.begin(), applicable.end(),
                           [&](const auto &info) { return info.action.name == actionName; });
    }

    void ExpectSingleFileEdits(ark::es2panda::lsp::RefactorContext *context, const std::string &actionName,
                               size_t expectedChanges, std::vector<::TextChange> &outChanges,
                               std::optional<size_t> *outRenameLoc = nullptr)
    {
        auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*context, RefactorName(), actionName);
        ASSERT_NE(edits, nullptr);
        ASSERT_EQ(edits->GetFileTextChanges().size(), 1U);
        const auto &fileChanges = edits->GetFileTextChanges().at(0).textChanges;
        ASSERT_EQ(fileChanges.size(), expectedChanges);
        outChanges = fileChanges;
        if (outRenameLoc != nullptr) {
            *outRenameLoc = edits->GetRenameLocation();
        }
    }
};

// --- Suite 1: value/constant extraction, inline insertion, availability rules

class LspExtractSymbolInlineInsertionTests : public LspExtractSymbolTestsBase {};

TEST_F(LspExtractSymbolInlineInsertionTests, ConstantEncloseMergesIntoSecondDeclarator)
{
    const std::string code = "\nfunction main(): Int {\n    let a = 1, b = 2 + 3, c = b * 2;\n    return a + c;\n}\n";
    const std::string target = "2 + 3";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_TRUE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name));

    std::vector<::TextChange> changes;
    std::optional<size_t> renameLoc;
    ExpectSingleFileEdits(refactorContext, std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name), 2U,
                          changes, &renameLoc);
    EXPECT_EQ(changes[0].span.start, code.find("b ="));
    EXPECT_EQ(changes[0].span.length, 0U);
    EXPECT_EQ(changes[0].newText, "newLocal = 2 + 3, ");
    EXPECT_EQ(changes[1].newText, "newLocal");
    EXPECT_TRUE(renameLoc.has_value());
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "\nfunction main(): Int {\n    let a = 1, newLocal = 2 + 3, b = newLocal, c = b * 2;\n"
              "    return a + c;\n}\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, ConstantGlobalTopLevelSecondDeclaratorPlacement)
{
    // Characterization of current behavior: for a top-level multi-declarator statement the global
    // constant extraction prepends the inline insertion at the statement start. The resulting text
    // is not compilable ArkTS (`newLocal = 4 * 5, let ...`); pinned so the regression becomes
    // visible once production adjusts the insertion anchor.
    const std::string code = "\nlet a = 1, b = 4 * 5, c = a;\n";
    const std::string target = "4 * 5";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name), 2U,
                          changes);
    EXPECT_EQ(ApplyEditsExactly(code, changes), "\nnewLocal = 4 * 5, let a = 1, b = newLocal, c = a;\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, PartialBinarySelectionProducesNoFileChanges)
{
    // Error boundary: a partial slice of a binary expression is rejected by the global-constant
    // selection validation, so the public entry returns an empty edit set.
    const std::string code = "\nlet a = 1, b = 4 * 5, c = a;\n";
    const size_t spanStart = code.find("4 *");
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + 3);

    auto emptyEdits = ark::es2panda::lsp::GetEditsForRefactorsImpl(
        *refactorContext, RefactorName(), std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name));
    ASSERT_NE(emptyEdits, nullptr);
    EXPECT_TRUE(emptyEdits->GetFileTextChanges().empty());

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, CallSelectionPullsDeclaredTypeFromDeclarationLine)
{
    // Selecting a full call whose declaration line carries an annotation lets the extraction copy
    // the declared type onto the new constant without consulting checker results.
    const std::string code = "\nfunction main(): void {\n    const cfg: Config = build();\n}\n";
    const std::string target = "build()";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name), 2U,
                          changes);
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "\nfunction main(): void {\n    const newLocal: Config = build();\n"
              "    const cfg: Config = newLocal;\n}\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, ComparisonBinaryYieldsBooleanAnnotation)
{
    const std::string code = "\nfunction main(status: number): void {\n    const ok: boolean = status == 1;\n}\n";
    const std::string target = "status == 1";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name), 2U,
                          changes);
    // Equality/comparison selections are annotated as boolean by the semantic rules.
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "\nfunction main(status: number): void {\n    const newLocal: boolean = status == 1;\n"
              "    const ok: boolean = newLocal;\n}\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, TypeAnnotationSelectionRemovesValueActions)
{
    // Selecting the declarator's type annotation must not offer any extract-value action.
    const std::string code = "\nfunction main(): void {\n    let sized: number = 3;\n    console.log(sized);\n}\n";
    const std::string target = "number";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, WholeCallStatementValueExtractionReplacesStatement)
{
    // Extracting a standalone call statement moves the call into the new constant and deletes the
    // original statement text.
    const std::string code = "\nfunction greet(): void {\n    shout(\"hi\");\n    return;\n}\n";
    const std::string target = "shout(\"hi\");";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name), 2U,
                          changes);
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "\nfunction greet(): void {const newLocal = shout(\"hi\");\n    \n    return;\n}\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, IfTestSelectionDropsConstantActions)
{
    const std::string code =
        "\nfunction classify(n: number): Int {\n    const limit = 10;\n"
        "    if (n > limit) {\n        return 1;\n    }\n    return 0;\n}\n";
    const std::string target = "n > limit";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_TRUE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, WhileTestSelectionDropsConstantActions)
{
    const std::string code =
        "\nfunction spin(limit: number): void {\n    let k: number = 0;\n"
        "    while (k < limit) {\n        k++;\n    }\n}\n";
    const std::string target = "k < limit";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_TRUE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, DoWhileTestSelectionDropsConstantActions)
{
    const std::string code =
        "\nfunction drain(limit: number): void {\n    let k: number = 0;\n"
        "    do {\n        k++;\n    } while (k < limit);\n}\n";
    const std::string target = "k < limit";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_TRUE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, ForUpdateTestSelectionDropsConstantActions)
{
    const std::string code =
        "\nfunction walk(limit: number): void {\n    let s: number = 0;\n"
        "    for (let i: number = 0; i < limit; i++) {\n        s += i;\n    }\n}\n";
    const std::string target = "i < limit";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_TRUE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, IfTestInnerIdentifierKeepsGlobalConstantAction)
{
    // Current behavior: selecting only an identifier inside an if-test drops the enclosing-scope
    // constant action while the global one stays offered.
    const std::string code =
        "\nfunction classify(n: number): Int {\n    const limit = 10;\n"
        "    if (n > limit) {\n        return 1;\n    }\n    return 0;\n}\n";
    const size_t testPos = code.find("if (");
    const size_t spanStart = code.find("limit", testPos);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + 5);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_TRUE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name));
    EXPECT_TRUE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, TopLevelLiteralArgumentHoistsConstantToFileStart)
{
    const std::string code = "\ngreet(\"x\");\nfunction greet(m: string): void {}\n";
    const std::string target = "\"x\"";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name), 2U,
                          changes);
    // The literal argument becomes a top-level constant placed at the very start of the file and
    // the call site references it.
    EXPECT_EQ(changes[0].span.start, 0U);
    EXPECT_EQ(changes[0].newText, "const newLocal: String = \"x\";");
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "const newLocal: String = \"x\";\ngreet(newLocal);\nfunction greet(m: string): void {}\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, MultilineRhsConstantEncloseInsertsAboveStatement)
{
    const std::string code = "\nfunction calc(): Int {\n    const sum = 111 +\n        222;\n    return sum;\n}\n";
    const std::string target = "111 +\n        222";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name), 2U,
                          changes);
    // The whole multi-line RHS moves verbatim into the new constant, inserted directly above the
    // source statement inside the enclosing function.
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "\nfunction calc(): Int {\n    const newLocal: Int = 111 +\n        222;\n"
              "    const sum = newLocal;\n    return sum;\n}\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, ReturnTypeAnnotationSelectionOffersNoExtractActions)
{
    // Selecting inside a function's return-type annotation must not offer any extraction action
    // from this refactor.
    const std::string code = "\nfunction scale(v: number): number {\n    return v;\n}\n";
    const size_t annoPos = code.find(": number {");
    ASSERT_NE(annoPos, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, annoPos + 2, annoPos + 8);

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_CLASS.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_GLOBAL.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, ArgumentSpanSelectionKeepsVariableEncloseAction)
{
    // A selection spanning several call arguments is not an AST expression on its own, but the
    // covering call keeps the enclosing-scope variable action available.
    const std::string code = "\nfunction main(): void {\n    let acc = 0;\n    combine(acc, acc + 1);\n}\n";
    const std::string target = "acc, acc + 1";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_TRUE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, MultilineBinarySelectionOffersNoExtractActions)
{
    // Current behavior: a free-standing multi-line binary selection offers none of this
    // refactor's actions.
    const std::string code =
        "\nfunction main(): Int {\n    const total = 11 +\n        22 + 33;\n    return total;\n}\n";
    const std::string target = "11 +\n        22 + 33";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_GLOBAL.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_VARIABLE_ACTION_ENCLOSE.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_ENCLOSE.name));
    EXPECT_FALSE(HasAction(applicable, ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name));

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolInlineInsertionTests, UnknownActionNameYieldsNullEdits)
{
    const std::string code = "\nfunction main(): void {\n    const value = 1 + 2;\n}\n";
    const std::string target = "1 + 2";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    auto nullEdits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, RefactorName(), "no_such_action");
    EXPECT_EQ(nullEdits, nullptr);

    initializer->DestroyContext(refactorContext->context);
}

// --- Suite 2: function extraction, async/generics/namespace, annotations -----

class LspExtractSymbolFunctionExtractionTests : public LspExtractSymbolTestsBase {};

TEST_F(LspExtractSymbolFunctionExtractionTests, ExtractReturnIntStatementAnnotatesHelper)
{
    const std::string code = "\nfunction answer(): Int {\n    return 42;\n}\n";
    const std::string target = "return 42;";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, FunctionGlobalAction(), 2U, changes);
    // The integer literal return feeds the helper signature and the call site returns its result.
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "\nfunction newFunction(): Int {\n  return 42;\n}\n\nfunction answer(): Int {\n"
              "    return newFunction();\n}\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolFunctionExtractionTests, BareReturnStatementBecomesVoidHelper)
{
    const std::string code = "\nfunction maybe(flag: boolean): void {\n    if (flag) {\n        return;\n    }\n}\n";
    const std::string target = "return;";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, FunctionGlobalAction(), 2U, changes);
    // A bare `return;` keeps the void annotation and still returns the helper result.
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "\nfunction newFunction(): void {\n  return;\n}\n\nfunction maybe(flag: boolean): void {\n"
              "    if (flag) {\n        return newFunction();\n    }\n}\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolFunctionExtractionTests, OuterWriteBecomesCapturedParamAndReturnValue)
{
    const std::string code = "\nfunction accumulate(): void {\n    let total = 0;\n    total = total + 41 + 1;\n}\n";
    const std::string target = "total = total + 41 + 1;";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, FunctionGlobalAction(), 2U, changes);
    // Current behavior: the outer variable read becomes the captured parameter `total: int`,
    // forwarded at the call site. The assignment stays verbatim as the returned expression, so the
    // outer write is currently dropped at the call site; pinned until production decides whether
    // the write must be preserved.
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "\nfunction newFunction(total: int): Int {\n  return total = total + 41 + 1;\n}\n\n"
              "function accumulate(): void {\n    let total = 0;\n    newFunction(total);\n}\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolFunctionExtractionTests, FreeLocalExpressionPinsCurrentNoCaptureBehavior)
{
    const std::string code =
        "\nfunction compute(): Int {\n    const base = 10;\n    const factor = 3;\n"
        "    const scaled = base * factor;\n    return scaled;\n}\n";
    const std::string target = "base * factor";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, FunctionGlobalAction(), 2U, changes);
    const std::string applied = ApplyEditsExactly(code, changes);
    // Characterization of current behavior: expression-mode extraction does not thread free locals
    // as parameters (the call site has no arguments) and the unresolved checker type is emitted as
    // the literal `*ERROR_TYPE*` annotation. Update this pin once capture/type resolution lands.
    EXPECT_NE(applied.find("*ERROR_TYPE*"), std::string::npos);
    EXPECT_NE(applied.find("  return base * factor;"), std::string::npos);
    EXPECT_NE(applied.find("const scaled = newFunction();"), std::string::npos);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolFunctionExtractionTests, GenericEnclosingTypeParamsThreadThroughHelperAndCall)
{
    const std::string code =
        "\nfunction chooseFirst<T>(a: T, b: T): T {\n    const picked = a == b ? a : b;\n    return picked;\n}\n";
    const std::string target = "a == b ? a : b";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, FunctionGlobalAction(), 2U, changes);
    // The enclosing `<T>` parameter is carried onto the helper signature and its call, and the free
    // locals become plain captured arguments.
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "\nfunction newFunction<T>(a: T, b: T): T {\n  return a == b ? a : b;\n}\n\n"
              "function chooseFirst<T>(a: T, b: T): T {\n    const picked = newFunction<T>(a, b);\n"
              "    return picked;\n}\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolFunctionExtractionTests, NamespaceQualifiedCallRewrittenWhenMovedToGlobal)
{
    const std::string code =
        "\nnamespace K {\n    export type W = number;\n    export function produce(): W {\n        return 1;\n"
        "    }\n    function gen(): W {\n        const out: W = produce();\n        return out;\n    }\n}\n";
    const std::string target = "produce()";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, FunctionGlobalAction(), 2U, changes);
    const std::string applied = ApplyEditsExactly(code, changes);
    // Moving the selection to the global scope qualifies the namespace-local callee so the helper
    // keeps resolving (`K.produce()`); the helper itself lands before the namespace block.
    EXPECT_NE(applied.find("\nfunction newFunction()"), std::string::npos);
    EXPECT_NE(applied.find("return K.produce();"), std::string::npos);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolFunctionExtractionTests, NamespaceEncloseInsertsIndentedHelperInsideNamespace)
{
    const std::string code =
        "\nnamespace M {\n    export type Q = number;\n    function worker(v: Q): Q {\n"
        "        const doubled = v + v;\n        return doubled;\n    }\n}\n";
    const std::string target = "v + v";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, std::string(ark::es2panda::lsp::EXTRACT_FUNCTION_ACTION_ENCLOSE.name), 2U,
                          changes);
    // The enclosing-namespace action keeps the helper inside `namespace M`, indented to match the
    // surrounding members and forwarding the namespace-typed parameter.
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "\nnamespace M {\n    export type Q = number;\n"
              "    function newFunction(v: Q): Double {\n      return v + v;\n    }\n\n"
              "    function worker(v: Q): Q {\n        const doubled = newFunction(v);\n"
              "        return doubled;\n    }\n}\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolFunctionExtractionTests, ClassScopeConstantBecomesPrivateReadonlyProperty)
{
    const std::string code =
        "\nclass Holder {\n    compute(base: number): number {\n"
        "        const k = base * 3;\n        return k;\n    }\n}\n";
    const std::string target = "base * 3";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_CLASS.name), 2U,
                          changes);
    const std::string applied = ApplyEditsExactly(code, changes);
    // The class-scope constant action creates a private readonly property referenced through
    // `this.` at the extraction site. Current behavior concatenates the property directly after
    // the class-opening line without separator newlines (pin).
    EXPECT_NE(applied.find("private readonly newProperty"), std::string::npos);
    EXPECT_NE(applied.find("= base * 3;"), std::string::npos);
    EXPECT_NE(applied.find("const k = this.newProperty;"), std::string::npos);

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolFunctionExtractionTests, AsyncStatementsWithoutReturnProducePromiseVoidHelper)
{
    const std::string code = "\nasync function run(): Promise<void> {\n    await stepOne();\n    await stepTwo();\n}\n";
    const std::string target = "await stepOne();\n    await stepTwo();";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, FunctionGlobalAction(), 2U, changes);
    // Await inside the range forces the async helper; without a resolvable value the return type
    // defaults to Promise<void>. Current behavior for the global action appends the helper after
    // the source function body.
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "\nasync function run(): Promise<void> {\n    newFunction();\n}\n"
              "async function newFunction(): Promise<void> {\n  await stepOne();\n  await stepTwo();\n}\n\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolFunctionExtractionTests, AwaitExpressionExtractionKeepsPromiseSignature)
{
    const std::string code =
        "\nasync function loadNumber(): Promise<number> {\n    return 3;\n}\n\n"
        "async function consume(): Promise<number> {\n    const v = await loadNumber();\n"
        "    return v;\n}\n";
    const std::string target = "await loadNumber()";
    const size_t spanStart = code.find(target, code.find("consume"));
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, FunctionGlobalAction(), 2U, changes);
    // The awaited call keeps the async wrapper and re-awaits inside the helper; the observed
    // annotation uses the checker's rendered element type.
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "\nasync function loadNumber(): Promise<number> {\n    return 3;\n}\n\n"
              "async function newFunction(): Promise<Double> {\n  return await loadNumber();\n}\n\n"
              "async function consume(): Promise<number> {\n    const v = newFunction();\n    return v;\n}\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolFunctionExtractionTests, MultilineExpressionIsRewrittenAsReturnedBody)
{
    const std::string code = "\nfunction calc(): Int {\n    const sum = 111 +\n        222;\n    return sum;\n}\n";
    const std::string target = "111 +\n        222";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, FunctionGlobalAction(), 2U, changes);
    // Current behavior: the multiline selection becomes a `return <expr>;` body and the generated
    // helper lands directly after the replaced statement line inside the source function.
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "\nfunction calc(): Int {\n    const sum = newFunction();\n\n"
              "function newFunction(): Int {\n  return 111 +\n        222;\n}\n\n    return sum;\n}\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolFunctionExtractionTests, DeclLeadingLiteralLetBecomesTypedHelper)
{
    const std::string code = "\nlet port = 8080;\n\nfunction main(): Int {\n    return port;\n}\n";
    const std::string selection = "let port = 8080;\n\n";
    const size_t spanStart = code.find("let port");
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + selection.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, FunctionGlobalAction(), 2U, changes);
    // Selecting the whole declaration statement extracts it into a helper that returns the bound
    // name; the declaration site collapses to the invocation.
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "\nfunction newFunction(): Int {\n  return 8080;\n}\n\nlet port = newFunction();\n\n"
              "function main(): Int {\n    return port;\n}\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolFunctionExtractionTests, DeclLeadingStringLiteralPinsCurrentAnnotation)
{
    const std::string code = "\nlet title = \"demo\";\n\nfunction main(): string {\n    return title;\n}\n";
    const size_t spanStart = code.find("let title");
    ASSERT_NE(spanStart, std::string::npos);
    const size_t spanEnd = code.find("\"demo\";") + 8;

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanEnd);

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, FunctionGlobalAction(), 2U, changes);
    // Characterization of current behavior: the inferred annotation mirrors the initializer text
    // itself (`: "demo"`), not a primitive type name; pinned until type naming is aligned.
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "\nfunction newFunction(): \"demo\" {\n  return \"demo\";\n}\n\n"
              "let title = newFunction();\n\nfunction main(): string {\n    return title;\n}\n");

    initializer->DestroyContext(refactorContext->context);
}

TEST_F(LspExtractSymbolFunctionExtractionTests, MemberExpressionPlaceholderPinsMidLineConcatenation)
{
    // Characterization of current behavior: extracting a member-expression inserts the new
    // constant directly after the previous line without separator newlines (`);const ...`),
    // and the annotation falls back to `*ERROR_TYPE*`.
    const std::string code = "\nlet wrapper = new Wrapper();\nlet deep = wrapper.field;\nfunction touch(): void {}\n";
    const std::string target = "wrapper.field";
    const size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    auto *refactorContext = CreateExtractContext(initializer.get(), code, spanStart, spanStart + target.size());

    std::vector<::TextChange> changes;
    ExpectSingleFileEdits(refactorContext, std::string(ark::es2panda::lsp::EXTRACT_CONSTANT_ACTION_GLOBAL.name), 2U,
                          changes);
    EXPECT_EQ(ApplyEditsExactly(code, changes),
              "\nlet wrapper = new Wrapper();const newLocal: *ERROR_TYPE* = wrapper.field;\n"
              "let deep = newLocal;\nfunction touch(): void {}\n");

    initializer->DestroyContext(refactorContext->context);
}

}  // namespace
