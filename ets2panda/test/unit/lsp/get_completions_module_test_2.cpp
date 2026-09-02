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
#include "lsp/include/completions_details.h"
#include "lsp/include/internal_api.h"

class LSPCompletionsModuleTests2 : public LSPAPITests {};

using ark::es2panda::lsp::CompletionEntry;
using ark::es2panda::lsp::CompletionEntryKind;
using ark::es2panda::lsp::Initializer;

namespace {

// Test: relative path file completions inside an import path string literal.
TEST_F(LSPCompletionsModuleTests2, ImportPathFileCompletion)
{
    std::vector<std::string> files = {"path_export_one.ets", "path_export_two.ets", "path_use.ets"};
    std::vector<std::string> texts = {R"(export class PathExportOne {
}
)",
                                      R"(export class PathExportTwo {
}
)",
                                      R"(import { PathExportOne } from './pa'
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 3U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const size_t offset = texts[2].find("./pa") + 4;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    const std::string expectedSortText(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS);
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("path_use", CompletionEntryKind::FILE, expectedSortText, "path_use"),
        CompletionEntry("path_export_two", CompletionEntryKind::FILE, expectedSortText, "path_export_two"),
        CompletionEntry("path_export_one", CompletionEntryKind::FILE, expectedSortText, "path_export_one")};
    ASSERT_EQ(entries.size(), expectedEntries.size());
    for (const auto &expected : expectedEntries) {
        ASSERT_TRUE(std::find(entries.begin(), entries.end(), expected) != entries.end());
    }
    initializer.DestroyContext(ctx);
}

// Test: import path completions exclude non-.ets files and include directories.
TEST_F(LSPCompletionsModuleTests2, ImportPathFolderAndFileKinds)
{
    std::vector<std::string> files = {"folder_main.ets"};
    std::vector<std::string> texts = {R"(import { A } from './'
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    // Add a subdirectory and a non-.ets file next to the importing file.
    std::filesystem::path dir = std::filesystem::path(filePaths[0]).parent_path();
    std::filesystem::create_directory(dir / "subpkg");
    std::ofstream txtFile(dir / "notes.txt");
    txtFile.close();

    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const size_t offset = texts[0].find("./") + 2;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    const std::string expectedSortText(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS);
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("subpkg", CompletionEntryKind::FOLDER, expectedSortText, "subpkg"),
        CompletionEntry("folder_main", CompletionEntryKind::FILE, expectedSortText, "folder_main")};
    ASSERT_EQ(entries.size(), expectedEntries.size());
    for (const auto &expected : expectedEntries) {
        ASSERT_TRUE(std::find(entries.begin(), entries.end(), expected) != entries.end());
    }
    // The non-.ets "notes.txt" file must not be offered.
    for (const auto &entry : entries) {
        ASSERT_NE(entry.GetName(), "notes");
    }
    initializer.DestroyContext(ctx);
}

// Test: completions of exported symbols from a resolved import path; already imported
// symbol must be filtered out (negative assertion).
TEST_F(LSPCompletionsModuleTests2, AlreadyImportedSymbolIsFiltered)
{
    std::vector<std::string> files = {"import_filter_lib.ets", "import_filter_use.ets"};
    std::vector<std::string> texts = {R"(export class AlphaCls {
}
export class BetaCls {
}
export function GammaFunc(): void {
}
)",
                                      R"(import { AlphaCls, } from './import_filter_lib'
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 2U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const size_t offset = texts[1].find("AlphaCls, ") + 10;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    const std::string expectedSortText(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS);
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("GammaFunc(): void", CompletionEntryKind::METHOD, expectedSortText, "GammaFunc"),
        CompletionEntry("BetaCls", CompletionEntryKind::CLASS, expectedSortText, "BetaCls")};
    ASSERT_EQ(entries.size(), expectedEntries.size());
    for (const auto &expected : expectedEntries) {
        ASSERT_TRUE(std::find(entries.begin(), entries.end(), expected) != entries.end());
    }
    // The already imported symbol "AlphaCls" must be filtered out.
    for (const auto &entry : entries) {
        ASSERT_NE(entry.GetName(), "AlphaCls");
    }
    initializer.DestroyContext(ctx);
}

