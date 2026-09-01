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
#include "lsp/include/completions.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

#include <algorithm>
#include <string>
#include <vector>

class GetCompletionsExportCollectTests : public LSPAPITests {
public:
    // Runs getCompletionsAtPosition at the offset right after the first marker occurrence.
    std::vector<ark::es2panda::lsp::CompletionEntry> CompletionsAfterMarker(const std::vector<std::string> &filePaths,
                                                                            size_t queryIdx,
                                                                            const std::string &queryText,
                                                                            const std::string &marker,
                                                                            es2panda_ContextState state)
    {
        auto markerPos = queryText.find(marker);
        if (markerPos == std::string::npos) {
            ADD_FAILURE() << "Marker not found: " << marker;
            return {};
        }
        LSPAPI const *lspApi = GetImpl();
        ark::es2panda::lsp::Initializer initializer;
        auto ctx = initializer.CreateContext(filePaths[queryIdx].c_str(), state);
        if (ctx == nullptr) {
            ADD_FAILURE() << "Failed to create LSP context";
            return {};
        }
        auto entries = lspApi->getCompletionsAtPosition(ctx, markerPos + marker.size()).GetEntries();
        initializer.DestroyContext(ctx);
        return entries;
    }

    static const ark::es2panda::lsp::CompletionEntry *FindEntry(
        const std::vector<ark::es2panda::lsp::CompletionEntry> &entries, const std::string &name)
    {
        for (const auto &entry : entries) {
            if (entry.GetName() == name) {
                return &entry;
            }
        }
        return nullptr;
    }

    // Finds an entry by name and pins kind/sortText/insertText exactly.
    void ExpectEntryExact(const std::vector<ark::es2panda::lsp::CompletionEntry> &entries, const std::string &name,
                          ark::es2panda::lsp::CompletionEntryKind kind, const std::string &sortText,
                          const std::string &insertText)
    {
        const auto *entry = FindEntry(entries, name);
        ASSERT_NE(entry, nullptr) << "completion entry not found: " << name;
        EXPECT_EQ(entry->GetCompletionKind(), kind) << "kind mismatch for entry: " << name;
        EXPECT_EQ(entry->GetSortText(), sortText) << "sortText mismatch for entry: " << name;
        EXPECT_EQ(entry->GetInsertText(), insertText) << "insertText mismatch for entry: " << name;
    }

    void ExpectNoEntryNamed(const std::vector<ark::es2panda::lsp::CompletionEntry> &entries, const std::string &name)
    {
        EXPECT_EQ(FindEntry(entries, name), nullptr) << "unexpected completion entry: " << name;
    }

    // Collects API info from filePaths[ctxIdx] and asserts the call succeeded.
    void CollectApiInfo(const std::vector<std::string> &filePaths, size_t ctxIdx, es2panda_ContextState state)
    {
        LSPAPI const *lspApi = GetImpl();
        ark::es2panda::lsp::Initializer initializer;
        auto ctx = initializer.CreateContext(filePaths[ctxIdx].c_str(), state);
        ASSERT_NE(ctx, nullptr);
        EXPECT_TRUE(lspApi->collectApiInfo(ctx));
        initializer.DestroyContext(ctx);
    }
};

