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
#include "lsp/include/symbol_reference_index.h"
#include "public/es2panda_lib.h"

#include <filesystem>
#include <fstream>
#include <unordered_set>

class LSPCompletions2Tests : public LSPAPITests {
public:
    // Returns all completion entries at the position right after the first occurrence of marker in text.
    std::vector<ark::es2panda::lsp::CompletionEntry> GetCompletionsAfterMarker(const std::string &fileName,
                                                                               const std::string &text,
                                                                               const std::string &marker)
    {
        std::vector<std::string> files = {fileName};
        std::vector<std::string> texts = {text};
        auto filePaths = CreateTempFile(files, texts);
        if (filePaths.size() != 1U) {
            ADD_FAILURE() << "Expected one temporary file";
            return {};
        }
        auto markerPos = text.find(marker);
        if (markerPos == std::string::npos) {
            ADD_FAILURE() << "Completion marker not found";
            return {};
        }
        size_t const offset = markerPos + marker.size();
        LSPAPI const *lspApi = GetImpl();
        ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
        auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
        if (ctx == nullptr) {
            ADD_FAILURE() << "Failed to create LSP context";
            return {};
        }
        auto entries = lspApi->getCompletionsAtPosition(ctx, offset).GetEntries();
        initializer.DestroyContext(ctx);
        return entries;
    }

    // Finds the first entry with the exact completion name and verifies kind, sortText and insertText.
    const ark::es2panda::lsp::CompletionEntry &ExpectEntry(
        const std::vector<ark::es2panda::lsp::CompletionEntry> &entries, const std::string &name,
        ark::es2panda::lsp::CompletionEntryKind kind, const std::string &sortText, const std::string &insertText)
    {
        for (const auto &entry : entries) {
            if (entry.GetName() == name) {
                EXPECT_EQ(entry.GetCompletionKind(), kind) << "kind mismatch for entry: " << name;
                EXPECT_EQ(entry.GetSortText(), sortText) << "sortText mismatch for entry: " << name;
                EXPECT_EQ(entry.GetInsertText(), insertText) << "insertText mismatch for entry: " << name;
                return entry;
            }
        }
        ADD_FAILURE() << "Expected completion entry not found: " << name;
        // NOLINTNEXTLINE(readability-identifier-naming)
        static const ark::es2panda::lsp::CompletionEntry emptyEntry;
        return emptyEntry;
    }

    void ExpectNoEntryWithName(const std::vector<ark::es2panda::lsp::CompletionEntry> &entries, const std::string &name)
    {
        for (const auto &entry : entries) {
            EXPECT_NE(entry.GetName(), name) << "Unexpected completion entry found: " << name;
        }
    }

    void ExpectNoActionlessBareEntry(const std::vector<ark::es2panda::lsp::CompletionEntry> &entries,
                                     const std::string &name)
    {
        for (auto entry : entries) {
            auto insertText = entry.GetInsertText().empty() ? entry.GetName() : entry.GetInsertText();
            EXPECT_FALSE(insertText == name && !entry.GetHasAction()) << "Unusable bare-name completion: " << name;
        }
    }

    void ExpectCompletedSourceResolves(const std::string &queryPath, const std::string &text, const std::string &name,
                                       const std::string &sourcePath, size_t declarationOffset)
    {
        ark::es2panda::lsp::Initializer initializer;
        auto ctx = initializer.CreateContext(queryPath.c_str(), ES2PANDA_STATE_CHECKED, text.c_str());
        ASSERT_NE(ctx, nullptr);
        EXPECT_TRUE(GetImpl()->getSyntacticDiagnostics(ctx).diagnostic.empty());
        for (const auto &diagnostic : GetImpl()->getSemanticDiagnostics(ctx).diagnostic) {
            EXPECT_NE(diagnostic.severity_, DiagnosticSeverity::Error) << diagnostic.message_;
        }
        auto usage = text.rfind(name);
        ASSERT_NE(usage, std::string::npos);
        auto definition = GetImpl()->getDefinitionAtPosition(ctx, usage);
        EXPECT_EQ(definition.fileName, sourcePath);
        EXPECT_EQ(definition.start, declarationOffset);
        EXPECT_EQ(definition.length, name.size());
        initializer.DestroyContext(ctx);
    }

    void ExpectAutoImportEntry(const std::vector<ark::es2panda::lsp::CompletionEntry> &entries, const std::string &name,
                               ark::es2panda::lsp::CompletionEntryKind kind, const std::string &insertText,
                               const std::string &source)
    {
        auto entry = ExpectEntry(entries, name, kind,
                                 std::string(ark::es2panda::lsp::sort_text::AUTO_IMPORT_SUGGESTIONS), insertText);
        EXPECT_TRUE(entry.GetHasAction()) << "auto-import action missing for entry: " << name;
        ASSERT_TRUE(entry.GetCompletionEntryData().has_value()) << "auto-import data missing for entry: " << name;
        EXPECT_EQ(entry.GetCompletionEntryData()->GetNamedExport(), name);
        EXPECT_NE(entry.GetCompletionEntryData()->GetImportDeclaration().find(source), std::string::npos);

        for (auto candidate : entries) {
            if (candidate.GetInsertText() == insertText) {
                EXPECT_TRUE(candidate.GetHasAction()) << "actionless SDK export leaked as a global: " << name;
            }
        }
    }
    static std::string FindStdLibSourcePath(ark::es2panda::public_lib::Context *ctx)
    {
        std::string sourcePath;
        ctx->parserProgram->GetExternalPrograms()->Visit([&sourcePath](auto *program) {
            auto source = std::string(program->SourceFilePath());
            if (source.find("etsstdlib.abc") != std::string::npos) {
                sourcePath = std::move(source);
            }
        });
        return sourcePath;
    }

    // Returns true when an entry with the exact completion name exists.
    static bool HasEntryWithName(const std::vector<ark::es2panda::lsp::CompletionEntry> &entries,
                                 const std::string &name)
    {
        for (const auto &entry : entries) {
            if (entry.GetName() == name) {
                return true;
            }
        }
        return false;
    }
};

