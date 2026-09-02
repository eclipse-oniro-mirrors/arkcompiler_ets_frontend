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

#include <filesystem>
#include <fstream>

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

TEST_F(LSPCompletions2Tests, SystemInterfaceCompletionVisitsExternalDecls)
{
    std::vector<std::string> files = {"@sys_iface_api.d.ets", "sys_helper_module.ets", "sys_iface_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export class SysIfaceClass {
    sysField: number = 0;
}
export const sysIfaceConst: number = 1;
)",
                                      R"(export function sysHelperFunc(): void {}
export let sysHelperValue: number = 2;
)",
                                      R"(
import { sysIfaceConst } from './@sys_iface_api';
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
    auto ctx = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    const std::string &queryText = texts[2];
    const std::string marker = "\nsys";
    auto markerPos = queryText.rfind(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    initializer.DestroyContext(ctx);

    ASSERT_TRUE(HasEntryWithName(res.GetEntries(), "SysIfaceClass")) << "'@'-prefixed module class not suggested";
    ASSERT_TRUE(HasEntryWithName(res.GetEntries(), "sysIfaceConst: number"))
        << "'@'-prefixed module const not suggested";
    ASSERT_TRUE(HasEntryWithName(res.GetEntries(), "sysHelperFunc(): void"))
        << "plain imported module function not suggested";
}

}  // namespace