namespace {

using ark::es2panda::lsp::CompletionEntryKind;

// collectApiInfo walks every export declaration kind of a '@'-prefixed SDK module
// through AddCollectApiEntryByDecl: class/interface/alias/function/const/let/
// struct/annotation/namespace(TSModuleDeclaration). PARSED keeps top-level decls
// unlowered so each AddCollectApiEntryByDecl dispatch arm is exercised directly.
TEST_F(GetCompletionsExportCollectTests, CollectApiInfoCoversAllExportDeclKinds)
{
    std::vector<std::string> files = {"@ec_sdk_kinds.d.ets", "ec_sdk_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export class EcSdkClass {}
export interface EcSdkIface {}
export type EcSdkAlias = number;
export function ecSdkFunc(a: number): number {
    return a;
}
export const EC_SDK_CONST: number = 1;
export let ecSdkLet: number = 2;
export struct EcSdkStruct {}
export @interface EcSdkAnn {
    f: number;
}
export namespace EcSdkNs {
}
export { EcSdkNs };
)",
                                      R"(
import { ecSdkLet } from './@ec_sdk_kinds';
ecSdkLet
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    CollectApiInfo(filePaths, 1, ES2PANDA_STATE_PARSED);

    auto expectOne = [](const std::string &name, ark::es2panda::lsp::CompletionEntryKind kind,
                        const std::string &insertText) {
        auto infos = ark::es2panda::lsp::GetExternalApiCollectInfos(name);
        ASSERT_FALSE(infos.empty()) << "export not collected: " << name;
        EXPECT_EQ(infos.front().kind, kind) << "kind mismatch for collected export: " << name;
        EXPECT_EQ(infos.front().insertText, insertText) << "insertText mismatch for collected export: " << name;
        EXPECT_FALSE(infos.front().importDeclaration.empty()) << "import path missing for: " << name;
    };
    expectOne("EcSdkClass", CompletionEntryKind::CLASS, "EcSdkClass");
    expectOne("EcSdkIface", CompletionEntryKind::INTERFACE, "EcSdkIface");
    expectOne("EcSdkAlias", CompletionEntryKind::ALIAS_TYPE, "EcSdkAlias");
    expectOne("ecSdkFunc", CompletionEntryKind::FUNCTION, "ecSdkFunc()");
    expectOne("EC_SDK_CONST", CompletionEntryKind::CONSTANT, "EC_SDK_CONST");
    expectOne("ecSdkLet", CompletionEntryKind::VARIABLE, "ecSdkLet");
    expectOne("EcSdkStruct", CompletionEntryKind::STRUCT, "EcSdkStruct");
    expectOne("EcSdkAnn", CompletionEntryKind::ANNOTATION, "EcSdkAnn");
    // CURRENT BEHAVIOR: at PARSED the `export { EcSdkNs }` specifier resolves to an
    // export-alias binding whose declaration node is not TSModuleDeclaration, so
    // AddCollectApiEntryByDecl falls through to the VARIABLE fallback in
    // CollectExportFromSpecifier. The MODULE kind for namespaces is covered by the
    // transformed-classification path in CollectApiInfoCheckedStateTransformedEnumAndNamespace.
    expectOne("EcSdkNs", CompletionEntryKind::VARIABLE, "EcSdkNs");

    // An unknown name yields no collected info.
    EXPECT_TRUE(ark::es2panda::lsp::GetExternalApiCollectInfos("NoSuchEcSdkSymbol").empty());
}

// At CHECKED state enum/namespace exports are lowered to class definitions flagged as
// enum-/namespace-transformed; collectApiInfo must classify them ENUM/MODULE.
TEST_F(GetCompletionsExportCollectTests, CollectApiInfoCheckedStateTransformedEnumAndNamespace)
{
    std::vector<std::string> files = {"@ec_tr.d.ets", "ec_tr_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export enum EcTrEnum {
    A,
}
export namespace EcTrNs {
    export function inner(): void {}
}
)",
                                      R"(
import { EcTrEnum } from './@ec_tr';
EcTrEnum
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    CollectApiInfo(filePaths, 1, ES2PANDA_STATE_CHECKED);

    auto enumInfos = ark::es2panda::lsp::GetExternalApiCollectInfos("EcTrEnum");
    ASSERT_FALSE(enumInfos.empty()) << "transformed enum export not collected";
    EXPECT_EQ(enumInfos.front().kind, CompletionEntryKind::ENUM);
    auto nsInfos = ark::es2panda::lsp::GetExternalApiCollectInfos("EcTrNs");
    ASSERT_FALSE(nsInfos.empty()) << "transformed namespace export not collected";
    EXPECT_EQ(nsInfos.front().kind, CompletionEntryKind::MODULE);
}

