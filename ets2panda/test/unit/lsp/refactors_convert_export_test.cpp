/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "lsp/include/refactors/refactor_types.h"
#include "lsp_api_test.h"
#include "lsp/include/get_edits_for_refactor.h"
#include "lsp/include/types.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/user_preferences.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

const int REFACTOR_SPAN = 15;

namespace {
using ark::es2panda::lsp::Initializer;

class LspConvExpImportTests2 : public LSPAPITests {
public:
    static constexpr std::string_view TO_DEFAULT_EXPORT_KIND = "refactor.rewrite.export.default";

    // Helper to create two files for the test
    std::vector<std::string> CreateNamedExportFiles()
    {
        std::vector<std::string> names = {"ExportFileDone2.ets", "ImportFileDone2.ets"};
        std::vector<std::string> contents = {
            R"(export function bar() {
  return 100;
})",
            R"(import { bar } from "./ExportFileDone2";

console.log(bar());
)"};

        return CreateTempFile(names, contents);
    }

    ark::es2panda::lsp::RefactorContext *CreateDefaultFiles(Initializer *initializer)
    {
        // Create initial files with default export and default import
        std::vector<std::string> names = {"ExportFileDone3.ets", "ImportFileDone3.ets"};
        std::vector<std::string> contents = {R"(export default function foo() { return 200; })",
                                             R"(import foo from "./ExportFileDone3"; console.log(foo());)"};
        auto filePaths = CreateTempFile(names, contents);

        auto ctx = initializer->CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

        ark::es2panda::lsp::UserPreferences localPrefs;
        ark::es2panda::lsp::FormatCodeSettings codeSettings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
        ark::es2panda::lsp::FormatContext fmt = ark::es2panda::lsp::GetFormatContext(codeSettings);

        ark::es2panda::lsp::UserPreferences prefs = ark::es2panda::lsp::UserPreferences::GetDefaultUserPreferences();
        LanguageServiceHost host;
        auto *textChangesContext = new TextChangesContext {host, fmt, prefs};
        auto *refactorContext = new ark::es2panda::lsp::RefactorContext;
        refactorContext->textChangesContext = textChangesContext;
        refactorContext->context = ctx;
        refactorContext->kind = std::string(TO_DEFAULT_EXPORT_KIND);
        refactorContext->span.pos = REFACTOR_SPAN;
        refactorContext->span.end = REFACTOR_SPAN;
        return refactorContext;
    }
};

TEST_F(LspConvExpImportTests2, ConvertDefaultExportToNamedAndUpdateImports)
{
    auto *initializer = new Initializer();
    auto *refactorContext = CreateDefaultFiles(initializer);
    auto applicable = GetApplicableRefactorsImpl(refactorContext);
    ASSERT_EQ(1, applicable.size());

    auto edits =
        ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, applicable[0].name, applicable[0].action.name);
    initializer->DestroyContext(refactorContext->context);
    int expectedEditSize = 2;  // One for export file, one for import file
    ASSERT_EQ(expectedEditSize, edits->GetFileTextChanges().size());

    bool exportChanged = false;
    bool importChanged = false;
    for (auto &edit : edits->GetFileTextChanges()) {
        for (auto &tc : edit.textChanges) {
            if (edit.fileName.find("ExportFileDone3.ets") != std::string::npos) {
                exportChanged = true;
                ASSERT_TRUE(tc.newText.empty());
            }
            if (edit.fileName.find("ImportFileDone3.ets") != std::string::npos) {
                importChanged = true;
                ASSERT_TRUE(tc.newText.find("import { foo } from") != std::string::npos);
            }
        }
    }
    ASSERT_TRUE(exportChanged);
    ASSERT_TRUE(importChanged);
}

}  // namespace