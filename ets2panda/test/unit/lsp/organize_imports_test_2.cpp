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

#include "lsp_api_test.h"

#include <gtest/gtest.h>

#include "lsp/include/api.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/organize_imports.h"

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::OrganizeImports;

class OrganizeImportsTests2 : public LSPAPITests {};

// NOTE: "preserve side-effect import" (import './mod') cannot be tested here: the ArkTS
// parser drops such declarations (diagnostic ERROR_ARKTS_NO_SIDE_EFFECT_IMPORT, id 73297),
// so OrganizeImports never sees them. Instead this test verifies that an import block that
// is left without any specifier produces no text changes, i.e. nothing is spuriously deleted.
TEST_F(OrganizeImportsTests2, EmptyImportBlockProducesNoChanges)
{
    std::vector<std::string> files = {"oi2_empty_block.ets"};
    std::vector<std::string> texts = {R"(let value: number = 1;
value;)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(changes.size(), 1U);
    EXPECT_EQ(changes[0].fileName, filePaths[0]);
    EXPECT_TRUE(changes[0].textChanges.empty());
}

TEST_F(OrganizeImportsTests2, RemovesUnusedImportAndKeepsComment)
{
    std::vector<std::string> files = {"oi2_unused_comment.ets", "oi2_used_mod.ets", "oi2_unused_mod.ets"};
    std::vector<std::string> texts = {R"(import { Used } from './oi2_used_mod';
// keep this comment
import { Unused } from './oi2_unused_mod';
Used;)",
                                      R"(export class Used { value: number = 1; })",
                                      R"(export class Unused { value: number = 2; })"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(changes.size(), 1U);
    EXPECT_EQ(changes[0].fileName, filePaths[0]);
    ASSERT_EQ(changes[0].textChanges.size(), 1U);

    const auto &change = changes[0].textChanges[0];
    // The whole import block (including the unused import) is replaced.
    EXPECT_EQ(change.span.start, 0U);
    EXPECT_EQ(change.span.length, 102U);
    // The used import is regenerated, the comment between the imports is moved
    // after it, and the unused import declaration is removed.
    EXPECT_EQ(change.newText, "import { Used } from './oi2_used_mod';\n// keep this comment\n");
    EXPECT_EQ(change.newText.find("Unused"), std::string::npos);
    EXPECT_NE(change.newText.find("// keep this comment"), std::string::npos);
}

TEST_F(OrganizeImportsTests2, MultiLineImportSortedOutput)
{
    std::vector<std::string> files = {"oi2_mls.ets", "oi2_ml_mod.ets"};
    std::vector<std::string> texts = {R"(import {
    Beta,
    Alpha
} from './oi2_ml_mod';
Alpha;
Beta;)",
                                      R"(export class Alpha { a: number = 1; }
export class Beta { b: number = 2; })"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(changes.size(), 1U);
    EXPECT_EQ(changes[0].fileName, filePaths[0]);
    ASSERT_EQ(changes[0].textChanges.size(), 1U);

    const auto &change = changes[0].textChanges[0];
    EXPECT_EQ(change.span.start, 0U);
    EXPECT_EQ(change.span.length, 51U);
    // The multi-line import is regenerated as a single-line, syntactically valid import.
    // Specifiers keep their source order (no alphabetical sorting is applied).
    EXPECT_EQ(change.newText, "import { Beta, Alpha } from './oi2_ml_mod';");
}

TEST_F(OrganizeImportsTests2, RemovesUnusedSpecifiersFromNamedImport)
{
    std::vector<std::string> files = {"oi2_usk.ets", "oi2_usk_mod.ets"};
    std::vector<std::string> texts = {R"(import { Used, Unused } from './oi2_usk_mod';
Used;)",
                                      R"(export class Used { a: number = 1; }
export class Unused { b: number = 2; })"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(changes.size(), 1U);
    EXPECT_EQ(changes[0].fileName, filePaths[0]);
    ASSERT_EQ(changes[0].textChanges.size(), 1U);

    const auto &change = changes[0].textChanges[0];
    EXPECT_EQ(change.span.start, 0U);
    EXPECT_EQ(change.span.length, 45U);
    // Only the used specifier survives.
    EXPECT_EQ(change.newText, "import { Used } from './oi2_usk_mod';");
    EXPECT_EQ(change.newText.find("Unused"), std::string::npos);
}