// Default export together with named exports of an '@'-prefixed SDK module are both
// collected; the default flag survives in ExternalApiCollectInfo.
TEST_F(GetCompletionsExportCollectTests, KitImportsCollectPlainModuleExports)
{
    std::vector<std::string> files = {"@ec_kit.d.ets", "ec_kit_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export class EcKitNamed {}
const ecKitDef = 9;
export default ecKitDef;
export default class EcKitDefaultCls {}
)",
                                      R"(
import { EcKitNamed } from './@ec_kit';
import EcKitDefault from './@ec_kit';
EcKitNamed
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    CollectApiInfo(filePaths, 1, ES2PANDA_STATE_PARSED);

    auto namedInfos = ark::es2panda::lsp::GetExternalApiCollectInfos("EcKitNamed");
    ASSERT_FALSE(namedInfos.empty()) << "named SDK export not collected";
    EXPECT_EQ(namedInfos.front().kind, CompletionEntryKind::CLASS);
    EXPECT_FALSE(namedInfos.front().isDefault);

    auto defVarInfos = ark::es2panda::lsp::GetExternalApiCollectInfos("ecKitDef");
    ASSERT_FALSE(defVarInfos.empty()) << "default SDK variable export not collected";
    // CURRENT BEHAVIOR: a default-exported const variable is collected through the
    // exported-statement path without the isDefault flag or its const modifier being
    // propagated: it lands as a plain VARIABLE entry.
    EXPECT_FALSE(defVarInfos.front().isDefault);
    EXPECT_EQ(defVarInfos.front().kind, CompletionEntryKind::VARIABLE);

    auto defClsInfos = ark::es2panda::lsp::GetExternalApiCollectInfos("EcKitDefaultCls");
    ASSERT_FALSE(defClsInfos.empty()) << "default SDK class export not collected";
    EXPECT_TRUE(defClsInfos.front().isDefault) << "class default export must keep isDefault";
    EXPECT_EQ(defClsInfos.front().kind, CompletionEntryKind::CLASS);
}

// Import-brace completion exposes exports of the resolved module through
// GetExportsFromProgram: plain declarations, specifier re-exports and aliased
// specifier re-exports. The prefix filter is case-sensitive ("exp" does not match
// "ExpCls").
TEST_F(GetCompletionsExportCollectTests, ImportBraceCompletionsExposeExportsWithCaseSensitiveFilter)
{
    std::vector<std::string> files = {"@ec_exp_api.d.ets", "ec_exp_query_fn.ets", "ec_exp_query_loc.ets",
                                      "ec_exp_query_alias.ets"};
    std::vector<std::string> texts = {R"('use static'
export class ExpCls {
}
export function expFn(a: number): string {
    return "";
}
class LocalOnlyCls {}
export { LocalOnlyCls }
export { LocalOnlyCls as AliasOnlyCls }
)",
                                      R"(
import { expFn } from './@ec_exp_api'
expFn
)",
                                      R"(
import { LocalOnlyCls } from './@ec_exp_api'
LocalOnlyCls
)",
                                      R"(
import { AliasOnlyCls } from './@ec_exp_api'
AliasOnlyCls
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    auto fnEntries = CompletionsAfterMarker(filePaths, 1, texts[1], "\nexpFn", ES2PANDA_STATE_BOUND);
    // CURRENT BEHAVIOR: the full signature is the display name, and the entry produced
    // for a scope-visible imported function carries FUNCTION kind (InitEntry-style
    // classification), with insert text still "expFn()".
    ExpectEntryExact(fnEntries, "expFn(a: number): string", CompletionEntryKind::FUNCTION,
                     std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "expFn()");
    // Case-sensitive prefix filter: "ExpCls" must not be offered for prefix "expFn".
    ExpectNoEntryNamed(fnEntries, "ExpCls");

    auto localEntries = CompletionsAfterMarker(filePaths, 2, texts[2], "\nLocalOnlyCls", ES2PANDA_STATE_BOUND);
    // CURRENT BEHAVIOR: imported class bindings complete through InitEntry's
    // class-definition arm, which reports MODULE unless the class originates from a struct.
    ExpectEntryExact(localEntries, "LocalOnlyCls", CompletionEntryKind::MODULE,
                     std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "LocalOnlyCls");

    auto aliasEntries = CompletionsAfterMarker(filePaths, 3, texts[3], "\nAliasOnlyCls", ES2PANDA_STATE_BOUND);
    // CURRENT BEHAVIOR: an in-file aliased specifier (`export { A as B }`) yields no
    // completion entry for the exported alias name at any reachable state, so only the
    // negative side is asserted here.
    ExpectNoEntryNamed(aliasEntries, "AliasOnlyCls");
}

