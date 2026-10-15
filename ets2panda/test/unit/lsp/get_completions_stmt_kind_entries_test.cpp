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

using ark::es2panda::lsp::CompletionEntry;
using ark::es2panda::lsp::CompletionEntryKind;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::sort_text::CLASS_MEMBER_SNIPPETS;
using ark::es2panda::lsp::sort_text::GLOBALS_OR_KEYWORDS;
using ark::es2panda::lsp::sort_text::MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT;
using ark::es2panda::lsp::sort_text::SUGGESTED_CLASS_MEMBERS;

namespace {

static bool IsMatchedStmtKindEntry(const CompletionEntry &actual, const CompletionEntry &expected)
{
    if (actual.GetCompletionKind() != expected.GetCompletionKind()) {
        return false;
    }
    return actual.GetName() == expected.GetName() && actual.GetInsertText() == expected.GetInsertText();
}

static void AssertStmtKindEntries(const std::vector<CompletionEntry> &entries,
                                  const std::vector<CompletionEntry> &expectedEntries,
                                  const std::vector<CompletionEntry> &unexpectedEntries)
{
    for (const auto &expectedEntry : expectedEntries) {
        bool found = false;
        for (const auto &entry : entries) {
            if (IsMatchedStmtKindEntry(entry, expectedEntry)) {
                found = true;
                break;
            }
        }
        ASSERT_TRUE(found) << "Expected completion '" << expectedEntry.GetName() << "' not found";
    }
    for (const auto &unexpectedEntry : unexpectedEntries) {
        bool found = false;
        for (const auto &entry : entries) {
            if (IsMatchedStmtKindEntry(entry, unexpectedEntry)) {
                found = true;
                break;
            }
        }
        ASSERT_FALSE(found) << "Unexpected completion '" << unexpectedEntry.GetName() << "' found";
    }
}

static const CompletionEntry *FindStmtKindEntry(const std::vector<CompletionEntry> &entries, const std::string &name)
{
    for (const auto &entry : entries) {
        if (entry.GetName() == name) {
            return &entry;
        }
    }
    return nullptr;
}

class LSPCompletionsStmtKindEntriesTests : public LSPAPITests {
public:
    static std::vector<CompletionEntry> BuildExpectedDerivedThisEntries()
    {
        return std::vector<CompletionEntry> {
            CompletionEntry("derivedFlag: boolean", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS),
                            "derivedFlag"),
            CompletionEntry("test(): void", CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS), "test()"),
            CompletionEntry("pubProp: number", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS),
                            "pubProp"),
            CompletionEntry("protProp: number", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS),
                            "protProp"),
            CompletionEntry("pubMethod(): void", CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS),
                            "pubMethod()"),
            CompletionEntry("protMethod(): void", CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS),
                            "protMethod()")};
    }

    void ExpectOutsideClassExcludesPrivates()
    {
        std::vector<std::string> files = {"stmt_kind_class_flags.ets"};
        std::vector<std::string> texts = {R"(
class StmtKindBase {
    public pubProp: number = 1;
    private privProp: number = 2;
    protected protProp: number = 3;
    public pubMethod(): void {
    }
    private privMethod(): void {
    }
    protected protMethod(): void {
    }
}
let baseObj = new StmtKindBase();
let baseUse = baseObj.
let a = 1;
)"};
        auto filePaths = CreateTempFile(files, texts);
        ASSERT_EQ(filePaths.size(), 1U);
        Initializer initializer = Initializer();
        auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
        LSPAPI const *lspApi = GetImpl();

        const std::string marker = "baseObj.";
        const size_t offsetOutside = texts[0].find(marker) + marker.size();
        auto resOutside = lspApi->getCompletionsAtPosition(ctx, offsetOutside);
        auto entriesOutside = resOutside.GetEntries();

        auto expectedOutside =
            std::vector<CompletionEntry> {CompletionEntry("pubProp: number", CompletionEntryKind::PROPERTY,
                                                          std::string(SUGGESTED_CLASS_MEMBERS), "pubProp"),
                                          CompletionEntry("pubMethod(): void", CompletionEntryKind::METHOD,
                                                          std::string(CLASS_MEMBER_SNIPPETS), "pubMethod()")};
        ASSERT_EQ(entriesOutside.size(), 2U);
        AssertStmtKindEntries(entriesOutside, expectedOutside, {});
        AssertStmtKindEntries(entriesOutside, {},
                              {CompletionEntry("privProp: number", CompletionEntryKind::PROPERTY,
                                               std::string(SUGGESTED_CLASS_MEMBERS), "privProp"),
                               CompletionEntry("protProp: number", CompletionEntryKind::PROPERTY,
                                               std::string(SUGGESTED_CLASS_MEMBERS), "protProp"),
                               CompletionEntry("privMethod(): void", CompletionEntryKind::METHOD,
                                               std::string(CLASS_MEMBER_SNIPPETS), "privMethod()"),
                               CompletionEntry("protMethod(): void", CompletionEntryKind::METHOD,
                                               std::string(CLASS_MEMBER_SNIPPETS), "protMethod()")});
        initializer.DestroyContext(ctx);
    }

    void ExpectDerivedClassCompletions()
    {
        std::vector<std::string> derivedFiles = {"stmt_kind_class_flags_derived.ets"};
        std::vector<std::string> derivedTexts = {R"(
class StmtKindBase {
    public pubProp: number = 1;
    private privProp: number = 2;
    protected protProp: number = 3;
    public pubMethod(): void {
    }
    private privMethod(): void {
    }
    protected protMethod(): void {
    }
}
class StmtKindDerived extends StmtKindBase {
    derivedFlag: boolean = false;
    test(): void {
        this.
        this.derivedFlag = true;
    }
}
let a = 1;
)"};
        auto derivedPaths = CreateTempFile(derivedFiles, derivedTexts);
        ASSERT_EQ(derivedPaths.size(), 1U);
        Initializer derivedInitializer = Initializer();
        auto derivedCtx = derivedInitializer.CreateContext(derivedPaths[0].c_str(), ES2PANDA_STATE_CHECKED);
        LSPAPI const *derivedLspApi = GetImpl();

        const std::string thisMarker = "this.";
        const size_t offsetThis = derivedTexts[0].find(thisMarker) + thisMarker.size();
        auto resThis = derivedLspApi->getCompletionsAtPosition(derivedCtx, offsetThis);
        auto entriesThis = resThis.GetEntries();
        auto expectedThis = BuildExpectedDerivedThisEntries();
        ASSERT_EQ(entriesThis.size(), 6U);
        AssertStmtKindEntries(entriesThis, expectedThis, {});
        // The base private members stay excluded even in the derived class.
        AssertStmtKindEntries(entriesThis, {},
                              {CompletionEntry("privProp: number", CompletionEntryKind::PROPERTY,
                                               std::string(SUGGESTED_CLASS_MEMBERS), "privProp"),
                               CompletionEntry("privMethod(): void", CompletionEntryKind::METHOD,
                                               std::string(CLASS_MEMBER_SNIPPETS), "privMethod()")});
        derivedInitializer.DestroyContext(derivedCtx);
    }

    void ExpectExportedStructInterfaceFnEntries(LSPAPI const *lspApi, es2panda_Context *ctx,
                                                const std::vector<std::string> &texts)
    {
        const std::string marker = "import { Pa16";
        const size_t offset = texts[1].find(marker) + marker.size();
        auto entries = lspApi->getCompletionsAtPosition(ctx, offset).GetEntries();

        const auto *structEntry = FindStmtKindEntry(entries, "Pa16Struct");
        ASSERT_NE(structEntry, nullptr) << "exported struct entry missing";
        EXPECT_EQ(structEntry->GetCompletionKind(), CompletionEntryKind::STRUCT);
        EXPECT_EQ(structEntry->GetInsertText(), "Pa16Struct");
        const auto *ifaceEntry = FindStmtKindEntry(entries, "Pa16Iface");
        ASSERT_NE(ifaceEntry, nullptr) << "exported interface entry missing";
        EXPECT_EQ(ifaceEntry->GetCompletionKind(), CompletionEntryKind::INTERFACE);
        EXPECT_EQ(ifaceEntry->GetInsertText(), "Pa16Iface");

        // The specifier filter compares the typed fragment case-sensitively, so the
        // lower-case function name needs its own lower-case fragment.
        const std::string fnMarker = "import { pa16";
        const size_t fnOffset = texts[1].find(fnMarker) + fnMarker.size();
        auto fnEntries = lspApi->getCompletionsAtPosition(ctx, fnOffset).GetEntries();
        const auto *fnEntry = FindStmtKindEntry(fnEntries, "pa16Fn(a: number): number");
        ASSERT_NE(fnEntry, nullptr) << "exported function entry missing";
        EXPECT_EQ(fnEntry->GetCompletionKind(), CompletionEntryKind::METHOD);
        EXPECT_EQ(fnEntry->GetInsertText(), "pa16Fn");
    }

    void ExpectAnonDefaultExportNoEmptyEntries(Initializer &initializer, LSPAPI const *lspApi)
    {
        // An anonymous default-exported function has no name to offer: GetDeclarationEntry
        // drops it and no empty-named entry may leak into the suggestions.
        std::vector<std::string> anonFiles = {"@pa16_anon.d.ets", "pa16_anon_query.ets"};
        std::vector<std::string> anonTexts = {R"('use static'
export default function(): void {}
)",
                                              R"(
import { Pa16Anon } from './@pa16_anon'
export const pa16AnonUsed: number = 1;
)"};
        auto anonPaths = CreateTempFile(anonFiles, anonTexts);
        ASSERT_EQ(anonPaths.size(), anonFiles.size());
        for (auto state : {ES2PANDA_STATE_PARSED, ES2PANDA_STATE_CHECKED}) {
            auto anonCtx = initializer.CreateContext(anonPaths[1].c_str(), state);
            const std::string anonMarker = "import { Pa16Anon";
            const size_t anonOffset = anonTexts[1].find(anonMarker) + anonMarker.size();
            auto anonEntries = lspApi->getCompletionsAtPosition(anonCtx, anonOffset).GetEntries();
            for (const auto &entry : anonEntries) {
                EXPECT_FALSE(entry.GetName().empty()) << "empty-named entry leaked from anonymous default export";
            }
            initializer.DestroyContext(anonCtx);
        }
    }

    void ExpectIndexedApiEntryShapes(const std::vector<CompletionEntry> &entries)
    {
        const std::string autoSort = std::string(ark::es2panda::lsp::sort_text::AUTO_IMPORT_SUGGESTIONS);
        auto expectIndexed = [&entries, &autoSort](const std::string &name, CompletionEntryKind kind,
                                                   const std::string &insertText) {
            const auto *entry = FindStmtKindEntry(entries, name);
            ASSERT_NE(entry, nullptr) << "indexed suggestion missing: " << name;
            EXPECT_EQ(entry->GetSortText(), autoSort) << "indexed sort text mismatch: " << name;
            EXPECT_EQ(entry->GetCompletionKind(), kind) << "indexed kind mismatch: " << name;
            EXPECT_EQ(entry->GetInsertText(), insertText) << "indexed insert text mismatch: " << name;
        };
        expectIndexed("PaIdxAlias", CompletionEntryKind::ALIAS_TYPE, "PaIdxAlias");
        expectIndexed("PaIdxGear", CompletionEntryKind::CLASS, "PaIdxGear");
        expectIndexed("PaIdxStyle", CompletionEntryKind::INTERFACE, "PaIdxStyle");
        expectIndexed("paIdxRun(): number", CompletionEntryKind::METHOD, "paIdxRun()");
        // CURRENT BEHAVIOR: at BOUND state the identifier's declaration type resolves to
        // a class-like declaration for enum/namespace/variable exports, so those indexed
        // suggestions report CLASS instead of ENUM/MODULE/VARIABLE.
        expectIndexed("PaIdxColor", CompletionEntryKind::CLASS, "PaIdxColor");
        expectIndexed("PaIdxSpace", CompletionEntryKind::CLASS, "PaIdxSpace");
        expectIndexed("paIdxVar", CompletionEntryKind::CLASS, "paIdxVar");

        // The local class already supplies a "PaIdxMachine" scope completion, so the
        // indexed auto-import suggestion for the same symbol name is filtered out.
        const auto *localMachine = FindStmtKindEntry(entries, "PaIdxMachine");
        ASSERT_NE(localMachine, nullptr) << "local class scope entry missing";
        EXPECT_NE(localMachine->GetSortText(), autoSort) << "indexed suggestion must be skipped for an existing name";
    }

    void ExpectTypePositionInterfaceEntries(Initializer &initializer, LSPAPI const *lspApi, es2panda_Context *ctx,
                                            const std::vector<std::string> &paths,
                                            const std::vector<std::string> &texts)
    {
        const std::string typeMarker = "target: Pa24Sh";
        const size_t typeOffset = texts[0].find(typeMarker) + typeMarker.size();
        auto typeEntries = lspApi->getCompletionsAtPosition(ctx, typeOffset).GetEntries();
        bool sawScopeSuggestion = false;
        bool sawSuffixSuggestion = false;
        for (const auto &entry : typeEntries) {
            if (entry.GetName() != "Pa24ShapeInterface") {
                continue;
            }
            if (entry.GetSortText() == std::string(SUGGESTED_CLASS_MEMBERS)) {
                sawSuffixSuggestion = true;
            }
            if (entry.GetSortText() == std::string(GLOBALS_OR_KEYWORDS)) {
                sawScopeSuggestion = true;
            }
        }
        // The scope-decl path already offers the interface in a type context, so the
        // type-context suffix rule finds the name among the existing completions and
        // suppresses its own duplicate entry (CURRENT BEHAVIOR).
        EXPECT_TRUE(sawScopeSuggestion) << "in-type-context interface scope entry missing";
        EXPECT_FALSE(sawSuffixSuggestion) << "duplicate suffix-rule entry must be suppressed";

        const std::string valueMarker = "let pa24Val = Pa24Sh";
        const size_t valueOffset = texts[0].find(valueMarker) + valueMarker.size();
        auto valueCtx = initializer.CreateContext(paths[0].c_str(), ES2PANDA_STATE_CHECKED);
        auto valueEntries = lspApi->getCompletionsAtPosition(valueCtx, valueOffset).GetEntries();
        EXPECT_EQ(FindStmtKindEntry(valueEntries, "Pa24ShapeInterface"), nullptr)
            << "interface declaration must be skipped in a value position";
        initializer.DestroyContext(valueCtx);
    }

    void ExpectCommentPositionEmpty(Initializer &initializer, LSPAPI const *lspApi)
    {
        // A completion position inside a leading comment has no preceding token at all,
        // so global completion returns nothing.
        std::vector<std::string> headFiles = {"pa24_head.ets"};
        std::vector<std::string> headTexts = {R"(// pa24 lead comment
function pa24Solo(): void {
}
)"};
        auto headPaths = CreateTempFile(headFiles, headTexts);
        ASSERT_EQ(headPaths.size(), 1U);
        auto headCtx = initializer.CreateContext(headPaths[0].c_str(), ES2PANDA_STATE_CHECKED);
        const std::string headMarker = "// pa24";
        const size_t headOffset = headTexts[0].find(headMarker) + headMarker.size();
        EXPECT_TRUE(lspApi->getCompletionsAtPosition(headCtx, headOffset).GetEntries().empty());
        initializer.DestroyContext(headCtx);
    }
};