namespace {

using ark::es2panda::lsp::CompletionEntryKind;
using ark::es2panda::lsp::sort_text::AUTO_IMPORT_SUGGESTIONS;
using ark::es2panda::lsp::sort_text::CLASS_MEMBER_SNIPPETS;
using ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS;
using ark::es2panda::lsp::sort_text::MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT;
using ark::es2panda::lsp::sort_text::SUGGESTED_CLASS_MEMBERS;

// Local variables: a let binding and a const binding in a function scope are both completable.
TEST_F(LSPCompletions2Tests, LocalVariablesInFunctionScope)
{
    const std::string text = R"delimiter(
function testLocal(): void {
    let localNum: number = 1;
    const localStr: string = "s";
    loc
}
)delimiter";
    auto entries = GetCompletionsAfterMarker("local_variables.ets", text, "    loc");
    ExpectEntry(entries, "localNum: number", CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                "localNum");
    ExpectEntry(entries, "localStr: string", CompletionEntryKind::CONSTANT, std::string(GLOBALS_OR_KEYWORDS),
                "localStr");
}

// this. inside the declaring class offers private, public and protected members.
TEST_F(LSPCompletions2Tests, ThisMembersIncludePrivateAndProtectedInsideClass)
{
    const std::string text = R"delimiter(
class VaultTwo {
    private secretNum: number = 42;
    public visibleNum: number = 1;
    protected protNum: number = 2;
    reveal(): number {
        return this.
    }
}
)delimiter";
    auto entries = GetCompletionsAfterMarker("this_members_private.ets", text, "this.");
    ASSERT_EQ(entries.size(), 4U);
    ExpectEntry(entries, "secretNum: number", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS),
                "secretNum");
    ExpectEntry(entries, "visibleNum: number", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS),
                "visibleNum");
    ExpectEntry(entries, "protNum: number", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS),
                "protNum");
    ExpectEntry(entries, "reveal(): number", CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS),
                "reveal()");
}

// Accessing the class name offers static members only, not instance members.
TEST_F(LSPCompletions2Tests, StaticMembersOfferedOnClassName)
{
    const std::string text = R"delimiter(
class RegistryCls {
    static staticCount: number = 0;
    static resetCount(): void {}
    instanceName: string = "";
}
RegistryCls.
)delimiter";
    auto entries = GetCompletionsAfterMarker("static_members.ets", text, "RegistryCls.");
    ASSERT_EQ(entries.size(), 2U);
    ExpectEntry(entries, "staticCount: number", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS),
                "staticCount");
    ExpectEntry(entries, "resetCount(): void", CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS),
                "resetCount()");
}

// obj. on a derived class instance offers both own and inherited public members.
TEST_F(LSPCompletions2Tests, InheritedMembersOfferedOnDerivedInstance)
{
    const std::string text = R"delimiter(
class BaseVeh {
    baseSpeed: number = 1;
    driveBase(): void {}
}
class CarVeh extends BaseVeh {
    carModel: string = "m";
    driveCar(): void {}
}
let carVehObj = new CarVeh();
carVehObj.
)delimiter";
    auto entries = GetCompletionsAfterMarker("inherited_members.ets", text, "carVehObj.");
    ASSERT_EQ(entries.size(), 4U);
    ExpectEntry(entries, "carModel: string", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS),
                "carModel");
    ExpectEntry(entries, "driveCar(): void", CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS),
                "driveCar()");
    ExpectEntry(entries, "baseSpeed: number", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS),
                "baseSpeed");
    ExpectEntry(entries, "driveBase(): void", CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS),
                "driveBase()");
}

// Ns. offers exported namespace members: variable, const, function and class, but not unexported ones.
TEST_F(LSPCompletions2Tests, NamespaceMembersOfferedAfterQualifier)
{
    const std::string text = R"delimiter(
namespace GeoNs {
    export let nsScale: number = 1;
    export const nsOrigin: string = "0";
    export function nsRender(): void {}
    export class NsWidget {}
    let nsHidden: number = 2;
}
let nsUse = GeoNs.
)delimiter";
    auto entries = GetCompletionsAfterMarker("namespace_members.ets", text, "GeoNs.");
    ASSERT_EQ(entries.size(), 4U);
    ExpectEntry(entries, "nsScale: number", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS),
                "nsScale");
    ExpectEntry(entries, "nsOrigin: string", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS),
                "nsOrigin");
    ExpectEntry(entries, "nsRender(): void", CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS),
                "nsRender()");
    ExpectEntry(entries, "NsWidget", CompletionEntryKind::CLASS, std::string(MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT),
                "NsWidget");
}

// EnumName.N offers the enum members filtered by the typed prefix.
TEST_F(LSPCompletions2Tests, EnumMembersFilteredByPrefix)
{
    const std::string text = R"delimiter(
enum DirectionTwo {
    North,
    South
}
let dirTwo = DirectionTwo.N
)delimiter";
    auto entries = GetCompletionsAfterMarker("enum_members.ets", text, "DirectionTwo.N");
    ASSERT_EQ(entries.size(), 1U);
    ExpectEntry(entries, "North", CompletionEntryKind::ENUM_MEMBER, std::string(MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT),
                "North");
}

// Type position: both the interface and the class matching the prefix are offered.
TEST_F(LSPCompletions2Tests, TypePositionOffersInterfacesAndClasses)
{
    const std::string text = R"delimiter(
interface ShaCircle {
    radius: number;
}
class ShaBox {
    side: number = 1;
}
let shaVar: Sha
)delimiter";
    auto entries = GetCompletionsAfterMarker("type_position.ets", text, "let shaVar: Sha");
    ExpectEntry(entries, "ShaCircle", CompletionEntryKind::KEYWORD, std::string(GLOBALS_OR_KEYWORDS), "ShaCircle");
    ExpectEntry(entries, "ShaBox", CompletionEntryKind::MODULE, std::string(GLOBALS_OR_KEYWORDS), "ShaBox");
}

// Expression position: the interface is filtered out, only the class (value symbol) remains.
TEST_F(LSPCompletions2Tests, ExpressionPositionFiltersOutInterfaces)
{
    const std::string text = R"delimiter(
interface ShaCircle {
    radius: number;
}
class ShaBox {
    side: number = 1;
}
let shaVar = Sha
)delimiter";
    auto entries = GetCompletionsAfterMarker("expression_position.ets", text, "let shaVar = Sha");
    ExpectEntry(entries, "ShaBox", CompletionEntryKind::MODULE, std::string(GLOBALS_OR_KEYWORDS), "ShaBox");
    ExpectNoEntryWithName(entries, "ShaCircle");
}