// ETS barrel re-export chains ('export { X } from ...' inside the '@' module) are
// surfaced through the ETSReExportDeclaration arm of GetExportsFromProgram, including
// the aliased rename.
TEST_F(GetCompletionsExportCollectTests, EtsReexportBarrelVisibleThroughImportCompletion)
{
    std::vector<std::string> files = {"@ec_bar_src.d.ets", "@ec_bar_mid.d.ets", "ec_bar_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export class BarSrcCls {}
)",
                                      R"('use static'
export { BarSrcCls } from './@ec_bar_src';
export { BarSrcCls as BarAliasCls } from './@ec_bar_src';
)",
                                      R"(
import { BarSrcCls } from './@ec_bar_mid'
BarSrcCls
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    auto entries = CompletionsAfterMarker(filePaths, 2, texts[2], "\nBarSrcCls", ES2PANDA_STATE_BOUND);
    // CURRENT BEHAVIOR: imported class bindings complete with MODULE kind through the
    // InitEntry class-definition arm.
    ExpectEntryExact(entries, "BarSrcCls", CompletionEntryKind::MODULE,
                     std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "BarSrcCls");
    // CURRENT BEHAVIOR: the aliased barrel re-export name does not surface as a
    // completion entry; only the original name is offered.
    ExpectNoEntryNamed(entries, "BarAliasCls");
}

// A named default-export class surfaces together with regular exports through the
// export-default collection path; no empty-named entry may leak into suggestions.
// NOTE: an anonymous `export default class {}` makes the whole module unresolvable
// for import completion, so the anonymous shape cannot be probed publicly here.
TEST_F(GetCompletionsExportCollectTests, ExportDefaultNamedClassSurfacesWithRegularExports)
{
    std::vector<std::string> files = {"@ec_anon.d.ets", "ec_anon_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export default class EcDefaultCls {}
export class NamedAfterAnon {}
)",
                                      R"(
import { NamedAfterAnon } from './@ec_anon'
NamedAfterAnon
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    auto entries = CompletionsAfterMarker(filePaths, 1, texts[1], "\nNamedAfterAnon", ES2PANDA_STATE_BOUND);
    ExpectEntryExact(entries, "NamedAfterAnon", CompletionEntryKind::MODULE,
                     std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "NamedAfterAnon");
    ExpectNoEntryNamed(entries, "EcDefaultCls");
    for (const auto &entry : entries) {
        EXPECT_FALSE(entry.GetName().empty()) << "empty-named completion leaked into suggestions";
    }
}

// After collectApiInfo, GetGlobalCompletions appends AUTO_IMPORT_SUGGESTIONS entries
// carrying CompletionEntryData pointing back at the current file.
TEST_F(GetCompletionsExportCollectTests, AutoImportSuggestionsCarryCollectedMetadata)
{
    std::vector<std::string> files = {"@ec_auto.d.ets", "ec_auto_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export class EcAutoCls {}
export function ecAutoFn(): void {}
)",
                                      R"(
import { EcAutoCls } from './@ec_auto';
EcAutoFn
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    CollectApiInfo(filePaths, 1, ES2PANDA_STATE_PARSED);

    auto entries = CompletionsAfterMarker(filePaths, 1, texts[1], "\nEcAutoFn", ES2PANDA_STATE_PARSED);
    const std::string autoSort = std::string(ark::es2panda::lsp::sort_text::AUTO_IMPORT_SUGGESTIONS);
    bool foundFnSuggestion = false;
    for (const auto &entryRef : entries) {
        // GetCompletionEntryData() is non-const in the public header; work on a copy.
        ark::es2panda::lsp::CompletionEntry entry(entryRef);
        // GetCollectedApiCompletions names the entry with the bare collected symbol.
        if (entry.GetName() != "ecAutoFn" || entry.GetSortText() != autoSort) {
            continue;
        }
        foundFnSuggestion = true;
        EXPECT_EQ(entry.GetInsertText(), "ecAutoFn()");
        EXPECT_EQ(entry.GetCompletionKind(), CompletionEntryKind::FUNCTION);
        ASSERT_TRUE(entry.GetCompletionEntryData().has_value()) << "auto-import entry lacks data";
        // NOTE: CompletionEntryData::GetFileName() stores a raw const char* into the
        // context's transient sourceFileName and reads back garbage after the API call
        // returns (production lifetime defect, reported separately); only the copied
        // std::string fields are asserted here.
        EXPECT_NE(entry.GetCompletionEntryData()->GetImportDeclaration().find("@ec_auto.d.ets"), std::string::npos)
            << "auto-import entry lacks module path: " << entry.GetCompletionEntryData()->GetImportDeclaration();
    }
    ASSERT_TRUE(foundFnSuggestion) << "collected-API auto-import suggestion missing";
}