// Test: completion positions on real source resolve receivers and identifiers across
// dots, underscores and separator characters.
TEST_F(LSPCompletionsStmtKindEntriesTests, GetPrecedingTokenPositionMatrix)
{
    const std::string source = R"(
class TokenMatrix {
    cd: number = 1;
}
let ab = new TokenMatrix();
let a_bc = ab.cd;
)";
    Initializer initializer;
    auto *ctx = initializer.CreateContext("completion_token_positions.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    const size_t afterDot = source.find("ab.cd") + std::string("ab.").size();
    auto memberEntries = lspApi->getCompletionsAtPosition(ctx, afterDot).GetEntries();
    const auto *cdEntry = FindStmtKindEntry(memberEntries, "cd: number");
    ASSERT_NE(cdEntry, nullptr) << "dot-separated receiver completion missing";
    EXPECT_EQ(cdEntry->GetCompletionKind(), CompletionEntryKind::PROPERTY);

    const size_t underscoreEnd = source.find("a_bc") + 4;
    auto identifierEntries = lspApi->getCompletionsAtPosition(ctx, underscoreEnd).GetEntries();
    ASSERT_NE(FindStmtKindEntry(identifierEntries, "a_bc: Double"), nullptr)
        << "underscore identifier completion missing";

    const size_t newlinePos = source.find(";\nlet a_bc") + 1;
    auto separatorEntries = lspApi->getCompletionsAtPosition(ctx, newlinePos).GetEntries();
    ASSERT_NE(FindStmtKindEntry(separatorEntries, "ab: TokenMatrix"), nullptr)
        << "separator-adjacent global completion missing";

    ASSERT_TRUE(lspApi->getCompletionsAtPosition(ctx, 1).GetEntries().empty());
    initializer.DestroyContext(ctx);

    const std::string separatorOnly = "//.\n";
    auto *separatorCtx =
        initializer.CreateContext("completion_separator_only.ets", ES2PANDA_STATE_CHECKED, separatorOnly.c_str());
    ASSERT_NE(separatorCtx, nullptr);
    ASSERT_TRUE(lspApi->getCompletionsAtPosition(separatorCtx, separatorOnly.size()).GetEntries().empty());
    initializer.DestroyContext(separatorCtx);
}