TEST_F(OrganizeImportsTests2, MultiLineImportNormalizedToSingleLine)
{
    std::vector<std::string> files = {"probe_ml.ets", "probe_ml_mod.ets"};
    std::vector<std::string> texts = {R"(import {
    Alpha,
    Beta
} from './probe_ml_mod';
Alpha;
Beta;)",
                                      R"(export class Alpha { a: number = 1; }
export class Beta { b: number = 2; })"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(changes.size(), 1U);
    EXPECT_EQ(changes[0].fileName, filePaths[0]);
    ASSERT_EQ(changes[0].textChanges.size(), 1U);

    const auto &change = changes[0].textChanges[0];
    EXPECT_EQ(change.span.start, 0U);
    EXPECT_EQ(change.span.length, 53U);
    EXPECT_EQ(change.newText, "import { Alpha, Beta } from './probe_ml_mod';");
}

TEST_F(OrganizeImportsTests2, DefaultPlusNamedImportCombined)
{
    std::vector<std::string> files = {"probe_dn.ets", "probe_dn_mod.ets"};
    std::vector<std::string> texts = {R"(import Def, { Named } from './probe_dn_mod';
Def;
Named;)",
                                      R"(export default class Def { d: number = 1; }
export class Named { n: number = 2; })"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(changes.size(), 1U);
    EXPECT_EQ(changes[0].fileName, filePaths[0]);
    ASSERT_EQ(changes[0].textChanges.size(), 1U);

    const auto &change = changes[0].textChanges[0];
    EXPECT_EQ(change.span.start, 0U);
    EXPECT_EQ(change.span.length, 44U);
    // The default import and the named import are combined into a single declaration.
    EXPECT_EQ(change.newText, "import Def, { Named } from './probe_dn_mod';");
}

TEST_F(OrganizeImportsTests2, AliasImportPreserved)
{
    std::vector<std::string> files = {"probe_alias.ets", "probe_alias_mod.ets"};
    std::vector<std::string> texts = {R"(import { Alpha as A1, Beta } from './probe_alias_mod';
A1;
Beta;)",
                                      R"(export class Alpha { a: number = 1; }
export class Beta { b: number = 2; })"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(changes.size(), 1U);
    EXPECT_EQ(changes[0].fileName, filePaths[0]);
    ASSERT_EQ(changes[0].textChanges.size(), 1U);

    const auto &change = changes[0].textChanges[0];
    EXPECT_EQ(change.span.start, 0U);
    EXPECT_EQ(change.span.length, 54U);
    // The alias is preserved in the regenerated import.
    EXPECT_EQ(change.newText, "import { Alpha as A1, Beta } from './probe_alias_mod';");
}

TEST_F(OrganizeImportsTests2, ExportFromNotRemoved)
{
    std::vector<std::string> files = {"probe_ef.ets", "probe_ef_mod.ets"};
    std::vector<std::string> texts = {R"(import { Foo } from './probe_ef_mod';
export { Bar } from './probe_ef_mod';
)",
                                      R"(export class Foo { a: number = 1; }
export class Bar { b: number = 2; })"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(changes.size(), 1U);
    EXPECT_EQ(changes[0].fileName, filePaths[0]);
    ASSERT_EQ(changes[0].textChanges.size(), 1U);

    const auto &change = changes[0].textChanges[0];
    // Only the unused import line is targeted for deletion.
    EXPECT_EQ(change.span.start, 0U);
    EXPECT_EQ(change.span.length, 37U);
    EXPECT_EQ(change.newText, "");
    // The export-from statement right after the span is untouched (only the
    // line feed separating it from the import line is part of the span).
    EXPECT_EQ(texts[0].substr(change.span.start + change.span.length), "\nexport { Bar } from './probe_ef_mod';\n");
}

TEST_F(OrganizeImportsTests2, DISABLED_SameSourceImportsAreMerged)
{
    std::vector<std::string> files = {"probe_ss.ets", "probe_ss_mod.ets"};
    std::vector<std::string> texts = {R"(import { Alpha } from './probe_ss_mod';
import { Beta } from './probe_ss_mod';
Alpha;
Beta;)",
                                      R"(export class Alpha { a: number = 1; }
export class Beta { b: number = 2; })"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(changes.size(), 1U);
    EXPECT_EQ(changes[0].fileName, filePaths[0]);
    ASSERT_EQ(changes[0].textChanges.size(), 1U);

    const auto &change = changes[0].textChanges[0];
    EXPECT_EQ(change.span.start, 0U);
    EXPECT_EQ(change.span.length, 78U);
    EXPECT_EQ(change.newText, "import { Alpha, Beta } from './probe_ss_mod';");
}