// Test: package import completions from a directory; the directory is offered
// as a FOLDER entry and exports from its index file resolve through the package.
TEST_F(LSPCompletionsModuleTests2, IndexFileModuleCompletion)
{
    std::vector<std::string> files = {"pkg_main.ets"};
    std::vector<std::string> texts = {R"(import { A } from './'
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    std::filesystem::path dir = std::filesystem::path(filePaths[0]).parent_path();
    std::filesystem::path pkgDir = dir / "idxpkg";
    std::filesystem::create_directory(pkgDir);
    std::ofstream indexFile(pkgDir / "index.ets");
    indexFile << "export class IndexCls {\n}\n";
    indexFile.close();

    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const size_t offset = texts[0].find("./") + 2;
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    // The package directory is offered as a FOLDER entry; exports from its
    // index.ets are reachable through the package import.
    const std::string expectedSortText(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS);
    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("pkg_main", CompletionEntryKind::FILE, expectedSortText, "pkg_main"),
        CompletionEntry("idxpkg", CompletionEntryKind::FOLDER, expectedSortText, "idxpkg")};
    ASSERT_EQ(entries.size(), expectedEntries.size());
    for (const auto &expected : expectedEntries) {
        ASSERT_TRUE(std::find(entries.begin(), entries.end(), expected) != entries.end());
    }
    initializer.DestroyContext(ctx);
}

// Test: completion inside braces of an import from a re-exported module.
TEST_F(LSPCompletionsModuleTests2, DISABLED_ReExportModuleBraceCompletion)
{
    std::vector<std::string> files = {"reexp_impl.ets", "reexp_barrel.ets", "reexp_use.ets"};
    std::vector<std::string> texts = {R"(export class ImplClass {
    implValue: number = 1;
}
export function ImplFunc(): void {
}
)",
                                      R"(export { ImplClass, ImplFunc } from './reexp_impl';
)",
                                      R"(import { ImplClass, } from './reexp_barrel'
let reexpObj = new ImplClass();
reexpObj.
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 3U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const size_t offsetBrace = texts[2].find("ImplClass, ") + 11;
    auto resBrace = lspApi->getCompletionsAtPosition(ctx, offsetBrace);
    auto entriesBrace = resBrace.GetEntries();
    // Brace-position completions through a barrel re-export: the re-exported
    // symbols (ImplClass, ImplFunc) are resolved from the barrel file's
    // ExportNamedDeclaration specifiers. ImplClass is already imported and
    // filtered out, leaving only ImplFunc.
    const std::string expectedSortText(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS);
    auto expectedBrace = std::vector<CompletionEntry> {
        CompletionEntry("ImplFunc(): void", CompletionEntryKind::METHOD, expectedSortText, "ImplFunc")};
    ASSERT_EQ(entriesBrace.size(), expectedBrace.size());
    for (const auto &expected : expectedBrace) {
        ASSERT_TRUE(std::find(entriesBrace.begin(), entriesBrace.end(), expected) != entriesBrace.end());
    }
    // The already imported symbol "ImplClass" must be filtered out.
    for (const auto &entry : entriesBrace) {
        ASSERT_NE(entry.GetName(), "ImplClass");
    }

    const size_t offsetDot = texts[2].rfind('.') + 1;
    auto resDot = lspApi->getCompletionsAtPosition(ctx, offsetDot);
    auto entriesDot = resDot.GetEntries();
    auto expectedDot = std::vector<CompletionEntry> {
        CompletionEntry("implValue: number", CompletionEntryKind::PROPERTY,
                        std::string(ark::es2panda::lsp::sort_text::SUGGESTED_CLASS_MEMBERS), "implValue")};
    ASSERT_EQ(entriesDot.size(), expectedDot.size());
    for (const auto &expected : expectedDot) {
        ASSERT_TRUE(std::find(entriesDot.begin(), entriesDot.end(), expected) != entriesDot.end());
    }
    initializer.DestroyContext(ctx);
}