// Test: interface member completions distinguish plain property (synthesized
// getter with original property node), plain method ("()" insert text) and
// explicitly declared get/set accessors (METHOD entries without "()" suffix).
TEST_F(LSPCompletionsStmtKindEntriesTests, InterfaceGetterSetterAndPropertyArms)
{
    std::vector<std::string> files = {"stmt_kind_interface.ets"};
    std::vector<std::string> texts = {R"(
interface StmtKindIf {
    kindProp: number;
    plainMethod(): void;
    get accValue(): number;
    set accValue(v: number);
}
let stmtInst: StmtKindIf
let kindUse = stmtInst.
let a = 1;
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const std::string marker = "stmtInst.";
    const size_t offset = texts[0].find(marker) + marker.size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("kindProp: number", CompletionEntryKind::PROPERTY,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "kindProp"),
                                      CompletionEntry("plainMethod(): void", CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "plainMethod()"),
                                      CompletionEntry("accValue(): number", CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "accValue"),
                                      CompletionEntry("accValue(v: number): void", CompletionEntryKind::METHOD,
                                                      std::string(CLASS_MEMBER_SNIPPETS), "accValue")};
    ASSERT_EQ(entries.size(), 4U);
    AssertStmtKindEntries(entries, expectedEntries, {});
    // The lowered setter overload of the source property must not surface as a
    // duplicated method entry with "()" insert text.
    AssertStmtKindEntries(
        entries, {},
        {CompletionEntry("kindProp()", CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS), "kindProp()")});
    initializer.DestroyContext(ctx);
}