// Keyword completion inside a function body: "ret" completes to the "return" keyword.
TEST_F(LSPCompletions2Tests, KeywordCompletionInsideFunctionBody)
{
    const std::string text = R"delimiter(
function kwTest(): number {
    ret
}
)delimiter";
    auto entries = GetCompletionsAfterMarker("keyword_in_body.ets", text, "    ret");
    ASSERT_EQ(entries.size(), 1U);
    ExpectEntry(entries, "return", CompletionEntryKind::KEYWORD, std::string(GLOBALS_OR_KEYWORDS), "return");
}

// Keyword completion is absent after a member access point: only property completions are returned.
TEST_F(LSPCompletions2Tests, NoKeywordCompletionAfterMemberAccessPoint)
{
    const std::string text = R"delimiter(
class KwPoint {
    returnValue: number = 1;
}
let kwPoint = new KwPoint();
kwPoint.
)delimiter";
    auto entries = GetCompletionsAfterMarker("keyword_after_point.ets", text, "kwPoint.");
    ASSERT_EQ(entries.size(), 1U);
    ExpectEntry(entries, "returnValue: number", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS),
                "returnValue");
    ExpectNoEntryWithName(entries, "return");
}

// obj?. completion is currently blocked by the parser/lowering: the ETS
// parser desugars `obj?.` into a synthetic expression with a gensym
// identifier (`gensym%%_1`) that carries no type information (tsType=nil).
// The LSP layer cannot recover the original variable's type or declaration
// from the lowered AST.  Fixing this requires the parser to preserve the
// original MemberExpression (with optional=true) or the lowering to annotate
// the synthetic identifier with the receiver's type.  Without one of those
// changes, any LSP-level fix would be a speculative workaround, not a
// correct semantic completion.
TEST_F(LSPCompletions2Tests, OptionalChainingMemberAccessReturnsNoCompletions)
{
    const std::string text = R"delimiter(
class OptWidget {
    optSize: number = 1;
    optDraw(): void {}
}
let optW: OptWidget | undefined = new OptWidget();
optW?.
)delimiter";
    auto entries = GetCompletionsAfterMarker("optional_chaining.ets", text, "optW?.");
    ASSERT_TRUE(entries.empty());
}

// Generic type instantiation: members of BoxTwo<number> are offered with the declared signatures.
TEST_F(LSPCompletions2Tests, GenericTypeMembersOfferedWithSubstitution)
{
    const std::string text = R"delimiter(
class BoxTwo<T> {
    content: T;
    boxSize: number = 0;
    peekContent(): T {
        return this.content;
    }
}
let numBox = new BoxTwo<number>();
numBox.
)delimiter";
    auto entries = GetCompletionsAfterMarker("generic_members.ets", text, "numBox.");
    ASSERT_EQ(entries.size(), 3U);
    ExpectEntry(entries, "content: T", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS), "content");
    ExpectEntry(entries, "boxSize: number", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS),
                "boxSize");
    ExpectEntry(entries, "peekContent(): T", CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS),
                "peekContent()");
}

// Intersection type receiver: the ETS parser replaces & with BrokenTypeNode at parse time
// (type annotation lost); the LSP layer cannot recover constituent
// types from the checked AST.  Currently no completions are returned.
// Context: ETS checker does not support TSIntersectionType; ETSAnalyzer::Check(TSIntersectionType)
// is ES2PANDA_UNREACHABLE (__builtin_unreachable in release).  Fixing this requires
// checker-level support for intersection types, not just LSP-layer changes.
TEST_F(LSPCompletions2Tests, IntersectionTypeMemberAccessReturnsNoCompletions)
{
    const std::string text = R"delimiter(
class AlphaTwo {
    alphaField: number = 1;
    alphaMethod(): void {}
}
class BetaTwo {
    betaField: string = "b";
    betaMethod(): void {}
}
function useBoth(v: AlphaTwo & BetaTwo): void {
    v.
}
)delimiter";
    auto entries = GetCompletionsAfterMarker("intersection_members.ets", text, "    v.");
    ASSERT_TRUE(entries.empty());
}

// Union type receiver: only members with matching name and signature across all union parts are offered.
TEST_F(LSPCompletions2Tests, UnionTypeOffersCommonMembersOnly)
{
    const std::string text = R"delimiter(
class UnionA {
    sharedProp: number = 1;
    sharedMethod(a: number): void {}
    onlyA: string = "a";
}
class UnionB {
    sharedProp: number = 2;
    sharedMethod(x: number): void {}
    onlyB: string = "b";
}
function useUnion(u: UnionA | UnionB): void {
    u.
}
)delimiter";
    auto entries = GetCompletionsAfterMarker("union_members.ets", text, "    u.");
    ASSERT_EQ(entries.size(), 2U);
    ExpectEntry(entries, "sharedProp: number", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS),
                "sharedProp");
    ExpectEntry(entries, "sharedMethod(a: number): void", CompletionEntryKind::METHOD,
                std::string(CLASS_MEMBER_SNIPPETS), "sharedMethod()");
}

// this. inside a derived class method offers the protected member inherited from the base class.
TEST_F(LSPCompletions2Tests, ProtectedMemberVisibleInDerivedClassViaThis)
{
    const std::string text = R"delimiter(
class ProtBase {
    protected heritageTwo: number = 1;
}
class ProtDerived extends ProtBase {
    getHeritage(): number {
        return this.
    }
}
)delimiter";
    auto entries = GetCompletionsAfterMarker("protected_in_derived.ets", text, "this.");
    ASSERT_EQ(entries.size(), 2U);
    ExpectEntry(entries, "heritageTwo: number", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS),
                "heritageTwo");
    ExpectEntry(entries, "getHeritage(): number", CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS),
                "getHeritage()");
}

// An already imported symbol makes its sibling module available to the symbol index.
// Completing another exported class from that module should offer an auto-import action.
TEST_F(LSPCompletions2Tests, AutoImportSiblingExportFromImportedModule)
{
    std::vector<std::string> files = {"a.ets", "b.ets"};
    std::vector<std::string> texts = {R"(export class aaa {}
export class bbb {}
)",
                                      R"(import {aaa} from './a.ets'
bb
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    lspApi->buildSymbolReferenceIndexForContextWithExternal(ctx);
    ASSERT_TRUE(lspApi->collectApiInfo(ctx));

    auto markerPos = texts[1].rfind("bb");
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + std::string("bb").size();
    auto entries = lspApi->getCompletionsAtPosition(ctx, offset).GetEntries();

    auto entry = ExpectEntry(entries, "bbb", CompletionEntryKind::CLASS, std::string(AUTO_IMPORT_SUGGESTIONS), "bbb");
    EXPECT_TRUE(entry.GetHasAction());
    ASSERT_TRUE(entry.GetCompletionEntryData().has_value());
    EXPECT_EQ(entry.GetCompletionEntryData()->GetNamedExport(), "bbb");
    EXPECT_NE(entry.GetCompletionEntryData()->GetImportDeclaration().find("a.ets"), std::string::npos);

    initializer.DestroyContext(ctx);
}

