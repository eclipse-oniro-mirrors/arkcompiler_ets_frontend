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
#include "lsp/include/organize_imports.h"
#include "test/unit/lsp/lsp_api_test.h"

namespace {
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::OrganizeImports;

class OrganizeImportsEdgeShapesTests : public LSPAPITests {};

// LSP_COVERAGE_V5 P3 tail batch - every test cites its target arm in
// ets2panda/lsp/src/organize_imports.cpp.
// NOTE: every module FILE name equals its import specifier + ".ets" (e.g.
// './edge_named' <-> edge_named.ets); a mismatch silently yields an unresolved
// module with unbound import specifiers and meaningless results.

// OI-EDGE-01: side-effect import form.  The ETS grammar PROHIBITS the pure
// side-effect form: `import './mod';` fails with ESY73297 ("Importing for
// side-effect only is prohibited", ets parser), so no zero-span
// ETSImportDeclaration is ever produced and the defensive skip at
// organize_imports.cpp:150-151 (Start().index == End().index) is unreachable
// through the ETS parser.  When the illegal statement follows a legal import,
// only the legal import is collected (organize_imports.cpp:133-137 Iterate +
// :142-152 filters) and the illegal line stays OUTSIDE the edit span, i.e. it
// is left untouched by the rewrite.  Measured-then-pinned exact TextChange.
TEST_F(OrganizeImportsEdgeShapesTests, SideEffectFormNotInOrganizeSpan)
{
    std::vector<std::string> files = {"edge_side_main.ets", "edge_side_effect.ets", "edge_named.ets"};
    std::vector<std::string> texts = {
        R"(import { Foo } from './edge_named';
import './edge_side_effect';
let foo: Foo = new Foo();)",
        R"(const sideMarker = 1;)", R"(export class Foo { value: number = 1; })"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1U);
    ASSERT_EQ(changes[0].fileName, filePaths[0]);
    ASSERT_EQ(changes[0].textChanges.size(), 1U);

    // Only the legal import (byte 0, 35 bytes) is inside the edit span; the
    // side-effect line that follows is never rewritten.
    EXPECT_EQ(changes[0].textChanges[0].span.start, 0U);
    EXPECT_EQ(changes[0].textChanges[0].span.length, 35U);
    EXPECT_EQ(changes[0].textChanges[0].newText, "import { Foo } from './edge_named';");

    initializer.DestroyContext(ctx);
}

// OI-EDGE-02: when RemoveUnusedImports (organize_imports.cpp:175-188) drops the
// only specifier, the ImportInfo survives with an empty namedImports vector, so
// GenerateTextChanges takes the `namedImports.empty()` continue at
// organize_imports.cpp:271-273 and the whole import line is deleted
// (newText == "", span == the import declaration).  Exact drop pin.
TEST_F(OrganizeImportsEdgeShapesTests, AllSpecifiersUnusedWholeImportDropped)
{
    std::vector<std::string> files = {"edge_unused_main.ets", "edge_unused_mod.ets"};
    std::vector<std::string> texts = {
        R"(import { Gone } from './edge_unused_mod';
let other = 1;
other;)",
        R"(export const Gone = 1;)"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1U);
    ASSERT_EQ(changes[0].textChanges.size(), 1U);
    // `import { Gone } from './edge_unused_mod';` is 41 bytes starting at byte
    // 0 (the raw string literal starts directly with the import keyword).
    EXPECT_EQ(changes[0].textChanges[0].span.start, 0U);
    EXPECT_EQ(changes[0].textChanges[0].span.length, 41U);
    EXPECT_EQ(changes[0].textChanges[0].newText, "");

    initializer.DestroyContext(ctx);
}