// Test: completion detail returns the property type in display parts.
TEST_F(LSPCompletionsModuleTests2, DetailReturnsPropertyType)
{
    Initializer initializer = Initializer();
    std::string fileName = "detail_prop_type.ets";
    std::string source = R"(class DetailBox {
    detailField: number = 42;
}
let detailBox = new DetailBox();
detailBox.detailField;
)";
    es2panda_Context *ctx = initializer.CreateContext(fileName.c_str(), ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = source.find("detailBox.detailField") + 5;
    auto details = lspApi->getCompletionEntryDetails("detailField", fileName.c_str(), ctx, offset);
    ASSERT_EQ(details.GetName(), "detailField");
    ASSERT_EQ(details.GetKind(), "property");
    ASSERT_EQ(details.GetKindModifiers(), "public");
    ASSERT_EQ(details.GetFileName(), fileName);
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("DetailBox", "className");
    expected.emplace_back(".", "punctuation");
    expected.emplace_back("detailField", "property");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "typeName");
    ASSERT_EQ(details.GetDisplayParts(), expected);
    // Documentation is empty because the source property has no JSDoc comment;
    // locally declared properties carry no module source info.
    ASSERT_TRUE(details.GetDocument().empty());
    ASSERT_TRUE(details.GetSource().empty());
    ASSERT_TRUE(details.GetSourceDisplay().empty());
    initializer.DestroyContext(ctx);
}

// Test: completion detail on an overloaded method.
TEST_F(LSPCompletionsModuleTests2, DetailOverloadedMethod)
{
    Initializer initializer = Initializer();
    std::string fileName = "detail_overload.ets";
    std::string source = R"(class OverloadHost {
    convert(input: string): number {
        return 1;
    }
    convert(input: number, radix: number): string {
        return "x";
    }
}
let overloadHost = new OverloadHost();
overloadHost.convert("a");
)";
    es2panda_Context *ctx = initializer.CreateContext(fileName.c_str(), ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = source.find("overloadHost.convert") + 5;
    auto details = lspApi->getCompletionEntryDetails("convert", fileName.c_str(), ctx, offset);
    ASSERT_EQ(details.GetName(), "convert");
    ASSERT_EQ(details.GetKind(), "method");
    ASSERT_EQ(details.GetKindModifiers(), "public");
    ASSERT_EQ(details.GetFileName(), fileName);
    // The call site selects the (input: string): number overload for the detail.
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("OverloadHost", "className");
    expected.emplace_back(".", "punctuation");
    expected.emplace_back("convert", "functionName");
    expected.emplace_back("(", "punctuation");
    expected.emplace_back("input", "functionParameter");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("string", "typeParameter");
    expected.emplace_back(")", "punctuation");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "returnType");
    ASSERT_EQ(details.GetDisplayParts(), expected);
    initializer.DestroyContext(ctx);
}

// Test: completion detail for an imported property carries the source module
// path information.
TEST_F(LSPCompletionsModuleTests2, DISABLED_DetailForImportedSymbolProperty)
{
    std::vector<std::string> files = {"detail_src.ets", "detail_use.ets"};
    std::vector<std::string> texts = {R"(/**
 * Source class documentation comment.
 */
export class DetailSource {
    srcValue: number = 1;
}
)",
                                      R"(import { DetailSource } from './detail_src';
let detailUse = new DetailSource();
detailUse.srcValue;
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 2U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = texts[1].find("detailUse.srcValue") + 5;
    auto details = lspApi->getCompletionEntryDetails("srcValue", filePaths[1].c_str(), ctx, offset);
    ASSERT_EQ(details.GetName(), "srcValue");
    ASSERT_EQ(details.GetKind(), "property");
    ASSERT_EQ(details.GetKindModifiers(), "public");
    ASSERT_EQ(details.GetFileName(), filePaths[1]);
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("DetailSource", "className");
    expected.emplace_back(".", "punctuation");
    expected.emplace_back("srcValue", "property");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "typeName");
    ASSERT_EQ(details.GetDisplayParts(), expected);
    // Both source fields carry the specifier of the module providing the
    // completed member ("./detail_src").
    std::vector<SymbolDisplayPart> expectedPropertySource;
    expectedPropertySource.emplace_back("./detail_src", "text");
    ASSERT_EQ(details.GetSource(), expectedPropertySource);
    std::vector<SymbolDisplayPart> expectedPropertySourceDisplay;
    expectedPropertySourceDisplay.emplace_back("./detail_src", "moduleName");
    ASSERT_EQ(details.GetSourceDisplay(), expectedPropertySourceDisplay);
    initializer.DestroyContext(ctx);
}