// Exports from an imported SDK declaration module must remain auto-import candidates.
// They must not be exposed as actionless global completions merely because their module is external.
TEST_F(LSPCompletions2Tests, DISABLED_AutoImportExportFromImportedSdkDeclarationModule)
{
    std::vector<std::string> files = {"@ohos.file.fs.d.ets", "sdk_auto_import_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export interface TaskSignal {}
export class TaskController {}
export type TaskAlias = number;
export function taskFactory(): number {
    return 1;
}
export let taskValue: number = 1;
export const taskLimit: number = 2;
export default class FileIo {}
)",
                                      R"(import fileIo from './@ohos.file.fs.d.ets'
Task
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContextWithExternal(ctx));
    ASSERT_TRUE(lspApi->collectApiInfo(ctx));

    const std::string marker = "Task";
    auto markerPos = texts[1].rfind(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();
    auto entries = lspApi->getCompletionsAtPosition(ctx, offset).GetEntries();

    const std::string source = "@ohos.file.fs.d.ets";
    ExpectAutoImportEntry(entries, "TaskSignal", CompletionEntryKind::INTERFACE, "TaskSignal", source);
    ExpectAutoImportEntry(entries, "TaskController", CompletionEntryKind::CLASS, "TaskController", source);
    ExpectAutoImportEntry(entries, "TaskAlias", CompletionEntryKind::ALIAS_TYPE, "TaskAlias", source);
    ExpectAutoImportEntry(entries, "taskFactory", CompletionEntryKind::FUNCTION, "taskFactory()", source);
    ExpectAutoImportEntry(entries, "taskValue", CompletionEntryKind::VARIABLE, "taskValue", source);
    ExpectAutoImportEntry(entries, "taskLimit", CompletionEntryKind::CONSTANT, "taskLimit", source);

    initializer.DestroyContext(ctx);
}

// An interface exported by another indexed project file should be offered as an
// auto-import completion even when the two files have no import relationship.
TEST_F(LSPCompletions2Tests, AutoImportInterfaceFromUnrelatedProjectFile)
{
    std::vector<std::string> files = {"auto_import_interface_source.ets", "auto_import_interface_query.ets"};
    std::vector<std::string> texts = {R"(export interface ImportedAaaInterface {}
export interface ImportedBbbInterface {}
)",
                                      R"(
let value: ImportedBbb
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::Initializer initializer;
    auto sourceCtx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto queryCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(sourceCtx, nullptr);
    ASSERT_NE(queryCtx, nullptr);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContextWithExternal(sourceCtx));
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContextWithExternal(queryCtx));
    ASSERT_TRUE(lspApi->collectApiInfo(queryCtx));

    const std::string marker = "ImportedBbb";
    auto markerPos = texts[1].rfind(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();
    auto entries = lspApi->getCompletionsAtPosition(queryCtx, offset).GetEntries();

    auto entry = ExpectEntry(entries, "ImportedBbbInterface", CompletionEntryKind::INTERFACE,
                             std::string(AUTO_IMPORT_SUGGESTIONS), "ImportedBbbInterface");
    EXPECT_TRUE(entry.GetHasAction());
    ASSERT_TRUE(entry.GetCompletionEntryData().has_value());
    EXPECT_EQ(entry.GetCompletionEntryData()->GetNamedExport(), "ImportedBbbInterface");
    EXPECT_NE(entry.GetCompletionEntryData()->GetImportDeclaration().find("auto_import_interface_source.ets"),
              std::string::npos);

    initializer.DestroyContext(sourceCtx);
    initializer.DestroyContext(queryCtx);
}

// A project export that has the same name as a directly available system symbol
// must not replace the system completion with an auto-import action.
TEST_F(LSPCompletions2Tests, IndexedExportDoesNotOverrideSameNamedSystemCompletion)
{
    std::vector<std::string> files = {"readonly_array_shadow.ets", "readonly_array_query.ets"};
    std::vector<std::string> texts = {R"(export class ReadonlyArray {}
)",
                                      R"(
let value: Readonly
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::Initializer initializer;
    auto sourceCtx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto queryCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(sourceCtx, nullptr);
    ASSERT_NE(queryCtx, nullptr);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContextWithExternal(sourceCtx));
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContextWithExternal(queryCtx));
    ASSERT_TRUE(lspApi->collectApiInfo(queryCtx));

    const std::string marker = "Readonly";
    auto markerPos = texts[1].rfind(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();
    auto entries = lspApi->getCompletionsAtPosition(queryCtx, offset).GetEntries();

    auto entry = ExpectEntry(entries, "ReadonlyArray", CompletionEntryKind::INTERFACE, std::string(GLOBALS_OR_KEYWORDS),
                             "ReadonlyArray");
    EXPECT_FALSE(entry.GetHasAction());
    EXPECT_EQ(std::count_if(entries.cbegin(), entries.cend(),
                            [](const auto &candidate) { return candidate.GetInsertText() == "ReadonlyArray"; }),
              1U);

    initializer.DestroyContext(sourceCtx);
    initializer.DestroyContext(queryCtx);
}