// OI-EDGE-03: re-export statements are ETSReExportDeclaration nodes, which
// IsImportUsed skips via organize_imports.cpp:74-75 (continue arm) and which
// CollectImports never collects, so the re-export is neither usage-counted nor
// touched by the edit.  The named import with a real usage survives verbatim.
TEST_F(OrganizeImportsEdgeShapesTests, ReExportStatementSkippedAndUntouched)
{
    std::vector<std::string> files = {"edge_reexp_main.ets", "edge_reexp.ets"};
    std::vector<std::string> texts = {
        R"(import { Bar } from './edge_reexp';
export { Foo } from './edge_reexp';
let bar: Bar = new Bar();)",
        R"(export class Foo { value: number = 1; }
export class Bar { value: number = 2; })"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1U);
    ASSERT_EQ(changes[0].textChanges.size(), 1U);

    const std::string &newText = changes[0].textChanges[0].newText;
    // The re-export statement is out of the edit span and out of the rewrite.
    EXPECT_EQ(newText.find("export { Foo }"), std::string::npos);
    EXPECT_EQ(newText, "import { Bar } from './edge_reexp';");
    // Span covers only the first import line (bytes 0..35), not the re-export.
    EXPECT_EQ(changes[0].textChanges[0].span.start, 0U);
    EXPECT_EQ(changes[0].textChanges[0].span.length, 35U);

    initializer.DestroyContext(ctx);
}

// OI-EDGE-04: namespace-qualified usage detection in TYPE position
// (organize_imports.cpp:35-41): a `NS1.T` TSQualifiedName makes Left()->Is-
// Identifier() true and the name comparison at :38-39 decide the namespace
// import is used.  The second, never-qualified namespace import fails the same
// comparison ("NS1" != "AB" - length-unequal too, taking both arms of :39's
// std::string equality) and is dropped.
TEST_F(OrganizeImportsEdgeShapesTests, NamespaceQualifiedTypeUseKeepsOnlyUsedNs)
{
    std::vector<std::string> files = {"edge_ns_main.ets", "edge_ns.ets"};
    std::vector<std::string> texts = {
        R"(import * as NS1 from './edge_ns';
import * as AB from './edge_ns';
let t: NS1.T = "hello";)",
        R"(export type T = string;)"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1U);
    ASSERT_EQ(changes[0].textChanges.size(), 1U);

    const std::string &newText = changes[0].textChanges[0].newText;
    EXPECT_EQ(newText.find("import * as AB from './edge_ns';"), std::string::npos);
    // NS-unused comparison at organize_imports.cpp:39: "NS1" != "AB" (also
    // length-unequal, taking the std::string==length-check arm) => AB is
    // judged unused and dropped; its ImportInfo contributes only the
    // inter-import gap "\n" bridged by organize_imports.cpp:268-269 +
    // ProcessComment (:248-256) after NS1's rewrite.
    EXPECT_EQ(newText, "import * as NS1 from './edge_ns';\n");
    // Span covers both import lines: 33 + 1 + 32 = 66 bytes from byte 0.
    EXPECT_EQ(changes[0].textChanges[0].span.start, 0U);
    EXPECT_EQ(changes[0].textChanges[0].span.length, 66U);

    initializer.DestroyContext(ctx);
}

// OI-EDGE-05: shadowed local names.  Every `foo` below resolves to the class
// property, the parameter, or the member access - none is the import specifier,
// so DeclarationFromIdentifier(usage) != DeclarationFromIdentifier(touching
// token at spec.start) and organize_imports.cpp:65-68 takes the not-equal arm
// (or the null-decl guard at :62-63 for the member-access property), leaving
// found == false and the import is removed.
TEST_F(OrganizeImportsEdgeShapesTests, ShadowedLocalNameImportJudgedUnused)
{
    std::vector<std::string> files = {"edge_shadow_main.ets", "edge_shadow.ets"};
    std::vector<std::string> texts = {
        R"(import { foo } from './edge_shadow';
class K {
    foo: number = 1;
}
function g(foo: number): number {
    return foo;
}
let k = new K();
let v1 = k.foo;
let v2 = g(1);)",
        R"(export const foo = 1;)"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1U);
    ASSERT_EQ(changes[0].textChanges.size(), 1U);
    // The raw string literal starts directly with the import keyword, so the
    // declaration spans bytes 0..35 (`import { foo } from './edge_shadow';`).
    EXPECT_EQ(changes[0].textChanges[0].span.start, 0U);
    EXPECT_EQ(changes[0].textChanges[0].span.length, 36U);
    EXPECT_EQ(changes[0].textChanges[0].newText, "");

    initializer.DestroyContext(ctx);
}