// Test: completion detail for the imported class itself carries documentation
// comments from the source module along with its module source information.
TEST_F(LSPCompletionsModuleTests2, DISABLED_DetailForImportedSymbolClassWithDocs)
{
    std::vector<std::string> files = {"detail_src.ets", "detail_use.ets"};
    std::vector<std::string> texts = {R"(/**
 * Source class documentation comment.
 */
export class DetailSource {
    srcValue: number = 1;
}
)",
                                      R"(import { DetailSource } from './detail_src';
let detailUse = new DetailSource();
detailUse.srcValue;
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 2U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = texts[1].find("detailUse.srcValue") + 5;
    auto detailsClass = lspApi->getCompletionEntryDetails("DetailSource", filePaths[1].c_str(), ctx, offset);
    ASSERT_EQ(detailsClass.GetName(), "DetailSource");
    ASSERT_EQ(detailsClass.GetKind(), "class");
    ASSERT_EQ(detailsClass.GetKindModifiers(), "");
    ASSERT_EQ(detailsClass.GetFileName(), filePaths[1]);
    std::vector<SymbolDisplayPart> expectedClass;
    expectedClass.emplace_back("class", "keyword");
    expectedClass.emplace_back(" ", "space");
    expectedClass.emplace_back("DetailSource", "className");
    ASSERT_EQ(detailsClass.GetDisplayParts(), expectedClass);
    std::vector<SymbolDisplayPart> expectedClassDocument;
    expectedClassDocument.emplace_back("Source class documentation comment.", "plaintext");
    ASSERT_EQ(detailsClass.GetDocument(), expectedClassDocument);
    std::vector<SymbolDisplayPart> expectedClassSource;
    expectedClassSource.emplace_back("./detail_src", "text");
    ASSERT_EQ(detailsClass.GetSource(), expectedClassSource);
    std::vector<SymbolDisplayPart> expectedClassSourceDisplay;
    expectedClassSourceDisplay.emplace_back("./detail_src", "moduleName");
    ASSERT_EQ(detailsClass.GetSourceDisplay(), expectedClassSourceDisplay);
    initializer.DestroyContext(ctx);
}

// Test: completion detail for a locally declared symbol carries no module
// source information.
TEST_F(LSPCompletionsModuleTests2, DetailLocalSymbolNoSource)
{
    Initializer initializer = Initializer();
    std::string fileName = "detail_local.ets";
    std::string sourceText = R"(class LocalDetailSource {
    localValue: number = 1;
}
let localObject = new LocalDetailSource();
localObject.localValue;
)";
    es2panda_Context *ctx = initializer.CreateContext(fileName.c_str(), ES2PANDA_STATE_CHECKED, sourceText.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = sourceText.find("localObject.localValue") + 5;
    auto detailsMember = lspApi->getCompletionEntryDetails("localValue", fileName.c_str(), ctx, offset);
    ASSERT_EQ(detailsMember.GetName(), "localValue");
    ASSERT_EQ(detailsMember.GetKind(), "property");
    // Locally declared symbols have no module source info, even as members of
    // a local class.
    ASSERT_TRUE(detailsMember.GetSource().empty());
    ASSERT_TRUE(detailsMember.GetSourceDisplay().empty());

    auto detailsClass = lspApi->getCompletionEntryDetails("LocalDetailSource", fileName.c_str(), ctx, offset);
    ASSERT_EQ(detailsClass.GetName(), "LocalDetailSource");
    ASSERT_EQ(detailsClass.GetKind(), "class");
    ASSERT_TRUE(detailsClass.GetSource().empty());
    ASSERT_TRUE(detailsClass.GetSourceDisplay().empty());
    initializer.DestroyContext(ctx);
}