// Inject a same-source indexed candidate to exercise the merge independently
// of which declarations the current stdlib cache exposes to the export index.
TEST_F(LSPCompletions2Tests, IndexedVisibleSystemExportFromSameSourceRemainsGlobal)
{
    std::vector<std::string> files = {"indexed_system_same_source_query.ets"};
    std::vector<std::string> texts = {R"(
let value: Readonly
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::InitSymbolReferenceIndex();
    ark::es2panda::lsp::Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    auto *publicCtx = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto systemSource = FindStdLibSourcePath(publicCtx);
    ASSERT_FALSE(systemSource.empty());

    const char *indexedSource = "export interface ReadonlyArray<T> {}";
    auto indexCtx = initializer.CreateContext("indexed_stdlib_source.ets", ES2PANDA_STATE_CHECKED, indexedSource);
    ASSERT_NE(indexCtx, nullptr);
    auto *indexPublicCtx = reinterpret_cast<ark::es2panda::public_lib::Context *>(indexCtx);
    ark::es2panda::SourceFile indexedFile(systemSource, indexedSource, false);
    indexPublicCtx->parserProgram->SetSource(indexedFile);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(indexCtx));
    auto indexedDefs = ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix("Readonly", filePaths[0]);
    ASSERT_TRUE(std::any_of(indexedDefs.cbegin(), indexedDefs.cend(), [&systemSource](const auto &definition) {
        return definition.symbolName == "ReadonlyArray" && definition.fileName == systemSource;
    }));

    const std::string marker = "Readonly";
    auto markerPos = texts[0].rfind(marker);
    ASSERT_NE(markerPos, std::string::npos);
    auto entries = lspApi->getCompletionsAtPosition(ctx, markerPos + marker.size()).GetEntries();

    auto entry = ExpectEntry(entries, "ReadonlyArray", CompletionEntryKind::INTERFACE, std::string(GLOBALS_OR_KEYWORDS),
                             "ReadonlyArray");
    EXPECT_FALSE(entry.GetHasAction());
    EXPECT_EQ(std::count_if(entries.cbegin(), entries.cend(),
                            [](const auto &candidate) { return candidate.GetInsertText() == "ReadonlyArray"; }),
              1U);

    initializer.DestroyContext(indexCtx);
    initializer.DestroyContext(ctx);
}

// An invisible declaration-module export must not suppress an auto-import with
// the same symbol name from a different project file.
TEST_F(LSPCompletions2Tests, InvisibleDeclarationExportDoesNotSuppressDifferentSourceAutoImport)
{
    std::vector<std::string> files = {"collision_api.d.ets", "collision_auto_import_source.ets", "collision_query.ets"};
    std::vector<std::string> texts = {R"(export const collisionAnchor: number = 0;
export interface ReviewCollisionSignal {}
)",
                                      R"(export interface ReviewCollisionSignal {}
)",
                                      R"(import { collisionAnchor } from './collision_api.d.ets'
let value: ReviewCollision
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::InitSymbolReferenceIndex();
    ark::es2panda::lsp::Initializer initializer;
    auto sourceCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto queryCtx = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(sourceCtx, nullptr);
    ASSERT_NE(queryCtx, nullptr);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(sourceCtx));

    const std::string marker = "ReviewCollision";
    auto markerPos = texts[2].rfind(marker);
    ASSERT_NE(markerPos, std::string::npos);
    auto entries = lspApi->getCompletionsAtPosition(queryCtx, markerPos + marker.size()).GetEntries();
    ExpectAutoImportEntry(entries, "ReviewCollisionSignal", CompletionEntryKind::INTERFACE, "ReviewCollisionSignal",
                          "collision_auto_import_source.ets");

    initializer.DestroyContext(sourceCtx);
    initializer.DestroyContext(queryCtx);
}

// A nested export is not visible as an unqualified global merely because its
// declaration module has been bound and loaded as an external program.
TEST_F(LSPCompletions2Tests, NestedDeclarationExportDoesNotSuppressDifferentSourceAutoImport)
{
    std::vector<std::string> files = {"nested_collision_api.d.ets", "nested_collision_source.ets",
                                      "nested_collision_query.ets"};
    std::vector<std::string> texts = {
        R"(export const nestedCollisionAnchor: number = 0;
export namespace CollisionApi {
    export interface NestedCollisionSignal {}
}
)",
        R"(export interface NestedCollisionSignal {}
)",
        R"(import { nestedCollisionAnchor, CollisionApi } from './nested_collision_api.d.ets'
let value: NestedCollision
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::InitSymbolReferenceIndex();
    ark::es2panda::lsp::Initializer initializer;
    auto sourceCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto queryCtx = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(sourceCtx, nullptr);
    ASSERT_NE(queryCtx, nullptr);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(sourceCtx));

    const std::string marker = "NestedCollision";
    auto markerPos = texts[2].rfind(marker);
    ASSERT_NE(markerPos, std::string::npos);
    auto entries = lspApi->getCompletionsAtPosition(queryCtx, markerPos + marker.size()).GetEntries();
    ExpectAutoImportEntry(entries, "NestedCollisionSignal", CompletionEntryKind::INTERFACE, "NestedCollisionSignal",
                          "nested_collision_source.ets");

    initializer.DestroyContext(sourceCtx);
    initializer.DestroyContext(queryCtx);
}

// Importing a namespace does not expose its members as unqualified globals.
TEST_F(LSPCompletions2Tests, ImportedNamespaceDoesNotExposeUnqualifiedMembers)
{
    std::vector<std::string> files = {"visible_namespace_api.d.ets", "visible_namespace_query.ets"};
    std::vector<std::string> texts = {R"(export namespace VisibleApi {
    export interface VisibleNestedSignal {}
}
)",
                                      R"(import { VisibleApi } from './visible_namespace_api.d.ets'
let value: VisibleNested
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    const std::string marker = "VisibleNested";
    auto markerPos = texts[1].rfind(marker);
    ASSERT_NE(markerPos, std::string::npos);
    auto entries = lspApi->getCompletionsAtPosition(ctx, markerPos + marker.size()).GetEntries();
    ExpectNoActionlessBareEntry(entries, "VisibleNestedSignal");

    initializer.DestroyContext(ctx);
}

// A qualified completion selects the namespace explicitly imported by the user.
TEST_F(LSPCompletions2Tests, NestedExportUsesItsActualNamespaceOwner)
{
    std::vector<std::string> files = {"multiple_namespace_owners.d.ets", "multiple_namespace_owners_query.ets"};
    std::vector<std::string> texts = {R"(export namespace FirstApi {
    export class SharedNestedSignal {}
}
export namespace SecondApi {
    export interface SharedNestedSignal {}
}
)",
                                      R"(import { SecondApi } from './multiple_namespace_owners.d.ets'