// Test: class member visibility flags. Completing on an instance from outside
// the class excludes private and protected members (default flag path);
// completing "this." inside a derived class includes its own private members
// and the base class protected members, but not the base private members.
TEST_F(LSPCompletionsStmtKindEntriesTests, ClassPrivateProtectedFlagArms)
{
    // Outside the class: private and protected members are skipped.
    ExpectOutsideClassExcludesPrivates();

    // Inside the derived class: own members (incl. private of this class via
    // the enclosing-class flag) and protected members of the base are offered.
    ExpectDerivedClassCompletions();
}
// Test: completing members of a namespace offers the nested enum as ENUM,
// the nested namespace as MODULE and the nested class as CLASS. The namespace
// is lowered to a class before checking, so these kinds are produced by the
// class-declaration completion arms for transformed declarations.
TEST_F(LSPCompletionsStmtKindEntriesTests, EnumAndNamespaceTransformedKindArms)
{
    std::vector<std::string> files = {"stmt_kind_ns_kinds.ets"};
    std::vector<std::string> texts = {R"(
namespace StmtNs {
    export enum StmtColor { Red = 1 }
    export namespace StmtInner {
    }
    export class StmtHost {
    }
}
let nsUse = StmtNs.
let a = 1;
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const std::string marker = "StmtNs.";
    const size_t offset = texts[0].find(marker) + marker.size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("StmtColor", CompletionEntryKind::ENUM,
                                                      std::string(MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), "StmtColor"),
                                      CompletionEntry("StmtInner", CompletionEntryKind::MODULE,
                                                      std::string(MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), "StmtInner"),
                                      CompletionEntry("StmtHost", CompletionEntryKind::CLASS,
                                                      std::string(MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), "StmtHost")};
    ASSERT_EQ(entries.size(), 3U);
    AssertStmtKindEntries(entries, expectedEntries, {});
    initializer.DestroyContext(ctx);
}

// Test: struct member completions offer properties (PROPERTY) and methods
// (METHOD with "()" insert text) of the struct through an instance receiver.
TEST_F(LSPCompletionsStmtKindEntriesTests, StructMemberKindArms)
{
    std::vector<std::string> files = {"stmt_kind_struct.ets"};
    std::vector<std::string> texts = {R"('use static'

struct StmtStruct {
    sid: int = 1;
    sname: string = "a";
    srun(): void {
    }
}

export function stmtFunc(arg1: StmtStruct): void {
    arg1.id += 1;
    arg1.
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const std::string marker = "arg1.";
    const size_t offset = texts[0].find(marker) + marker.size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("sid: int", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS), "sid"),
        CompletionEntry("sname: string", CompletionEntryKind::PROPERTY, std::string(SUGGESTED_CLASS_MEMBERS), "sname"),
        CompletionEntry("srun(): void", CompletionEntryKind::METHOD, std::string(CLASS_MEMBER_SNIPPETS), "srun()")};
    ASSERT_EQ(entries.size(), 3U);
    AssertStmtKindEntries(entries, expectedEntries, {});
    initializer.DestroyContext(ctx);
}