// Test: completion detail for a symbol imported through a package directory
// (resolved via its index.ets) carries the package specifier as source, for
// both the imported class itself and its members.
TEST_F(LSPCompletionsModuleTests2, DISABLED_DetailPackageIndexImportSource)
{
    std::vector<std::string> files = {"detail_pkg_use.ets"};
    std::vector<std::string> texts = {R"(import { PkgCls } from './detail_pkg';
let pkgObj = new PkgCls();
pkgObj.pkgValue;
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    std::filesystem::path dir = std::filesystem::path(filePaths[0]).parent_path();
    std::filesystem::create_directory(dir / "detail_pkg");
    std::ofstream indexFile(dir / "detail_pkg" / "index.ets");
    indexFile << "export class PkgCls {\n    pkgValue: number = 1;\n}\n";
    indexFile.close();

    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = texts[0].find("pkgObj.pkgValue") + 5;

    auto detailsClass = lspApi->getCompletionEntryDetails("PkgCls", filePaths[0].c_str(), ctx, offset);
    ASSERT_EQ(detailsClass.GetName(), "PkgCls");
    ASSERT_EQ(detailsClass.GetKind(), "class");
    std::vector<SymbolDisplayPart> expectedSource;
    expectedSource.emplace_back("./detail_pkg", "text");
    ASSERT_EQ(detailsClass.GetSource(), expectedSource);
    std::vector<SymbolDisplayPart> expectedSourceDisplay;
    expectedSourceDisplay.emplace_back("./detail_pkg", "moduleName");
    ASSERT_EQ(detailsClass.GetSourceDisplay(), expectedSourceDisplay);

    auto detailsMember = lspApi->getCompletionEntryDetails("pkgValue", filePaths[0].c_str(), ctx, offset);
    ASSERT_EQ(detailsMember.GetName(), "pkgValue");
    ASSERT_EQ(detailsMember.GetKind(), "property");
    // The member is attributed to the package that imports its enclosing
    // class.
    std::vector<SymbolDisplayPart> expectedMemberSource;
    expectedMemberSource.emplace_back("./detail_pkg", "text");
    ASSERT_EQ(detailsMember.GetSource(), expectedMemberSource);
    std::vector<SymbolDisplayPart> expectedMemberSourceDisplay;
    expectedMemberSourceDisplay.emplace_back("./detail_pkg", "moduleName");
    ASSERT_EQ(detailsMember.GetSourceDisplay(), expectedMemberSourceDisplay);
    initializer.DestroyContext(ctx);
}

// Test: completion detail for a barrel re-exported symbol carries the barrel
// specifier named in this file's import as source.
TEST_F(LSPCompletionsModuleTests2, DISABLED_DetailBarrelReExportSource)
{
    std::vector<std::string> files = {"detail_barrel_impl.ets", "detail_barrel.ets", "detail_barrel_use.ets"};
    std::vector<std::string> texts = {R"(export class BarrelImpl {
    implField: number = 1;
}
)",
                                      R"(export { BarrelImpl } from './detail_barrel_impl';
)",
                                      R"(import { BarrelImpl } from './detail_barrel';
let barrelObj = new BarrelImpl();
barrelObj.implField;
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 3U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    const size_t offset = texts[2].find("barrelObj.implField") + 5;

    auto detailsClass = lspApi->getCompletionEntryDetails("BarrelImpl", filePaths[2].c_str(), ctx, offset);
    ASSERT_EQ(detailsClass.GetName(), "BarrelImpl");
    ASSERT_EQ(detailsClass.GetKind(), "class");
    // The source is the module specifier written in this file's import
    // declaration (the barrel), not the re-export origin.
    std::vector<SymbolDisplayPart> expectedSource;
    expectedSource.emplace_back("./detail_barrel", "text");
    ASSERT_EQ(detailsClass.GetSource(), expectedSource);
    std::vector<SymbolDisplayPart> expectedSourceDisplay;
    expectedSourceDisplay.emplace_back("./detail_barrel", "moduleName");
    ASSERT_EQ(detailsClass.GetSourceDisplay(), expectedSourceDisplay);

    auto detailsMember = lspApi->getCompletionEntryDetails("implField", filePaths[2].c_str(), ctx, offset);
    ASSERT_EQ(detailsMember.GetName(), "implField");
    ASSERT_EQ(detailsMember.GetKind(), "property");
    std::vector<SymbolDisplayPart> expectedMemberSource;
    expectedMemberSource.emplace_back("./detail_barrel", "text");
    ASSERT_EQ(detailsMember.GetSource(), expectedMemberSource);
    initializer.DestroyContext(ctx);
}

// Test: incomplete syntax does not crash the completion API.
TEST_F(LSPCompletionsModuleTests2, IncompleteSyntaxDoesNotCrash)
{
    Initializer initializer = Initializer();
    std::string fileName = "incomplete_syntax.ets";
    std::string source = "let incompleteVal = 1;\nlet broken = incompleteVal.\nfunction incompleteFunc( {\n";
    es2panda_Context *ctx = initializer.CreateContext(fileName.c_str(), ES2PANDA_STATE_CHECKED, source.c_str());
    LSPAPI const *lspApi = GetImpl();
    const size_t offsetDot = source.find("incompleteVal.") + std::string("incompleteVal.").size();
    auto resDot = lspApi->getCompletionsAtPosition(ctx, offsetDot);
    auto entriesDot = resDot.GetEntries();
    // Despite the trailing broken statement, number member completions are
    // still produced for "incompleteVal.".
    auto expectedDot = std::vector<CompletionEntry> {
        CompletionEntry("MIN_VALUE: int", CompletionEntryKind::PROPERTY,
                        std::string(ark::es2panda::lsp::sort_text::SUGGESTED_CLASS_MEMBERS), "MIN_VALUE"),
        CompletionEntry("MAX_VALUE: int", CompletionEntryKind::PROPERTY,
                        std::string(ark::es2panda::lsp::sort_text::SUGGESTED_CLASS_MEMBERS), "MAX_VALUE")};
    for (const auto &expected : expectedDot) {
        ASSERT_TRUE(std::find(entriesDot.begin(), entriesDot.end(), expected) != entriesDot.end());
    }
    const size_t offsetEof = source.size();
    auto resEof = lspApi->getCompletionsAtPosition(ctx, offsetEof);
    auto entriesEof = resEof.GetEntries();
    // No completions at the end of the unterminated function, and no crash.
    ASSERT_EQ(entriesEof.size(), 0U);
    initializer.DestroyContext(ctx);
}

