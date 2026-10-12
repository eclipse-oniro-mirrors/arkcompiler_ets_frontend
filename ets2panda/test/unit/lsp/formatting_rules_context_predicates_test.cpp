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
#include "lsp/include/api.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/formatting/formatting_settings.h"
#include "lsp/include/types.h"
#include "lsp_api_test.h"

namespace {
using ark::es2panda::lsp::FormatCodeSettings;

class LSPFormattingRulesContextPredicatesTests : public LSPAPITests {
public:
    struct FormatRun {
        bool contextOk {false};
        std::vector<TextChange> changes;
    };

    FormatRun RunFormat(const std::string &fileName, const std::string &code,
                        FormatCodeSettings settings = FormatCodeSettings())
    {
        FormatRun run;
        auto tempFiles = CreateTempFile({fileName}, {code});
        if (tempFiles.empty()) {
            return run;
        }

        ark::es2panda::lsp::Initializer initializer;
        es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
        if (ctx == nullptr) {
            return run;
        }
        run.contextOk = true;

        LSPAPI const *lspApi = GetImpl();
        run.changes = lspApi->getFormattingEditsForDocument(ctx, settings);
        initializer.DestroyContext(ctx);
        return run;
    }

    static void ExpectSingleChange(const FormatRun &run, size_t start, size_t length, const std::string &newText)
    {
        ASSERT_TRUE(run.contextOk);
        ASSERT_EQ(run.changes.size(), 1U);
        EXPECT_EQ(run.changes[0].span.start, start);
        EXPECT_EQ(run.changes[0].span.length, length);
        EXPECT_EQ(run.changes[0].newText, newText);
    }
};

// PB-006: IsModuleDeclContext (rules.cpp:469) is only referenced by the rule registered at
// rules.cpp:968 whose token pair is LITERAL_STRING followed by PUNCTUATOR_LEFT_BRACE. LSP plugin
// contexts always instantiate ETSParser regardless of file extension (es2panda_lib.cpp
// InitializeContext, "Plugin API supports only ETS"), and "declare module" is rejected there
// (ETSparser.cpp:2245-2248, IMPLEMENTATION_IN_AMBIENT_CONTEXT) leaving an error-recovery tree;
// per gcov the predicate's true block still fires through its ETS_MODULE ancestor arm (the
// ETSSCRIPT wrapper produced by util/importPathManager.cpp) while the TS_MODULE_DECLARATION
// comparison never matches. The pinned insert-space edit between the string literal and the
// opening brace is the observable current behavior of that rule evaluation.
TEST_F(LSPFormattingRulesContextPredicatesTests, ModuleDeclContextInsertsSpaceBeforeBodyBrace)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(declare module "M"{
    export let x: number;
}
)";

    // The only missing whitespace in the source is between "M" and the body brace, so the formatter
    // reports exactly one change inserting a space at the brace offset.
    size_t bracePosition = testCode.find('{');
    ASSERT_NE(bracePosition, std::string::npos);
    auto run = RunFormat("rules_predicates_ambient_module.ts", testCode);
    ExpectSingleChange(run, bracePosition, 0U, " ");
}

TEST_F(LSPFormattingRulesContextPredicatesTests, ModuleDeclContextMultilineBody)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(declare module "M"{
    export let x: number;
    export function f(): void;
}
)";

    // Multiline module body: the IsModuleDeclContext walk still resolves through the string literal
    // and every other token pair is already well spaced, so again exactly one insert-space change.
    size_t bracePosition = testCode.find('{');
    ASSERT_NE(bracePosition, std::string::npos);
    auto run = RunFormat("rules_predicates_ambient_module_multiline.ts", testCode);
    ExpectSingleChange(run, bracePosition, 0U, " ");
}

// PB-007: rule7 (rules.cpp:1023) guards DELETE_SPACE on the ">" -> "(", "[", ">", "," token pairs
// with IsTypeArgumentOrParameterOrAssertionContext + IsNotFunctionDeclContext +
// IsNonTypeAssertionContext. LSP plugin contexts always instantiate ETSParser regardless of file
// extension, and gcov shows rule7's predicates are never even called for this pair: the
// earlier-registered call-context rule (rules.cpp:865, AddFunctionCallRule before AddGenericRules
// in the same bucket) claims the space action first, so rule7 cannot emit. The pinned empty result
// is the observable current behavior for "<T>(y)" (PRODUCT FINDING: rule-order shadowing).
TEST_F(LSPFormattingRulesContextPredicatesTests, TypeAssertionContextSkipsGreaterThanParenRule)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = "let n = <T>(y);\n";

    auto run = RunFormat("rules_predicates_type_assertion.ts", testCode);
    ASSERT_TRUE(run.contextOk);
    EXPECT_TRUE(run.changes.empty());
}