// Test: reachability probe for the TSModuleDeclaration completion family.
// With the namespace lowered to a class before checking, completing "MyNs."
// resolves the receiver to a namespace-transformed class definition, so the
// entries (class/interface/function/const) come from the class member path.
TEST_F(LSPCompletionsStmtKindEntriesTests, NamespaceMemberChainCompletion)
{
    std::vector<std::string> files = {"stmt_kind_ns_chain.ets"};
    std::vector<std::string> texts = {R"(
namespace StmtChainNs {
    export class ChainCls {
        chainValue: number = 1;
    }
    export interface ChainIf {
        chainProp: number;
    }
    export function chainFunc(): void {
    }
    export const chainConst = 10;
}
let chainUse = StmtChainNs.
let a = 1;
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const std::string marker = "StmtChainNs.";
    const size_t offset = texts[0].find(marker) + marker.size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    auto expectedEntries = std::vector<CompletionEntry> {
        CompletionEntry("ChainCls", CompletionEntryKind::CLASS, std::string(MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT),
                        "ChainCls"),
        // Interface members carry no insert text.
        CompletionEntry("ChainIf", CompletionEntryKind::INTERFACE, std::string(SUGGESTED_CLASS_MEMBERS), ""),
        CompletionEntry("chainFunc(): void", CompletionEntryKind::METHOD,
                        std::string(MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), "chainFunc()"),
        // Namespace-level const is offered as a property with its inferred type.
        CompletionEntry("chainConst: Int", CompletionEntryKind::PROPERTY, "", "chainConst")};
    AssertStmtKindEntries(entries, expectedEntries, {});
    initializer.DestroyContext(ctx);
}

// Test: reachability probe with an ambient namespace declaration. The ambient
// namespace is also lowered before checking, so completion after its name
// follows the same transformed-class path and offers the declared members.
TEST_F(LSPCompletionsStmtKindEntriesTests, AmbientNamespaceCompletion)
{
    std::vector<std::string> files = {"stmt_kind_ambient_ns.ets"};
    std::vector<std::string> texts = {R"(
declare namespace StmtAmbientNs {
    export class AmbCls {
    }
    export function ambFunc(): void {
    }
}
let ambUse = StmtAmbientNs.
let a = 1;
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const std::string marker = "StmtAmbientNs.";
    const size_t offset = texts[0].find(marker) + marker.size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("AmbCls", CompletionEntryKind::CLASS,
                                                      std::string(MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), "AmbCls"),
                                      CompletionEntry("ambFunc(): void", CompletionEntryKind::METHOD,
                                                      std::string(MEMBER_DECLARED_BY_SPREAD_ASSIGNMENT), "ambFunc()")};
    AssertStmtKindEntries(entries, expectedEntries, {});
    initializer.DestroyContext(ctx);
}

// Test: top-level let/const declarations lowered into the global class are
// offered by identifier completion as VARIABLE / CONSTANT entries; the kind
// for "let" is decided inside the global static-block assignment lookup.
TEST_F(LSPCompletionsStmtKindEntriesTests, GlobalVarCompletionInitEntryKinds)
{
    std::vector<std::string> files = {"stmt_kind_global_vars.ets"};
    std::vector<std::string> texts = {R"(
let stmtGlobalVar: number = 1;
const stmtGlobalConst: number = 2;
function stmtGUser(): number {
    return stmtGlob
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const std::string marker = "stmtGlob";
    const size_t offset = texts[0].find(marker) + marker.size();
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    auto expectedEntries =
        std::vector<CompletionEntry> {CompletionEntry("stmtGlobalVar: number", CompletionEntryKind::VARIABLE,
                                                      std::string(GLOBALS_OR_KEYWORDS), "stmtGlobalVar"),
                                      CompletionEntry("stmtGlobalConst: number", CompletionEntryKind::CONSTANT,
                                                      std::string(GLOBALS_OR_KEYWORDS), "stmtGlobalConst")};
    AssertStmtKindEntries(entries, expectedEntries, {});
    initializer.DestroyContext(ctx);
}

// Test: import-brace completion over an exported module feeds GetExportsFromProgram.
// A struct export keeps its dedicated STRUCT entry, an interface export goes through
// GetInterfaceDeclarationEntry, and a function export is a METHOD entry whose insert
// text carries no "()" suffix because the cursor sits inside the import statement.
TEST_F(LSPCompletionsStmtKindEntriesTests, ExportedStructAndFallbackArms)
{
    std::vector<std::string> files = {"@pa16_defs.d.ets", "pa16_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export struct Pa16Struct {
    pa16Field: number = 1;
}
export interface Pa16Iface {
}
export function pa16Fn(a: number): number {
    return a;
}
)",
                                      R"(
import { Pa16 } from './@pa16_defs'
import { pa16 } from './@pa16_defs'
export const pa16Used: number = 1;
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_PARSED);
    LSPAPI const *lspApi = GetImpl();

    ExpectExportedStructInterfaceFnEntries(lspApi, ctx, texts);
    initializer.DestroyContext(ctx);

    ExpectAnonDefaultExportNoEmptyEntries(initializer, lspApi);
}

// Test: GetExportsFromProgram callable entries render the parameter list and the
// return type; inside an import statement the insert text stays the bare function
// name (no "()" suffix). Explicit, void, unannotated and generic returns are pinned.
TEST_F(LSPCompletionsStmtKindEntriesTests, CallableEntryImportAndReturnArms)
{
    std::vector<std::string> files = {"@pa17_defs.d.ets", "pa17_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export function pa17Typed(a: number): string {
    return "";
}
export function pa17Void(): void {
}
export function pa17NoRet() {
}
export function pa17Gen<T>(t: T): T {
    return t;
}
)",
                                      R"(
import { pa17 } from './@pa17_defs'
export const pa17Used: number = 1;
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_PARSED);
    LSPAPI const *lspApi = GetImpl();

    const std::string marker = "import { pa17";
    const size_t offset = texts[1].find(marker) + marker.size();
    auto entries = lspApi->getCompletionsAtPosition(ctx, offset).GetEntries();

    const auto *typedEntry = FindStmtKindEntry(entries, "pa17Typed(a: number): string");
    ASSERT_NE(typedEntry, nullptr) << "explicit-return callable entry missing";
    EXPECT_EQ(typedEntry->GetCompletionKind(), CompletionEntryKind::METHOD);
    EXPECT_EQ(typedEntry->GetInsertText(), "pa17Typed");
    const auto *voidEntry = FindStmtKindEntry(entries, "pa17Void(): void");
    ASSERT_NE(voidEntry, nullptr) << "void-return callable entry missing";
    EXPECT_EQ(voidEntry->GetInsertText(), "pa17Void");
    const auto *noRetEntry = FindStmtKindEntry(entries, "pa17NoRet(): void");
    ASSERT_NE(noRetEntry, nullptr) << "unannotated-return callable entry missing";
    EXPECT_EQ(noRetEntry->GetInsertText(), "pa17NoRet");
    const auto *genEntry = FindStmtKindEntry(entries, "pa17Gen(t: T): T");
    ASSERT_NE(genEntry, nullptr) << "generic callable entry missing";
    EXPECT_EQ(genEntry->GetInsertText(), "pa17Gen");
    initializer.DestroyContext(ctx);
}

