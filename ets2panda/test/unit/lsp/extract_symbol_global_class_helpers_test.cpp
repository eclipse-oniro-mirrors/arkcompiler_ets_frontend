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

class ExtractSymbolGlobalClassHelpersTests : public LSPAPITests {
public:
    struct GlobalClassHelperScenario {
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

    void ExpectSemanticExtraction(const GlobalClassHelperScenario &scenario)
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

// A method parameter is passed through the generated global helper and its call.
TEST_F(ExtractSymbolGlobalClassHelpersTests, GlobalPiecesHelperCapturesMethodParam)
{
    ExpectSemanticExtraction({
        R"(class C {
    m(base: int): int {
        let scaled = base * 7;
        return scaled;
    }
}
)",
        "base * 7",
        "GlobalPiecesParam.ets",
        "extract_function_scope_2",
        ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME,
        "newFunction",
        {
            "function newFunction(base: int):",
            "return base * 7;",
            "let scaled = newFunction(base);",
        },
    });
}

// A block-bodied arrow initializer remains a complete statement in the generated helper.
TEST_F(ExtractSymbolGlobalClassHelpersTests, GlobalPiecesArrowBlockInitializerSemicolon)
{
    ExpectSemanticExtraction({
        R"(class C {
    m(): int {
        let fn = (v: int): int => {
            return v + 1;
        };
        return fn(2);
    }
}
)",
        "(v: int): int => {\n            return v + 1;\n        }",
        "GlobalPiecesArrow.ets",
        "extract_function_scope_2",
        ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME,
        "newFunction",
        {
            "function newFunction()",
            "return (v: int): int =>",
            "return v + 1;",
            "};",
            "let fn = newFunction();",
        },
    });
}

// Extraction in a static method still invokes the generated class helper through this.
TEST_F(ExtractSymbolGlobalClassHelpersTests, ClassPiecesStaticMethodShape)
{
    ExpectSemanticExtraction({
        R"(class C {
    static m(): int {
        let v = 6 * 7;
        return v;
    }
}
)",
        "6 * 7",
        "ClassPiecesStatic.ets",
        "extract_function_scope_1",
        ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME,
        "newMethod",
        {
            "private newMethod(): number",
            "return 6 * 7;",
            "return v;",
            "let v = this.newMethod();",
            "static m(): int",
        },
    });
}

// A two-space class retains its members while adding the generated helper and call.
TEST_F(ExtractSymbolGlobalClassHelpersTests, ClassPiecesTwoSpaceMemberIndent)
{
    ExpectSemanticExtraction({
        R"(class C {
  m(): int {
    let v = 3 + 4;
    return v;
  }
}
)",
        "3 + 4",
        "ClassPiecesIndent.ets",
        "extract_function_scope_1",
        ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME,
        "newMethod",
        {
            "private newMethod(): number",
            "return 3 + 4;",
            "let v = this.newMethod();",
        },
    });
}

// A helper capturing a this-qualified value preserves that access in its body.
TEST_F(ExtractSymbolGlobalClassHelpersTests, ClassPiecesHelperCapturesThisMember)
{
    ExpectSemanticExtraction({
        R"(class C {
    v: int = 5;

    m(): int {
        let doubled = this.v * 2;
        return doubled;
    }
}
)",
        "this.v * 2",
        "ClassPiecesThis.ets",
        "extract_function_scope_1",
        ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME,
        "newMethod",
        {
            "private newMethod(): number",
            "return this.v * 2;",
            "let doubled = this.newMethod();",
            "return doubled;",
        },
    });
}

// A declaration-leading selection becomes one helper and a const replacement for its first binding.
TEST_F(ExtractSymbolGlobalClassHelpersTests, GlobalFromDeclarationConstThenLet)
{
    ExpectSemanticExtraction({
        "const k: int = 3;\nlet b = k + 4;\nfunction use(): int { return b; }\n",
        "const k: int = 3;\nlet b = k + 4;",
        "GlobalDeclSelConst.ets",
        "extract_function_scope_2",
        ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME,
        "newFunction",
        {
            "function newFunction(): int",
            "const k: int = 3;",
            "let b = k + 4;",
            "return k;",
            "const k: int = newFunction();",
        },
    });
}