let value: SecondApi.SharedNested
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    const std::string marker = "SharedNested";
    auto markerPos = texts[1].rfind(marker);
    ASSERT_NE(markerPos, std::string::npos);
    auto entries = lspApi->getCompletionsAtPosition(ctx, markerPos + marker.size()).GetEntries();
    ASSERT_TRUE(HasEntryWithName(entries, "SharedNestedSignal"));
    auto entry = *std::find_if(entries.begin(), entries.end(),
                               [](const auto &candidate) { return candidate.GetName() == "SharedNestedSignal"; });
    EXPECT_FALSE(entry.GetHasAction());
    EXPECT_EQ(entry.GetCompletionKind(), CompletionEntryKind::INTERFACE);
    auto insertText = entry.GetInsertText().empty() ? entry.GetName() : entry.GetInsertText();
    EXPECT_EQ(insertText, "SharedNestedSignal");

    initializer.DestroyContext(ctx);
    auto completed = texts[1];
    completed.replace(markerPos, marker.size(), insertText);
    ExpectCompletedSourceResolves(filePaths[1], completed, "SharedNestedSignal", filePaths[0],
                                  texts[0].rfind("SharedNestedSignal"));
}

// Importing a namespace under a local alias must preserve visibility of its
// nested declaration completions.
TEST_F(LSPCompletions2Tests, AliasedNamespaceImportKeepsNestedSystemCompletion)
{
    std::vector<std::string> files = {"aliased_namespace_api.d.ets", "aliased_namespace_query.ets"};
    std::vector<std::string> texts = {R"(export namespace OriginalApi {
    export interface AliasedNestedSignal {}
}
)",
                                      R"(import { OriginalApi as ApiAlias } from './aliased_namespace_api.d.ets'
let value: ApiAlias.AliasedNested
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    const std::string marker = "AliasedNested";
    auto markerPos = texts[1].rfind(marker);
    ASSERT_NE(markerPos, std::string::npos);
    auto entries = lspApi->getCompletionsAtPosition(ctx, markerPos + marker.size()).GetEntries();
    ASSERT_TRUE(HasEntryWithName(entries, "AliasedNestedSignal"));
    auto entry = *std::find_if(entries.begin(), entries.end(),
                               [](const auto &candidate) { return candidate.GetName() == "AliasedNestedSignal"; });
    EXPECT_FALSE(entry.GetHasAction());
    auto insertText = entry.GetInsertText().empty() ? entry.GetName() : entry.GetInsertText();
    EXPECT_EQ(insertText, "AliasedNestedSignal");

    initializer.DestroyContext(ctx);
    auto completed = texts[1];
    completed.replace(markerPos, marker.size(), insertText);
    ExpectCompletedSourceResolves(filePaths[1], completed, "AliasedNestedSignal", filePaths[0],
                                  texts[0].find("AliasedNestedSignal"));
}

// ConcurrentHashMap belongs to the real stdlib containers namespace. It must
// not replace an unrelated project export with an actionless bare-name entry.
TEST_F(LSPCompletions2Tests, StdLibNamespaceMemberDoesNotSuppressProjectAutoImport)
{
    std::vector<std::string> files = {"stdlib_collision_source.ets", "stdlib_collision_query.ets"};
    std::vector<std::string> texts = {R"(export class ConcurrentHashMap {}
)",
                                      R"(let value: ConcurrentHash
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::InitSymbolReferenceIndex();
    ark::es2panda::lsp::Initializer initializer;
    auto sourceCtx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto queryCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(sourceCtx, nullptr);
    ASSERT_NE(queryCtx, nullptr);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(sourceCtx));

    const std::string marker = "ConcurrentHash";
    auto markerPos = texts[1].rfind(marker);
    ASSERT_NE(markerPos, std::string::npos);
    auto entries = lspApi->getCompletionsAtPosition(queryCtx, markerPos + marker.size()).GetEntries();
    ExpectAutoImportEntry(entries, "ConcurrentHashMap", CompletionEntryKind::CLASS, "ConcurrentHashMap",
                          "stdlib_collision_source.ets");

    initializer.DestroyContext(sourceCtx);
    initializer.DestroyContext(queryCtx);
}

// Simulate a client consuming completion data. The native completion API exposes
// an export name and source, not import TextChanges; the import below is a client
// simulation, while syntax, semantics and the resulting definition are checked by LSP.
TEST_F(LSPCompletions2Tests, AutoImportDataProducesResolvableSourceWithoutDamagingExistingImport)
{
    constexpr size_t sourceFileIndex = 0;
    constexpr size_t queryFileIndex = 2;
    std::vector<std::string> files = {"accept_signal.ets", "accept_sdk.d.ets", "accept_query.ets"};
    std::vector<std::string> texts = {
        "export interface AcceptedSignal { value: number; }",
        "export const anchor: number = 0;\nexport interface AcceptedSignal { wrong: string; }",
        "import { anchor } from './accept_sdk.d.ets'\nlet signal: AcceptedSig = { value: 1 };\n"};
    auto paths = CreateTempFile(files, texts);
    ASSERT_EQ(paths.size(), files.size());
    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::InitSymbolReferenceIndex();
    ark::es2panda::lsp::Initializer initializer;
    auto sourceCtx = initializer.CreateContext(paths[sourceFileIndex].c_str(), ES2PANDA_STATE_CHECKED);
    auto queryCtx = initializer.CreateContext(paths[queryFileIndex].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(sourceCtx, nullptr);
    ASSERT_NE(queryCtx, nullptr);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(sourceCtx));
    const std::string marker = "AcceptedSig";
    auto position = texts[queryFileIndex].rfind(marker);
    ASSERT_NE(position, std::string::npos);
    auto entries = lspApi->getCompletionsAtPosition(queryCtx, position + marker.size()).GetEntries();
    auto entry = ExpectEntry(entries, "AcceptedSignal", CompletionEntryKind::INTERFACE,
                             std::string(AUTO_IMPORT_SUGGESTIONS), "AcceptedSignal");
    ASSERT_TRUE(entry.GetHasAction());
    auto data = entry.GetCompletionEntryData();
    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(data->GetImportDeclaration(), paths[sourceFileIndex]);
    EXPECT_EQ(data->GetNamedExport(), "AcceptedSignal");
    auto completed = texts[queryFileIndex];
    completed.replace(position, marker.size(), entry.GetInsertText());
    auto modulePath =
        ark::es2panda::lsp::ComputeRelativeImportPath(paths[queryFileIndex], data->GetImportDeclaration());
    completed.insert(0, "import { " + data->GetNamedExport() + " } from '" + modulePath + "'\n");
    EXPECT_EQ(completed,
              "import { AcceptedSignal } from './accept_signal'\n"
              "import { anchor } from './accept_sdk.d.ets'\n"
              "let signal: AcceptedSignal = { value: 1 };\n");
    initializer.DestroyContext(queryCtx);
    ExpectCompletedSourceResolves(paths[queryFileIndex], completed, entry.GetInsertText(), paths[sourceFileIndex],
                                  texts[sourceFileIndex].find("AcceptedSignal"));
    initializer.DestroyContext(sourceCtx);
}