// Test: checked receiver completions expose interface fields and methods through the
// public completion API.
TEST_F(LSPCompletionsStmtKindEntriesTests, InterfaceEntryRemainingArms)
{
    std::vector<std::string> files = {"pa18_iface.ets"};
    std::vector<std::string> texts = {R"('use static'
interface Pa18If {
    pa18Prop: number;
    pa18Meth(v: string): boolean;
}
let pa18Inst: Pa18If
let pa18Use = pa18Inst.
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const std::string marker = "pa18Inst.";
    const size_t offset = texts[0].find(marker) + marker.size();
    auto entries = lspApi->getCompletionsAtPosition(ctx, offset).GetEntries();

    const auto *propEntry = FindStmtKindEntry(entries, "pa18Prop: number");
    ASSERT_NE(propEntry, nullptr) << "interface property completion missing";
    EXPECT_EQ(propEntry->GetCompletionKind(), CompletionEntryKind::PROPERTY);
    EXPECT_EQ(propEntry->GetInsertText(), "pa18Prop");
    EXPECT_EQ(propEntry->GetTypeSig(), "number");

    const auto *methodEntry = FindStmtKindEntry(entries, "pa18Meth(v: string): boolean");
    ASSERT_NE(methodEntry, nullptr) << "interface method completion missing";
    EXPECT_EQ(methodEntry->GetCompletionKind(), CompletionEntryKind::METHOD);
    EXPECT_EQ(methodEntry->GetInsertText(), "pa18Meth()");
    EXPECT_EQ(methodEntry->GetTypeSig(), "(string):boolean");
    initializer.DestroyContext(ctx);
}

// Test: an exported interface of an imported module reaches
// GetInterfaceDeclarationEntry's named arm; the INTERFACE entry carries the name as
// insert text and GLOBALS_OR_KEYWORDS sort text.
TEST_F(LSPCompletionsStmtKindEntriesTests, InterfaceDeclEntryFallbackArms)
{
    std::vector<std::string> files = {"@pa19_defs.d.ets", "pa19_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export interface Pa19Iface {
    pa19Field: number;
}
)",
                                      R"(
import { Pa19 } from './@pa19_defs'
export const pa19Used: number = 1;
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const std::string marker = "import { Pa19";
    const size_t offset = texts[1].find(marker) + marker.size();
    auto entries = lspApi->getCompletionsAtPosition(ctx, offset).GetEntries();

    const auto *ifaceEntry = FindStmtKindEntry(entries, "Pa19Iface");
    ASSERT_NE(ifaceEntry, nullptr) << "exported interface entry missing";
    EXPECT_EQ(ifaceEntry->GetCompletionKind(), CompletionEntryKind::INTERFACE);
    EXPECT_EQ(ifaceEntry->GetSortText(), std::string(GLOBALS_OR_KEYWORDS));
    EXPECT_EQ(ifaceEntry->GetInsertText(), "Pa19Iface");
    initializer.DestroyContext(ctx);
}

// Test: symbol-reference-index-backed completion classifies exported declarations by
// their indexed declaration kind (alias/class/interface/function) and suppresses an
// indexed suggestion whose name already completes from scope.
TEST_F(LSPCompletionsStmtKindEntriesTests, SymbolIndexedApiArms)
{
    std::vector<std::string> files = {"pa20_defs.ets", "pa20_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export enum PaIdxColor {
    Red,
}
export namespace PaIdxSpace {
}
export type PaIdxAlias = number;
export let paIdxVar: number = 1;
export class PaIdxGear {
}
export class PaIdxMachine {
}
export interface PaIdxStyle {
}
export function paIdxRun(a: number): number {
    return a;
}
)",
                                      R"('use static'
class PaIdxMachine {
}
let paIdxUse = PaIdx
export const pa20Used: number = 1;
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    LSPAPI const *lspApi = GetImpl();
    lspApi->initSymbolReferenceIndex();
    Initializer initializer = Initializer();
    auto *ctxDefs = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_BOUND);
    ASSERT_NE(ctxDefs, nullptr);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(ctxDefs));
    auto *ctxQuery = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxQuery, nullptr);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(ctxQuery));

    const std::string marker = "let paIdxUse = PaIdx";
    const size_t offset = texts[1].find(marker) + marker.size();
    auto entries = lspApi->getCompletionsAtPosition(ctxQuery, offset).GetEntries();
    initializer.DestroyContext(ctxQuery);
    initializer.DestroyContext(ctxDefs);
    lspApi->clearSymbolReferenceIndex();

    ExpectIndexedApiEntryShapes(entries);
}