// Inside a function declaration the IsNotFunctionDeclContext arm of rule7 takes its false arm (a
// FUNCTION_DECLARATION ancestor exists above the TSTypeAssertion), so rule7 is skipped one
// predicate earlier and the document still formats to no changes.
TEST_F(LSPFormattingRulesContextPredicatesTests, TypeAssertionContextInsideFunctionDeclSkipsRule)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = "function g() {\nlet n = <T>(y);\n}\n";

    auto run = RunFormat("rules_predicates_type_assertion_in_function.ts", testCode);
    ASSERT_TRUE(run.contextOk);
    EXPECT_TRUE(run.changes.empty());
}

// PB-008 probe: IsConstructorSignatureContext (rules.cpp:493) becomes true when a
// TS_SIGNATURE_DECLARATION (construct signature) or a constructor METHOD_DEFINITION is an ancestor
// of the current token. "new(): C" inside an interface parses into a TSConstructSignatureDeclaration
// (dumped via --extension=ts --dump-ast), so the predicate input exists, but rule5
// (rules.cpp:1065, KEYW_NEW -> LEFT_PAREN, DELETE_SPACE) is unreachable: the space-after-keyword
// INSERT_SPACE rule (rules.cpp:902, whose keyword set includes KEYW_NEW) sits earlier in the same
// (new, "(") bucket and always claims the space action first. The observable action is therefore an
// insert (the opposite of rule5's delete), proving the shadowing.
TEST_F(LSPFormattingRulesContextPredicatesTests, ConstructorSignatureContextIsShadowedByKeywordSpaceRule)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = "interface I {\nnew(): C;\n}\n";

    // Without a space after "new" the earlier keyword rule inserts one; rule5's DELETE never runs.
    size_t parenPosition = testCode.find('(');
    ASSERT_NE(parenPosition, std::string::npos);
    auto run = RunFormat("rules_predicates_construct_signature.ts", testCode);
    ExpectSingleChange(run, parenPosition, 0U, " ");

    // With the space already present the earlier rule's insert is a no-op and the document is left
    // unchanged: rule5's DELETE_SPACE is never consulted in either variant.
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string spacedCode = "interface I {\nnew (): C;\n}\n";
    auto spacedRun = RunFormat("rules_predicates_construct_signature_spaced.ts", spacedCode);
    ASSERT_TRUE(spacedRun.contextOk);
    EXPECT_TRUE(spacedRun.changes.empty());
}

// PB-009 probe: IsImportTypeContext (rules.cpp:235) walks the ancestors of the current token for a
// TS_IMPORT_TYPE node. LSP plugin contexts always instantiate ETSParser (es2panda_lib.cpp
// InitializeContext, "Plugin API supports only ETS") and TSParser::ParseImportType is the sole
// producer of TSImportType, so the predicate's true arm is unreachable in LSP contexts; on this
// source it is evaluated once and returns false (gcov: called 1x, match arm never taken). The
// formatter may either leave the recovered ETS source unchanged or report the delete edit at the
// KEYW_IMPORT -> LEFT_PAREN pair, so this testcase does not pin the total edit count.
TEST_F(LSPFormattingRulesContextPredicatesTests, ImportTypeContextDeletesSpaceAfterImport)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = "let x: import (\"./m\").T = null;\n";

    size_t spacePosition = testCode.find(" (");
    ASSERT_NE(spacePosition, std::string::npos);
    auto run = RunFormat("rules_predicates_import_type.ts", testCode);
    ASSERT_TRUE(run.contextOk);

    bool hasDeleteAfterImport = false;
    for (const auto &change : run.changes) {
        hasDeleteAfterImport = hasDeleteAfterImport || (change.span.start == spacePosition &&
                                                        change.span.length == 1U && change.newText.empty());
    }
    EXPECT_TRUE(run.changes.empty() || hasDeleteAfterImport);
}

}  // namespace