// Different modules exporting the same unavailable name remain distinct
// auto-import choices, with each candidate retaining its own source module.
TEST_F(LSPCompletions2Tests, SameNamedProjectExportsKeepBothAutoImportSources)
{
    std::vector<std::string> files = {"signal_first.ets", "signal_second.ets", "signal_choice_query.ets",
                                      "signal_api.d.ets"};
    std::vector<std::string> texts = {"export interface ChoiceSignal {}", "export interface ChoiceSignal {}",
                                      "import { signalAnchor } from './signal_api.d.ets'\nlet value: ChoiceSig",
                                      "export const signalAnchor: number = 0;\nexport interface ChoiceSignal {}"};
    auto paths = CreateTempFile(files, texts);
    ASSERT_EQ(paths.size(), files.size());
    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::InitSymbolReferenceIndex();
    ark::es2panda::lsp::Initializer initializer;
    auto firstCtx = initializer.CreateContext(paths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto secondCtx = initializer.CreateContext(paths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto queryCtx = initializer.CreateContext(paths[2].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(firstCtx, nullptr);
    ASSERT_NE(secondCtx, nullptr);
    ASSERT_NE(queryCtx, nullptr);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(firstCtx));
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(secondCtx));

    auto entries = lspApi->getCompletionsAtPosition(queryCtx, texts[2].size()).GetEntries();
    std::unordered_set<std::string> sources;
    for (auto &entry : entries) {
        if (entry.GetName() != "ChoiceSignal") {
            continue;
        }
        EXPECT_TRUE(entry.GetHasAction());
        ASSERT_TRUE(entry.GetCompletionEntryData().has_value());
        sources.insert(entry.GetCompletionEntryData()->GetImportDeclaration());
    }
    EXPECT_EQ(sources, (std::unordered_set<std::string> {paths[0], paths[1]}));

    initializer.DestroyContext(firstCtx);
    initializer.DestroyContext(secondCtx);
    initializer.DestroyContext(queryCtx);
}

// The external-program tree and the symbol index may refer to the same file
// through different path spellings. That must still be treated as one source,
// so the indexed export keeps its auto-import action.
TEST_F(LSPCompletions2Tests, SameSourceViaHardLinkKeepsAutoImportCompletion)
{
    std::vector<std::string> files = {"same_source_api.d.ets", "same_source_query.ets"};
    std::vector<std::string> texts = {R"(export interface SameSourceSignal {}
export const sameSourceAnchor: number = 0;
)",
                                      R"(import { sameSourceAnchor } from './same_source_api.d.ets'
let value: SameSource
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    std::filesystem::path const sourcePath(filePaths[0]);
    auto aliasPath = sourcePath.parent_path() / "same_source_alias.d.ets";
    std::error_code error;
    std::filesystem::create_hard_link(sourcePath, aliasPath, error);
    ASSERT_FALSE(error) << "failed to create hard-link source alias: " << error.message();
    auto aliasPathString = aliasPath.string();

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::Initializer initializer;
    auto sourceCtx = initializer.CreateContext(aliasPathString.c_str(), ES2PANDA_STATE_CHECKED);
    auto queryCtx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(sourceCtx, nullptr);
    ASSERT_NE(queryCtx, nullptr);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContextWithExternal(sourceCtx));

    const std::string marker = "SameSource";
    auto markerPos = texts[1].rfind(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();
    auto entries = lspApi->getCompletionsAtPosition(queryCtx, offset).GetEntries();

    auto entry = ExpectEntry(entries, "SameSourceSignal", CompletionEntryKind::INTERFACE,
                             std::string(AUTO_IMPORT_SUGGESTIONS), "SameSourceSignal");
    EXPECT_TRUE(entry.GetHasAction());
    ASSERT_TRUE(entry.GetCompletionEntryData().has_value());
    EXPECT_EQ(entry.GetCompletionEntryData()->GetNamedExport(), "SameSourceSignal");
    EXPECT_NE(entry.GetCompletionEntryData()->GetImportDeclaration().find("same_source_alias.d.ets"),
              std::string::npos);

    initializer.DestroyContext(sourceCtx);
    initializer.DestroyContext(queryCtx);
}

// collectApiInfo collects annotations, type aliases, functions, variables and export
// specifiers from a '@'-prefixed source declaration module, so that a later completion
// request can surface them as auto-import suggestions.
TEST_F(LSPCompletions2Tests, ApiInfoCollectsDiverseExportKinds)
{
    std::vector<std::string> files = {"@cov_api.d.ets", "cov_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export @interface CovAnn {
    covField: number;
}
export type CovAlias = number;
export function covFunc(a: number): number {
    return a;
}
export let covVar: number = 1;
export const covConst: number = 2;
)",
                                      R"(
import { covVar } from './@cov_api';
cov
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    // PARSED keeps top-level function/variable declarations unlowered, so
    // collectApiInfo can resolve every export kind of the '@'-prefixed module.
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_TRUE(lspApi->collectApiInfo(ctx));
    initializer.DestroyContext(ctx);

    // All exported symbols from the '@'-prefixed module must be collected.
    auto annInfos = ark::es2panda::lsp::GetExternalApiCollectInfos("CovAnn");
    auto aliasInfos = ark::es2panda::lsp::GetExternalApiCollectInfos("CovAlias");
    auto funcInfos = ark::es2panda::lsp::GetExternalApiCollectInfos("covFunc");
    auto varInfos = ark::es2panda::lsp::GetExternalApiCollectInfos("covVar");
    auto constInfos = ark::es2panda::lsp::GetExternalApiCollectInfos("covConst");
    ASSERT_FALSE(annInfos.empty()) << "annotation export not collected";
    ASSERT_FALSE(aliasInfos.empty()) << "type alias export not collected";
    ASSERT_FALSE(funcInfos.empty()) << "function export not collected";
    ASSERT_FALSE(varInfos.empty()) << "variable export not collected";
    ASSERT_FALSE(constInfos.empty()) << "const export not collected";
    EXPECT_EQ(annInfos.front().kind, ark::es2panda::lsp::CompletionEntryKind::ANNOTATION);
    EXPECT_EQ(aliasInfos.front().kind, ark::es2panda::lsp::CompletionEntryKind::ALIAS_TYPE);
    EXPECT_EQ(funcInfos.front().kind, ark::es2panda::lsp::CompletionEntryKind::FUNCTION);
}