// Test: InitEntry scope-decl arms. A function parameter is offered as VARIABLE at
// CHECKED; at BOUND the unlowered top-level let/const declarations still are
// VariableDeclarations, so they flow through the declarator split and keep
// VARIABLE/CONSTANT kinds.
TEST_F(LSPCompletionsStmtKindEntriesTests, InitEntryRemainingLiveArms)
{
    std::vector<std::string> paramFiles = {"pa21_param.ets"};
    std::vector<std::string> paramTexts = {R"('use static'
function pa21Fn(pa21Param: number): void {
    let pa21Use = pa21Par
}
)"};
    auto paramPaths = CreateTempFile(paramFiles, paramTexts);
    ASSERT_EQ(paramPaths.size(), 1U);
    Initializer initializer = Initializer();
    auto paramCtx = initializer.CreateContext(paramPaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const std::string paramMarker = "pa21Par";
    const size_t paramOffset = paramTexts[0].find(paramMarker) + paramMarker.size();
    auto paramEntries = lspApi->getCompletionsAtPosition(paramCtx, paramOffset).GetEntries();
    // The parameter completes as a VARIABLE scope entry and the enclosing function
    // surfaces through the InitEntry method arm as a FUNCTION entry.
    auto expectedParamEntries =
        std::vector<CompletionEntry> {CompletionEntry("pa21Param: number", CompletionEntryKind::VARIABLE,
                                                      std::string(GLOBALS_OR_KEYWORDS), "pa21Param"),
                                      CompletionEntry("pa21Fn(pa21Param: number): void", CompletionEntryKind::FUNCTION,
                                                      std::string(GLOBALS_OR_KEYWORDS), "pa21Fn()")};
    ASSERT_EQ(paramEntries.size(), 2U);
    AssertStmtKindEntries(paramEntries, expectedParamEntries, {});
    initializer.DestroyContext(paramCtx);

    std::vector<std::string> varFiles = {"pa21_vars.ets"};
    std::vector<std::string> varTexts = {R"('use static'
let pa21LetVar: number = 1;
const pa21ConstVar: number = 2;
let [pa21DestrA, pa21DestrB] = [1, 2];
let pa21Probe = pa21
)"};
    auto varPaths = CreateTempFile(varFiles, varTexts);
    ASSERT_EQ(varPaths.size(), 1U);
    auto varCtx = initializer.CreateContext(varPaths[0].c_str(), ES2PANDA_STATE_BOUND);
    const std::string varMarker = "= pa21";
    const size_t varOffset = varTexts[0].find(varMarker) + varMarker.size();
    auto varEntries = lspApi->getCompletionsAtPosition(varCtx, varOffset).GetEntries();
    auto expectedVarEntries = std::vector<CompletionEntry> {
        CompletionEntry("pa21LetVar: number", CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS),
                        "pa21LetVar"),
        CompletionEntry("pa21ConstVar: number", CompletionEntryKind::CONSTANT, std::string(GLOBALS_OR_KEYWORDS),
                        "pa21ConstVar"),
        // Unlike at CHECKED state, the destructured pattern bindings do surface at
        // BOUND as bare VARIABLE entries.
        CompletionEntry("pa21DestrA", CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS), "pa21DestrA"),
        CompletionEntry("pa21DestrB", CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS), "pa21DestrB"),
        CompletionEntry("pa21Probe", CompletionEntryKind::VARIABLE, std::string(GLOBALS_OR_KEYWORDS), "pa21Probe")};
    ASSERT_EQ(varEntries.size(), 5U);
    AssertStmtKindEntries(varEntries, expectedVarEntries, {});
    initializer.DestroyContext(varCtx);
}