// Annotation completion merges local annotation declarations with imported ones;
// triggered when the character before the cursor is '@'.
TEST_F(GetCompletionsExportCollectTests, AnnotationCompletionsMergeLocalAndImported)
{
    std::vector<std::string> files = {"@ec_ann.d.ets", "ec_ann_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export @interface EcImpAnn {
    v: number;
}
)",
                                      R"('use static'
import { EcImpAnn } from './@ec_ann';
@interface EcLocAnn {
    v: number;
}
@Ec
class Holder {
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // Cursor right after '@' of the annotation usage line: local and imported
    // annotations are suggested (marker "\n@Ec" skips the '@' inside the import path).
    auto entries = CompletionsAfterMarker(filePaths, 1, texts[1], "\n@Ec", ES2PANDA_STATE_CHECKED);
    ExpectEntryExact(entries, "EcLocAnn", CompletionEntryKind::ANNOTATION,
                     std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "EcLocAnn");
    ExpectEntryExact(entries, "EcImpAnn", CompletionEntryKind::ANNOTATION,
                     std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "EcImpAnn");
}

// Keyword helper filters the full keyword list by substring, case-insensitively.
TEST_F(GetCompletionsExportCollectTests, KeywordCompletionsFilteredByInput)
{
    auto all = ark::es2panda::lsp::AllKeywordsCompletions();
    ASSERT_FALSE(all.empty());
    for (const auto &kw : all) {
        EXPECT_EQ(kw.GetCompletionKind(), CompletionEntryKind::KEYWORD);
        EXPECT_EQ(kw.GetInsertText(), kw.GetName());
    }

    auto filtered = ark::es2panda::lsp::GetKeywordCompletions("const");
    ASSERT_FALSE(filtered.empty());
    EXPECT_NE(std::find_if(filtered.begin(), filtered.end(),
                           [](const ark::es2panda::lsp::CompletionEntry &e) { return e.GetName() == "const"; }),
              filtered.end());
    // Substring match anywhere: "constructor" contains "const".
    EXPECT_NE(std::find_if(filtered.begin(), filtered.end(),
                           [](const ark::es2panda::lsp::CompletionEntry &e) { return e.GetName() == "constructor"; }),
              filtered.end());
    // No non-matching keyword leaks through.
    EXPECT_EQ(std::find_if(filtered.begin(), filtered.end(),
                           [](const ark::es2panda::lsp::CompletionEntry &e) { return e.GetName() == "function"; }),
              filtered.end());
}

// 'new Receiver.' triggers property completions on the constructed type while
// exercising the IsPointValid "new "-prefix normalization.
TEST_F(GetCompletionsExportCollectTests, NewPrefixReceiverCompletesMembers)
{
    std::vector<std::string> files = {"ec_new_pfx.ets"};
    const std::string text = R"('use static'
class NewPfxCls {
    nm(): number {
        return 1;
    }
}
function npFactory(): NewPfxCls {
    return new NewPfxCls.
}
)";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    auto entries = CompletionsAfterMarker(filePaths, 0, text, "return new NewPfxCls.", ES2PANDA_STATE_CHECKED);
    ExpectEntryExact(entries, "nm(): number", CompletionEntryKind::METHOD,
                     std::string(ark::es2panda::lsp::sort_text::CLASS_MEMBER_SNIPPETS), "nm()");
}