// A chained-let selection returns its first binding and replaces only that declaration.
TEST_F(ExtractSymbolGlobalClassHelpersTests, GlobalFromDeclarationChainedLet)
{
    ExpectSemanticExtraction({
        "let s = 11;\nlet t = s * 2;\nfunction use(): int { return t; }\n",
        "let s = 11;\nlet t = s * 2;",
        "GlobalDeclSelChain.ets",
        "extract_function_scope_2",
        ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME,
        "newFunction",
        {
            "function newFunction(): int",
            "let s = 11;",
            "let t = s * 2;",
            "return s;",
            "let s = newFunction();",
        },
    });
}

// An exported namespace value used by an extracted body is captured as a helper parameter.
TEST_F(ExtractSymbolGlobalClassHelpersTests, QualifyGlobalBodyExportedNamespaceValue)
{
    ExpectSemanticExtraction({
        R"(namespace N {
    export let base: int = 10;

    export function f(): int {
        let s = base + 2;
        let t = s * 3;
        return t;
    }
}
)",
        "let s = base + 2;\n        let t = s * 3;",
        "QualifyNsValue.ets",
        "extract_function_scope_2",
        ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME,
        "newFunction",
        {
            "function newFunction()",
            "let s = N.base + 2;",
            "let t = s * 3;",
            "newFunction();",
        },
    });
}

// A type from a nested namespace is qualified through every enclosing namespace.
TEST_F(ExtractSymbolGlobalClassHelpersTests, QualifyGlobalBodyNestedNamespaceType)
{
    ExpectSemanticExtraction({
        R"(namespace O {
    namespace I {
        export class T {
            v: int = 0;
        }

        export function g(): int {
            let t: T = new T();
            let s = t.v + 1;
            return s;
        }
    }
}
)",
        "let t: T = new T();\n            let s = t.v + 1;",
        "QualifyNestedNs.ets",
        "extract_function_scope_2",
        ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME,
        "newFunction",
        {
            "function newFunction()",
            "let t: O.I.T = new O.I.T();",
            "let s = t.v + 1;",
            "newFunction();",
        },
    });
}

// A namespace class type used in annotations and object creation stays fully qualified.
TEST_F(ExtractSymbolGlobalClassHelpersTests, QualifyGlobalBodyNamespaceTypeReference)
{
    ExpectSemanticExtraction({
        R"(namespace N {
    export class C {
        v: int = 0;
    }

    export function h(): int {
        let c: C = new C();
        let s = c.v + 1;
        return s;
    }
}
)",
        "let c: C = new C();\n        let s = c.v + 1;",
        "QualifyNsType.ets",
        "extract_function_scope_2",
        ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME,
        "newFunction",
        {
            "function newFunction()",
            "let c: N.C = new N.C();",
            "let s = c.v + 1;",
            "newFunction();",
        },
    });
}

// The global-constant route qualifies a namespace value used by the extracted expression.
TEST_F(ExtractSymbolGlobalClassHelpersTests, NamespaceQualifierOnGlobalConstantRoute)
{
    ExpectSemanticExtraction({
        R"(namespace N {
    export let base: int = 10;

    export function f(): int {
        let s = base * 2;
        return s;
    }
}
)",
        "base * 2",
        "NsConstQualifier.ets",
        "extract_constant_scope_2",
        ark::es2panda::lsp::refactor_name::EXTRACT_CONSTANT_ACTION_NAME,
        "newLocal",
        {
            "const newLocal: Int = N.base * 2;",
            "let s = newLocal;",
        },
    });
}
}  // namespace