// Test: type-signature rendering for interface members: union, array, generic and
// qualified type annotations in the display name (checker-dumped, single-spaced) and
// the compact type signature; a method signature renders parameter and return types.
TEST_F(LSPCompletionsStmtKindEntriesTests, TypeSigMatrix)
{
    std::vector<std::string> files = {"pa22_sig.ets"};
    std::vector<std::string> texts = {R"('use static'
namespace Pa22Ns {
    export class Pa22In {
    }
}
interface Pa22If {
    pa22Union: string | number;
    pa22Arr: number[];
    pa22Gen: Map<string, number>;
    pa22Qual: Pa22Ns.Pa22In;
    pa22Meth(v: string): boolean;
}
let pa22Inst: Pa22If
let pa22Use = pa22Inst.
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const std::string marker = "pa22Inst.";
    const size_t offset = texts[0].find(marker) + marker.size();
    auto entries = lspApi->getCompletionsAtPosition(ctx, offset).GetEntries();

    auto expectSig = [&entries](const std::string &name, const std::string &typeSig) {
        const auto *entry = FindStmtKindEntry(entries, name);
        ASSERT_NE(entry, nullptr) << "completion entry missing: " << name;
        EXPECT_EQ(entry->GetTypeSig(), typeSig) << "type signature mismatch: " << name;
    };
    expectSig("pa22Union: (string | number)", "(string|number)");
    expectSig("pa22Arr: Array<number>", "Array<number>");
    expectSig("pa22Gen: Map<string, number>", "Map<string,number>");
    expectSig("pa22Qual: Pa22Ns.Pa22In", "Pa22Ns.Pa22In");
    expectSig("pa22Meth(v: string): boolean", "(string):boolean");
    initializer.DestroyContext(ctx);
}

// Test: scope-decl entries pass through ProcessAutoImportForEntry without
// CompletionEntryData, so the helper's early-return arm keeps them unchanged; the
// collected-API path, by contrast, attaches auto-import data to its suggestions.
TEST_F(LSPCompletionsStmtKindEntriesTests, AutoImportArms)
{
    std::vector<std::string> files = {"pa23_scope.ets"};
    std::vector<std::string> texts = {R"('use static'
let pa23Local: number = 1;
function pa23User(): number {
    return pa23Loc
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    const std::string marker = "pa23Loc";
    const size_t offset = texts[0].find(marker) + marker.size();
    auto entries = lspApi->getCompletionsAtPosition(ctx, offset).GetEntries();
    const auto *localEntry = FindStmtKindEntry(entries, "pa23Local: number");
    ASSERT_NE(localEntry, nullptr) << "scope-decl entry missing";
    CompletionEntry localCopy(*localEntry);
    EXPECT_FALSE(localCopy.GetCompletionEntryData().has_value()) << "scope-decl entry must not gain auto-import data";
    initializer.DestroyContext(ctx);

    std::vector<std::string> sdkFiles = {"@pa23_sdk.d.ets", "pa23_query.ets"};
    std::vector<std::string> sdkTexts = {R"('use static'
export function pa23SdkFn(): void {
}
)",
                                         R"(
import { pa23SdkFn } from './@pa23_sdk';
let pa23Q = Pa23Sdk
)"};
    auto sdkPaths = CreateTempFile(sdkFiles, sdkTexts);
    ASSERT_EQ(sdkPaths.size(), sdkFiles.size());
    auto sdkCtx = initializer.CreateContext(sdkPaths[1].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_TRUE(lspApi->collectApiInfo(sdkCtx));
    const std::string sdkMarker = "Pa23Sdk";
    const size_t sdkOffset = sdkTexts[1].find(sdkMarker) + sdkMarker.size();
    auto sdkEntries = lspApi->getCompletionsAtPosition(sdkCtx, sdkOffset).GetEntries();
    const auto *sdkEntry = FindStmtKindEntry(sdkEntries, "pa23SdkFn");
    ASSERT_NE(sdkEntry, nullptr) << "collected auto-import suggestion missing";
    CompletionEntry sdkCopy(*sdkEntry);
    EXPECT_TRUE(sdkCopy.GetCompletionEntryData().has_value()) << "collected suggestion lacks auto-import data";
    EXPECT_EQ(sdkCopy.GetInsertText(), "pa23SdkFn()");
    initializer.DestroyContext(sdkCtx);
}

// Test: global completion in a type-annotation position offers interface declarations
// whose name ends with "Interface" (type-context suffix rule plus the scope-decl
// path), while a value position skips interface declarations entirely.
TEST_F(LSPCompletionsStmtKindEntriesTests, GlobalCompletionArms)
{
    std::vector<std::string> files = {"pa24_global.ets"};
    std::vector<std::string> texts = {R"('use static'
interface Pa24ShapeInterface {
    pa24Side: number;
}
function pa24Draw(target: Pa24Sh): void {
}
let pa24Val = Pa24Sh
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    ExpectTypePositionInterfaceEntries(initializer, lspApi, ctx, filePaths, texts);
    initializer.DestroyContext(ctx);

    ExpectCommentPositionEmpty(initializer, lspApi);
}

// Test: property-completion receiver normalization for call, constructed-instance
// and qualified-new receivers; a call-of-call receiver normalizes to nullptr and
// yields no members.
TEST_F(LSPCompletionsStmtKindEntriesTests, ReceiverNormalizationArms)
{
    std::vector<std::string> files = {"pa25_recv.ets"};
    std::vector<std::string> texts = {R"('use static'
class Pa25Cls {
    pa25Member: number = 1;
    static pa25Create(): Pa25Cls {
        return new Pa25Cls();
    }
}
namespace Pa25Ns {
    export class Pa25In {
        pa25InField: number = 2;
    }
}
function pa25Make(): Pa25Cls {
    return new Pa25Cls();
}
function pa25Use(): number {
    let pa25A = new Pa25Cls().
    let pa25B = Pa25Cls.pa25Create().
    let pa25C = new Pa25Ns.Pa25In().
    let pa25D = pa25Make()().
    let pa25E = new Pa25Cls()().
    return pa25A + pa25B + pa25C + pa25D + pa25E;
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto completionsAfter = [&lspApi, &ctx, &texts](const std::string &marker) {
        const size_t offset = texts[0].find(marker) + marker.size();
        return lspApi->getCompletionsAtPosition(ctx, offset).GetEntries();
    };
    auto constructEntries = completionsAfter("let pa25A = new Pa25Cls().");
    EXPECT_NE(FindStmtKindEntry(constructEntries, "pa25Member: number"), nullptr)
        << "constructed-instance receiver members missing";
    auto callEntries = completionsAfter("let pa25B = Pa25Cls.pa25Create().");
    // CURRENT BEHAVIOR: the call receiver resolves to the callee declaration, whose
    // class-typed return is completed in a static context, so only static members of
    // the return type are offered.
    EXPECT_NE(FindStmtKindEntry(callEntries, "pa25Create(): Pa25Cls"), nullptr)
        << "static-call receiver completion missing";
    auto qualifiedEntries = completionsAfter("let pa25C = new Pa25Ns.Pa25In().");
    EXPECT_NE(FindStmtKindEntry(qualifiedEntries, "pa25InField: number"), nullptr)
        << "qualified constructed-instance receiver members missing";
    auto chainEntries = completionsAfter("let pa25D = pa25Make()().");
    EXPECT_TRUE(chainEntries.empty()) << "call-of-call receiver must produce no member entries";
    auto constructedChainEntries = completionsAfter("let pa25E = new Pa25Cls()().");
    EXPECT_TRUE(constructedChainEntries.empty()) << "constructed-call receiver must produce no member entries";
    initializer.DestroyContext(ctx);
}

}  // namespace