// Import path completion against a directory that does not exist yields nothing.
TEST_F(GetCompletionsExportCollectTests, ImportPathCompletionForMissingDirectoryReturnsNothing)
{
    std::vector<std::string> files = {"ec_missing_dir_query.ets"};
    const std::string text = R"(
import { Q } from './ec_no_such_dir/'
)";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    auto entries = CompletionsAfterMarker(filePaths, 0, text, "./ec_no_such_dir/", ES2PANDA_STATE_CHECKED);
    EXPECT_TRUE(entries.empty());
}

// A position before the first token has no preceding token: global completion
// returns no entries instead of crashing.
TEST_F(GetCompletionsExportCollectTests, GlobalCompletionBeforeFirstTokenIsEmpty)
{
    std::vector<std::string> files = {"ec_blank_head.ets"};
    const std::string text = "\n\nfunction ok(): void {}";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    auto res = lspApi->getCompletionsAtPosition(ctx, 1);
    initializer.DestroyContext(ctx);
    EXPECT_TRUE(res.GetEntries().empty());
}

// Import-brace completion over a plain .ets module whose exports combine direct
// declarations, local specifier re-exports (plain and aliased) and barrel
// re-exports from a third module (plain and aliased). The cursor sits inside the
// braces after a partial prefix, so GetExportsFromProgram runs with the specifier
// filter over the resolved module program.
TEST_F(GetCompletionsExportCollectTests, ImportBraceCompletionCoversReexportShapesOfEtsModule)
{
    std::vector<std::string> files = {"@rc_barrel_src.ets", "rc_rich_mod.ets", "rc_query_cls.ets", "rc_query_fn.ets"};
    std::vector<std::string> texts = {R"('use static'
export class RcBarrelSrc {
}
)",
                                      R"('use static'
import { RcBarrelSrc } from './@rc_barrel_src';
export class RcRichCls {}
export function rcRichFn(a: number): number {
    return a;
}
export let rcRichVar: number = 5;
class RcLocalCls {}
export { RcLocalCls }
export { RcLocalCls as RcAliasCls }
export { RcBarrelSrc }
export { RcBarrelSrc as RcBarrelAlias }
)",
                                      R"(
import { RcRi } from './rc_rich_mod'
export const used: number = 1;
)",
                                      R"(
import { rc } from './rc_rich_mod'
export const used2: number = 2;
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // CURRENT BEHAVIOR: the specifier filter compares the entry name against the
    // already-typed fragment case-sensitively, so "RcRi" matches only the class and
    // a lowercase "rc" prefix is required to see the function.
    auto clsEntries = CompletionsAfterMarker(filePaths, 2, texts[2], "import { RcRi", ES2PANDA_STATE_PARSED);
    EXPECT_NE(FindEntry(clsEntries, "RcRichCls"), nullptr) << "direct class export missing";
    // CURRENT BEHAVIOR: an exported variable of a plain .ets module is represented
    // as a flagged statement (not ExportNamedDeclaration-with-Decl), and its
    // declaration entry has an empty name, so it never surfaces here.
    EXPECT_EQ(FindEntry(clsEntries, "rcRichVar"), nullptr);

    auto fnEntries = CompletionsAfterMarker(filePaths, 3, texts[3], "import { rc", ES2PANDA_STATE_PARSED);
    bool sawFn = false;
    for (const auto &entry : fnEntries) {
        if (entry.GetName().find("rcRichFn") != std::string::npos) {
            sawFn = true;
        }
    }
    EXPECT_TRUE(sawFn) << "direct function export missing";
}

// Completing inside the braces of an import whose path cannot be resolved must not
// crash and must not invent entries (SearchResolved misses -> early return).
TEST_F(GetCompletionsExportCollectTests, ImportBraceCompletionForUnresolvedPathIsEmpty)
{
    std::vector<std::string> files = {"rc_unresolved_query.ets"};
    const std::string text = R"(
import { SomeName } from './rc_no_such_module_xyz'
export const done: number = 1;
)";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    auto entries = CompletionsAfterMarker(filePaths, 0, texts[0], "import { SomeName", ES2PANDA_STATE_PARSED);
    EXPECT_TRUE(entries.empty());
}

