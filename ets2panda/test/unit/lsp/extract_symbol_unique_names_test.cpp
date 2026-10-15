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
#include <cctype>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <string_view>
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

// =============================================================================
// Unique-name scenarios. Every source declares a name that collides with a
// default extraction name, so the generated declaration and replacement must
// use the next free suffix.
// =============================================================================

class ExtractSymbolUniqueNamesTests : public LSPAPITests {
public:
    struct UniqueNameExtractionScenario {
        std::string code;
        std::string target;
        std::string fileName;
        std::string actionName;
        std::string_view refactorName;
        std::string generatedName;
        std::vector<std::string> semanticMarkers;
    };

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

    static std::vector<::TextChange> AllTextChanges(const RefactorEditInfo &edits)
    {
        std::vector<::TextChange> changes;
        for (const auto &fileChange : edits.GetFileTextChanges()) {
            changes.insert(changes.end(), fileChange.textChanges.begin(), fileChange.textChanges.end());
        }
        return changes;
    }

    static bool IsIdentifierCharacter(char character)
    {
        return character == '_' || std::isalnum(static_cast<unsigned char>(character)) != 0;
    }

    static bool ContainsIdentifierToken(const std::string &text, const std::string &token)
    {
        size_t position = text.find(token);
        while (position != std::string::npos) {
            const bool hasLeftBoundary = position == 0 || !IsIdentifierCharacter(text[position - 1]);
            const size_t afterToken = position + token.size();
            const bool hasRightBoundary = afterToken >= text.size() || !IsIdentifierCharacter(text[afterToken]);
            if (hasLeftBoundary && hasRightBoundary) {
                return true;
            }
            position = text.find(token, position + token.size());
        }
        return false;
    }

    static std::string WithoutWhitespace(const std::string &text)
    {
        std::string result;
        result.reserve(text.size());
        for (const unsigned char character : text) {
            if (std::isspace(character) == 0) {
                result.push_back(static_cast<char>(character));
            }
        }
        return result;
    }

    static bool ContainsSemanticMarker(const std::string &text, const std::string &marker)
    {
        return WithoutWhitespace(text).find(WithoutWhitespace(marker)) != std::string::npos;
    }

    void ExpectSemanticExtraction(const UniqueNameExtractionScenario &scenario)
    {
        const size_t start = scenario.code.find(scenario.target);
        ASSERT_NE(start, std::string::npos);
        Initializer initializer;
        auto *refactorContext =
            MakeContext(&initializer, scenario.fileName, scenario.code, start, start + scenario.target.size());
        ASSERT_NE(refactorContext, nullptr);

        auto applicable = GetApplicableRefactorsImpl(refactorContext);
        ASSERT_TRUE(HasAction(applicable, scenario.actionName));

        const std::string refactorName(scenario.refactorName);
        auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, scenario.actionName);
        ASSERT_NE(edits, nullptr);
        ASSERT_FALSE(edits->GetFileTextChanges().empty());
        const auto changes = AllTextChanges(*edits);
        ASSERT_FALSE(changes.empty());

        const std::string applied = ApplyEdits(scenario.code, changes);
        const std::string message = "missing generated identifier: " + scenario.generatedName;
        ASSERT_TRUE(ContainsIdentifierToken(applied, scenario.generatedName)) << message;
        for (const auto &marker : scenario.semanticMarkers) {
            EXPECT_TRUE(ContainsSemanticMarker(applied, marker)) << "missing semantic marker: " << marker;
        }

        initializer.DestroyContext(refactorContext->context);
    }
};

// A top-level collision makes global variable extraction select the next suffix.
TEST_F(ExtractSymbolUniqueNamesTests, UniqueGlobalVarNameCollidesWithTopLevelLet)
{
    ExpectSemanticExtraction({
        "let newLocal = 1;\nconst g = 11 + 13;\nfunction use(): int { return g; }\n",
        "11 + 13",
        "UniqueGlobalVar.ets",
        "extract_variable_scope_2",
        ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME,
        "newLocal_1",
        {
            "let newLocal_1: Int = 11 + 13;",
            "const g = newLocal_1;",
            "let newLocal = 1;",
        },
    });
}