// OI-EDGE-06: ExtractDefaultImport ordering edge with the default import in the
// MIDDLE plus an aliased remainder (organize_imports.cpp:198-227): the alias
// arm at :210-211 emits "one as uno" and the :215-217 skip does NOT fire
// (defaultIndex != size-1), so the comma at :218-219 is kept after "C".
// Expected rewrite: "import Foo, { C, one as uno } from './edge_defmid';"
TEST_F(OrganizeImportsEdgeShapesTests, ExtractDefaultImportDefaultMiddleWithAlias)
{
    std::vector<std::string> files = {"edge_defmid_main.ets", "edge_defmid.ets"};
    std::vector<std::string> texts = {
        R"(import { C, Foo, one as uno } from './edge_defmid';
let c: number = C;
let foo: Foo = new Foo();
let u = uno;)",
        R"(export const C: number = 1;
export function one(): number {
    return 1;
}
class Foo {
    name: string = "john";
}
export default Foo;)"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1U);
    ASSERT_EQ(changes[0].textChanges.size(), 1U);
    EXPECT_EQ(changes[0].textChanges[0].newText, "import Foo, { C, one as uno } from './edge_defmid';");

    initializer.DestroyContext(ctx);
}

// OI-EDGE-07: default import LAST with an aliased named import: here the
// :215-217 skip DOES fire (i+1 == defaultIndex && defaultIndex == size-1), so
// no trailing comma is emitted before the closing brace.
// Expected rewrite: "import Foo, { one as uno } from './edge_deflast';"
TEST_F(OrganizeImportsEdgeShapesTests, ExtractDefaultImportDefaultLastSkipsTrailingComma)
{
    std::vector<std::string> files = {"edge_deflast_main.ets", "edge_deflast.ets"};
    std::vector<std::string> texts = {
        R"(import { one as uno, Foo } from './edge_deflast';
let foo: Foo = new Foo();
let u = uno;)",
        R"(export function one(): number {
    return 1;
}
class Foo {
    name: string = "john";
}
export default Foo;)"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1U);
    ASSERT_EQ(changes[0].textChanges.size(), 1U);
    EXPECT_EQ(changes[0].textChanges[0].newText, "import Foo, { one as uno } from './edge_deflast';");

    initializer.DestroyContext(ctx);
}

// OI-EDGE-08: multi-import rewrite with a comment between two imports: the
// comment bridge at organize_imports.cpp:268-269 + ProcessComment
// (:248-256) copies the inter-import gap verbatim into the single TextChange
// that spans from the first import start to the last import end (:297).
// Measured-then-pinned: the single edit covers the whole two-import region
// byte-for-byte, including the preserved comment gap.
TEST_F(OrganizeImportsEdgeShapesTests, MultiImportCommentBridgeExactTextChange)
{
    std::vector<std::string> files = {"edge_multi_main.ets", "edge_multi_a.ets", "edge_multi_b.ets"};
    std::vector<std::string> texts = {
        R"(import { A } from './edge_multi_a';
// bridge comment
import { B } from './edge_multi_b';
let a = A;
let b = B;)",
        R"(export const A = 1;)", R"(export const B = 2;)"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1U);
    ASSERT_EQ(changes[0].textChanges.size(), 1U);
    // Both imports plus the comment line between them are one edit region:
    // 35 + 1 + 17 + 1 + 35 = 89 bytes starting at byte 0.  The rewrite
    // re-emits both used imports in order and copies the comment gap in
    // between (organize_imports.cpp:268-269 + ProcessComment :248-256).
    EXPECT_EQ(changes[0].textChanges[0].span.start, 0U);
    EXPECT_EQ(changes[0].textChanges[0].span.length, 89U);
    EXPECT_EQ(changes[0].textChanges[0].newText,
              "import { A } from './edge_multi_a';\n// bridge comment\nimport { B } from './edge_multi_b';");

    initializer.DestroyContext(ctx);
}
}  // namespace