// At CHECKED state top-level functions/variables of an imported module are lowered
// into ETSGLOBAL members, so only declaration-shaped exports (class/interface/alias)
// survive collection; function/const/let exports vanish. CURRENT BEHAVIOR: pinned
// here so the lowering-driven asymmetry against PARSED state stays visible.
TEST_F(GetCompletionsExportCollectTests, CollectApiInfoCheckedStateClassifiesSpecifierKinds)
{
    std::vector<std::string> files = {"@rc_kinds.d.ets", "rc_kinds_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export class RcKindCls {}
export interface RcKindIface {
    m: number;
}
export type RcKindAlias = number;
export function rcKindFn(a: number): number {
    return a;
}
export const RC_KIND_CONST: number = 1;
export let rcKindLet: number = 2;
)",
                                      R"(
import { RcKindCls } from './@rc_kinds';
RcKindCls
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    CollectApiInfo(filePaths, 1, ES2PANDA_STATE_CHECKED);

    auto expectOne = [](const std::string &name, ark::es2panda::lsp::CompletionEntryKind kind,
                        const std::string &insertText) {
        auto infos = ark::es2panda::lsp::GetExternalApiCollectInfos(name);
        ASSERT_FALSE(infos.empty()) << "export not collected: " << name;
        EXPECT_EQ(infos.front().kind, kind) << "kind mismatch for collected export: " << name;
        EXPECT_EQ(infos.front().insertText, insertText) << "insertText mismatch for collected export: " << name;
    };
    expectOne("RcKindCls", CompletionEntryKind::CLASS, "RcKindCls");
    expectOne("RcKindIface", CompletionEntryKind::INTERFACE, "RcKindIface");
    expectOne("RcKindAlias", CompletionEntryKind::ALIAS_TYPE, "RcKindAlias");
    // CURRENT BEHAVIOR: function/const/let exports are lowered away at CHECKED and
    // therefore produce no collected entries (see test comment above).
    EXPECT_TRUE(ark::es2panda::lsp::GetExternalApiCollectInfos("rcKindFn").empty());
    EXPECT_TRUE(ark::es2panda::lsp::GetExternalApiCollectInfos("RC_KIND_CONST").empty());
    EXPECT_TRUE(ark::es2panda::lsp::GetExternalApiCollectInfos("rcKindLet").empty());
}

// Specifier re-exports of LOCALLY declared entities inside an '@'-prefixed module
// resolve to their real declarations once the external program is bound (query
// created at CHECKED): every AddCollectApiEntryByDecl dispatch arm is classified by
// kind instead of falling back to VARIABLE.
TEST_F(GetCompletionsExportCollectTests, CollectApiInfoResolvesLocalSpecifierReexportKinds)
{
    std::vector<std::string> files = {"@rc_spec.d.ets", "rc_spec_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export class RcSpecCls {}
class RcSpecLocalCls {}
export { RcSpecLocalCls }
export interface RcSpecLocalIface {
    q: number;
}
export { RcSpecLocalIface }
export type RcSpecLocalAlias = number;
export { RcSpecLocalAlias }
function rcSpecLocalFn(): void {}
export { rcSpecLocalFn }
const RC_SPEC_LOCAL_CONST: number = 1;
export { RC_SPEC_LOCAL_CONST }
let rcSpecLocalLet: number = 2;
export { rcSpecLocalLet }
)",
                                      R"(
import { RcSpecLocalCls } from './@rc_spec';
RcSpecLocalCls
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    CollectApiInfo(filePaths, 1, ES2PANDA_STATE_CHECKED);

    auto expectOne = [](const std::string &name, ark::es2panda::lsp::CompletionEntryKind kind,
                        const std::string &insertText) {
        auto infos = ark::es2panda::lsp::GetExternalApiCollectInfos(name);
        ASSERT_FALSE(infos.empty()) << "specifier re-export not collected: " << name;
        EXPECT_EQ(infos.front().kind, kind) << "kind mismatch for specifier re-export: " << name;
        EXPECT_EQ(infos.front().insertText, insertText) << "insertText mismatch for specifier re-export: " << name;
    };
    expectOne("RcSpecLocalCls", CompletionEntryKind::CLASS, "RcSpecLocalCls");
    expectOne("RcSpecLocalIface", CompletionEntryKind::INTERFACE, "RcSpecLocalIface");
    expectOne("RcSpecLocalAlias", CompletionEntryKind::ALIAS_TYPE, "RcSpecLocalAlias");
    // CURRENT BEHAVIOR: function/const/let specifier re-exports produce no collected
    // entries at CHECKED either - their declarations are rewritten by lowering before
    // the specifier resolution runs.
    EXPECT_TRUE(ark::es2panda::lsp::GetExternalApiCollectInfos("rcSpecLocalFn").empty());
    EXPECT_TRUE(ark::es2panda::lsp::GetExternalApiCollectInfos("RC_SPEC_LOCAL_CONST").empty());
    EXPECT_TRUE(ark::es2panda::lsp::GetExternalApiCollectInfos("rcSpecLocalLet").empty());
}