// A local collision makes enclosing-scope variable extraction select the next suffix.
TEST_F(ExtractSymbolUniqueNamesTests, UniqueEncloseVarNameCollidesWithLocalLet)
{
    ExpectSemanticExtraction({
        R"(
function f(): int {
    let newLocal = 4;
    let r = 3 * 5;
    return r + newLocal;
}
)",
        "3 * 5",
        "UniqueEncloseVar.ets",
        "extract_variable_scope_0",
        ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME,
        "newLocal_1",
        {
            "let newLocal_1: Int = 3 * 5;",
            "let r = newLocal_1;",
            "return r + newLocal;",
        },
    });
}

// A class-property collision keeps the generated property distinct and retains member access syntax.
TEST_F(ExtractSymbolUniqueNamesTests, UniqueClassPropertyNameCollidesWithExistingProperty)
{
    ExpectSemanticExtraction({
        R"(
class C {
    newProperty: int = 2;

    compute(): int {
        return 9 * 8 + this.newProperty;
    }
}
)",
        "9 * 8",
        "UniqueClassProp.ets",
        "extract_constant_scope_1",
        ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME,
        "newProperty_1",
        {
            "private readonly newProperty_1 = 9 * 8;",
            "return this.newProperty_1 + this.newProperty;",
        },
    });
}

// A class-method collision makes the generated helper method distinct at both declaration and call sites.
TEST_F(ExtractSymbolUniqueNamesTests, UniqueClassMethodNameCollidesWithExistingMethod)
{
    ExpectSemanticExtraction({
        R"(
class C {
    newMethod(): int {
        return 0;
    }

    m(): int {
        let v = 5 + 1;
        return v;
    }
}
)",
        "5 + 1",
        "UniqueClassMethod.ets",
        "extract_function_scope_1",
        ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME,
        "newMethod_1",
        {
            "private newMethod_1(): number",
            "let v = this.newMethod_1();",
            "return 5 + 1;",
            "newMethod(): int",
        },
    });
}

// A top-level function collision keeps the generated global helper distinct.
TEST_F(ExtractSymbolUniqueNamesTests, UniqueGlobalFuncNameCollidesWithExistingFunction)
{
    ExpectSemanticExtraction({
        R"(class C {
    m(): int {
        let cfg = 12;
        return cfg;
    }
}
function newFunction(): int {
    return 0;
}
)",
        "12",
        "UniqueGlobalFunc.ets",
        "extract_function_scope_2",
        ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME,
        "newFunction_1",
        {
            "function newFunction_1():",
            "return 12;",
            "let cfg = newFunction_1();",
            "function newFunction(): int",
        },
    });
}

// A namespace variable collision keeps the generated namespace constant distinct.
TEST_F(ExtractSymbolUniqueNamesTests, UniqueNamespaceVarNameCollidesWithNamespaceLet)
{
    ExpectSemanticExtraction({
        R"(
namespace N {
    let newLocal = 3;

    export function f(): int {
        return 4 * 6;
    }
}
)",
        "4 * 6",
        "UniqueNsVar.ets",
        "extract_constant_scope_0",
        ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME,
        "newLocal_1",
        {
            "const newLocal_1: Int = 4 * 6;",
            "return newLocal_1;",
            "let newLocal = 3;",
        },
    });
}

// Two collisions advance the generated suffix to the next free name.
TEST_F(ExtractSymbolUniqueNamesTests, UniqueGlobalVarNameWalksTwoCollisions)
{
    ExpectSemanticExtraction({
        "let newLocal = 1;\nlet newLocal_1 = 2;\nconst g = 30 + 4;\nfunction use(): int { return g; }\n",
        "30 + 4",
        "UniqueTwoCollide.ets",
        "extract_variable_scope_2",
        ark::es2panda::lsp::refactor_name::EXTRACT_VARIABLE_ACTION_NAME,
        "newLocal_2",
        {
            "let newLocal_2: Int = 30 + 4;",
            "const g = newLocal_2;",
            "let newLocal = 1;",
            "let newLocal_1 = 2;",
        },
    });
}

// A namespace function collision keeps the generated namespace helper distinct.
TEST_F(ExtractSymbolUniqueNamesTests, UniqueNamespaceFuncNameCollidesWithNamespaceFunction)
{
    ExpectSemanticExtraction({
        R"(
namespace O {
    export function newFunction(): int {
        return 0;
    }

    namespace I {
        export function g(): int {
            return 2 * 9;
        }
    }
}
)",
        "2 * 9",
        "UniqueNsFunc.ets",
        "extract_function_scope_ns_1",
        ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME,
        "newFunction_1",
        {
            "function newFunction_1():",
            "return 2 * 9;",
            "return newFunction_1();",
            "export function newFunction(): int",
        },
    });
}
}  // namespace