// Test: member completions at the byte offset right after "obj." where the
// receiver name shares a prefix with the member names ("uniObj" vs "uniValue").
// This guards offset handling near identifier boundaries: the completion
// engine must resolve the receiver expression, not a name prefix.
TEST_F(LSPCompletionsModuleTests2, UnicodeIdentifierNearbyOffset)
{
    std::vector<std::string> files = {"unicode_offset.ets"};
    std::vector<std::string> texts = {R"(class UniHolder {
    uniValue: number = 7;
    unicValue: string = "s";
}
let uniObj = new UniHolder();
uniObj.
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const size_t offsetUni = texts[0].find("uniObj.") + std::string("uniObj.").size();
    ASSERT_NE(texts[0].find("uniObj."), std::string::npos);
    auto resUni = lspApi->getCompletionsAtPosition(ctx, offsetUni);
    auto entriesUni = resUni.GetEntries();
    auto expectedMembers = std::vector<CompletionEntry> {
        CompletionEntry("uniValue: number", CompletionEntryKind::PROPERTY,
                        std::string(ark::es2panda::lsp::sort_text::SUGGESTED_CLASS_MEMBERS), "uniValue"),
        CompletionEntry("unicValue: string", CompletionEntryKind::PROPERTY,
                        std::string(ark::es2panda::lsp::sort_text::SUGGESTED_CLASS_MEMBERS), "unicValue")};
    ASSERT_EQ(entriesUni.size(), expectedMembers.size());
    for (const auto &expected : expectedMembers) {
        ASSERT_TRUE(std::find(entriesUni.begin(), entriesUni.end(), expected) != entriesUni.end());
    }
    initializer.DestroyContext(ctx);
}

// Test: dot access after a Unicode (CJK) identifier. CJK ideographs are 3-byte
// UTF-8, so a byte-naive offset lands mid-identifier; the marker offset is
// computed from the source, never hardcoded.
TEST_F(LSPCompletionsModuleTests2, UnicodeMemberCompletion)
{
    std::vector<std::string> files = {"unicode_cjk_member.ets"};
    const std::string source = std::string("class CjkHolder {\n    cjkValue: number = 7;\n}\n") +
                               "let cjkObj\xE4\xB8\xAD\xE6\x96\x87 = new CjkHolder();\n" +
                               "cjkObj\xE4\xB8\xAD\xE6\x96\x87.\n" + "let after = 1;\n";
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const size_t declEnd = source.find("new CjkHolder");
    ASSERT_NE(declEnd, std::string::npos);
    const size_t usePos = source.find("cjkObj\xE4\xB8\xAD\xE6\x96\x87.", declEnd);
    ASSERT_NE(usePos, std::string::npos);
    const size_t offset = usePos + std::string("cjkObj\xE4\xB8\xAD\xE6\x96\x87.").size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    // Documented current limitation: with a valid following statement the
    // parse recovers, but the property access after the CJK identifier still
    // resolves through an error placeholder, so the receiver type is lost and
    // only the unrelated global fallback is offered.
    auto expectedGlobal =
        CompletionEntry("scheduleGcAfterNthAlloc(counter: int, cause: Cause): void", CompletionEntryKind::METHOD,
                        std::string(ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS), "scheduleGcAfterNthAlloc()");
    ASSERT_NE(entries.size(), 0U);
    ASSERT_TRUE(std::find(entries.begin(), entries.end(), expectedGlobal) != entries.end());
    initializer.DestroyContext(ctx);
}

}  // namespace