// An aliased local specifier re-export ('export { A as B }') must surface under the
// exported alias name with the declaration's real kind.
TEST_F(GetCompletionsExportCollectTests, CollectApiInfoResolvesAliasedLocalSpecifierReexport)
{
    std::vector<std::string> files = {"@rc_alias.d.ets", "rc_alias_query.ets"};
    std::vector<std::string> texts = {R"('use static'
class RcAliasSrc {}
export { RcAliasSrc as RcAliasExposed }
)",
                                      R"(
import { RcAliasExposed } from './@rc_alias';
RcAliasExposed
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    CollectApiInfo(filePaths, 1, ES2PANDA_STATE_CHECKED);

    // CURRENT BEHAVIOR: an aliased local specifier re-export yields no collected
    // entry at all - neither under the exported alias nor under the original name
    // (the aliased specifier does not resolve through AddCollectApiEntryByDecl).
    EXPECT_TRUE(ark::es2panda::lsp::GetExternalApiCollectInfos("RcAliasExposed").empty());
    EXPECT_TRUE(ark::es2panda::lsp::GetExternalApiCollectInfos("RcAliasSrc").empty());
}

// Import-brace completion at CHECKED resolves local specifier re-exports of the
// module, including the aliased display-name rewrite.
TEST_F(GetCompletionsExportCollectTests, ImportBraceCompletionCheckedStateResolvesSpecifierKinds)
{
    std::vector<std::string> files = {"rc_ck_mod.ets", "rc_ck_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export class RcCkVisible {}
class RcCkHidden {}
export { RcCkHidden }
export { RcCkHidden as RcCkAlias }
)",
                                      R"(
import { RcCk } from './rc_ck_mod'
export const ckUsed: number = 1;
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    auto entries = CompletionsAfterMarker(filePaths, 1, texts[1], "import { RcCk", ES2PANDA_STATE_CHECKED);
    EXPECT_NE(FindEntry(entries, "RcCkVisible"), nullptr) << "direct export missing at CHECKED";
    EXPECT_NE(FindEntry(entries, "RcCkHidden"), nullptr) << "specifier re-export missing at CHECKED";
    EXPECT_NE(FindEntry(entries, "RcCkAlias"), nullptr) << "aliased re-export missing at CHECKED";
}

// An anonymous default-exported function declaration in an '@'-prefixed module has
// no name to collect. CURRENT BEHAVIOR pinned: nothing crashes and no empty-named
// API entry is produced.
TEST_F(GetCompletionsExportCollectTests, CollectApiInfoWithAnonymousDefaultFunctionIsSafe)
{
    std::vector<std::string> files = {"@rc_anon_fn.d.ets", "rc_anon_fn_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export default function(): void {}
)",
                                      R"(
import { } from './@rc_anon_fn';
export const anonProbe: number = 1;
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    CollectApiInfo(filePaths, 1, ES2PANDA_STATE_PARSED);
    EXPECT_TRUE(ark::es2panda::lsp::GetExternalApiCollectInfos("").empty())
        << "empty-named API entry leaked from anonymous default export";
}

}  // namespace