// A re-export statement 'export { a, b };' in an '@'-prefixed module collects
// the exported names through CollectExportFromSpecifier.
// NOTE: module file names must be unique across tests in this binary. Imported
// programs are cached per absolute source path, so reusing a file name from
// another test would serve the previously parsed AST and hide the specifier
// statement from CollectExportsFromProgram.
TEST_F(LSPCompletions2Tests, ReexportSpecifierCollectsExports)
{
    std::vector<std::string> files = {"@reexport_api.d.ets", "reexport_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export class CovCls {}
export let covVar: number = 1;
export const covConst: number = 2;
export { covVar, covConst };
)",
                                      R"(
import { covVar } from './@reexport_api';
cov
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_TRUE(lspApi->collectApiInfo(ctx));
    initializer.DestroyContext(ctx);

    auto varInfos = ark::es2panda::lsp::GetExternalApiCollectInfos("covVar");
    auto constInfos = ark::es2panda::lsp::GetExternalApiCollectInfos("covConst");
    ASSERT_FALSE(varInfos.empty()) << "specifier export (let) not collected";
    ASSERT_FALSE(constInfos.empty()) << "specifier export (const) not collected";
    for (const auto &info : varInfos) {
        EXPECT_FALSE(info.importDeclaration.empty());
    }
}

// Namespace member completions after 'NsM.' offer the exported members.
TEST_F(LSPCompletions2Tests, NamespaceModuleDeclarationExportedMembers)
{
    std::vector<std::string> files = {"ns_module_bound.ets"};
    const std::string text = R"delimiter(
namespace NsM {
    export class NsClass {}
    export interface NsIface {}
    export const NsConst: number = 1;
    export let NsLet: number = 2;
    export function NsFunc(): void {}
    export namespace NsInner {}
}
NsM.
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto markerPos = text.find("NsM.");
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + std::string("NsM.").size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();
    initializer.DestroyContext(ctx);

    ASSERT_TRUE(HasEntryWithName(entries, "NsClass")) << "namespace exported class not suggested";
    ASSERT_TRUE(HasEntryWithName(entries, "NsIface")) << "namespace exported interface not suggested";
    ASSERT_TRUE(HasEntryWithName(entries, "NsConst: number")) << "namespace exported const not suggested";
    ASSERT_TRUE(HasEntryWithName(entries, "NsLet: number")) << "namespace exported let not suggested";
    ASSERT_TRUE(HasEntryWithName(entries, "NsFunc(): void")) << "namespace exported function not suggested";
    ASSERT_TRUE(HasEntryWithName(entries, "NsInner")) << "namespace exported nested namespace not suggested";
}

// Global completion on a source with top-level 'let'/'const' assignments: the
// globals are hoisted into ETSGLOBAL class properties, and InitEntry's IsNamespaceVar
// arm derives VARIABLE/CONSTANT from each property's modifier (no '$init$' lookup is
// involved; that scan was unreachable dead code and has been removed).
TEST_F(LSPCompletions2Tests, GlobalVariableInitEntryCompletion)
{
    const std::string text = R"delimiter(
let globalInitVar: number = 1;
const globalInitConst: number = 2;
function globalInitFunc(): void {}
g
)delimiter";
    auto entries = GetCompletionsAfterMarker("global_init_entry.ets", text, "\ng");
    ExpectEntry(entries, "globalInitVar: number", CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                "globalInitVar");
    ExpectEntry(entries, "globalInitConst: number", CompletionEntryKind::CONSTANT, std::string(GLOBALS_OR_KEYWORDS),
                "globalInitConst");
    ExpectEntry(entries, "globalInitFunc(): void", CompletionEntryKind::FUNCTION, std::string(GLOBALS_OR_KEYWORDS),
                "globalInitFunc()");
}

TEST_F(LSPCompletions2Tests, AutoImportOverridesSystemCompletionForDeclarationExports)
{
    std::vector<std::string> files = {"@sys_iface_api.d.ets", "sys_helper_module.ets", "sys_iface_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export class SysIfaceClass {
    sysField: number = 0;
}
export const sysIfaceAnchor: number = 0;
export const sysIfaceConst: number = 1;
)",
                                      R"(export function sysHelperFunc(): void {}
export let sysHelperValue: number = 2;
)",
                                      R"(
import { sysIfaceAnchor } from './@sys_iface_api';
import { sysHelperValue } from './sys_helper_module';
import { sysPkgValue } from './sys_pkg';
sys
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    // A directory import resolves as a package: create 'sys_pkg' with one fraction file.
    std::filesystem::path queryPath = std::filesystem::path(filePaths[2]);
    std::filesystem::path pkgDir = queryPath.parent_path() / "sys_pkg";
    ASSERT_TRUE(std::filesystem::create_directory(pkgDir));
    {
        std::ofstream fraction(pkgDir / "sys_fraction.ets");
        fraction << "export const sysPkgValue: number = 3;\n";
        ASSERT_FALSE(fraction.fail());
    }

    LSPAPI const *lspApi = GetImpl();
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto ctx = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    ASSERT_TRUE(lspApi->collectApiInfo(ctx));
    const std::string &queryText = texts[2];
    const std::string marker = "\nsys";
    auto markerPos = queryText.rfind(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);

    auto classEntry = ExpectEntry(res.GetEntries(), "SysIfaceClass", CompletionEntryKind::CLASS,
                                  std::string(AUTO_IMPORT_SUGGESTIONS), "SysIfaceClass");
    EXPECT_TRUE(classEntry.GetHasAction());
    auto constEntry = ExpectEntry(res.GetEntries(), "sysIfaceConst", CompletionEntryKind::CONSTANT,
                                  std::string(AUTO_IMPORT_SUGGESTIONS), "sysIfaceConst");
    EXPECT_TRUE(constEntry.GetHasAction());
    ExpectNoEntryWithName(res.GetEntries(), "sysIfaceConst: number");
    ExpectNoEntryWithName(res.GetEntries(), "sysHelperFunc(): void");
}

}  // namespace
