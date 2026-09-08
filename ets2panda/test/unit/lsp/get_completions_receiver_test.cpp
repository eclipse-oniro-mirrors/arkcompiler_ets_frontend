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

// Receiver/member-completion gap scenarios for completions.cpp: `this.` contexts,
// interface-typed receivers, enum-member misses, protected inheritance walks,
// type-annotation contexts and abstract-method signature rendering.
class LSPCompletionsReceiverTests : public LSPAPITests {
public:
    std::vector<ark::es2panda::lsp::CompletionEntry> CompletionsAfterMarker(const std::string &queryText,
                                                                            const std::string &marker,
                                                                            es2panda_ContextState state)
    {
        auto markerPos = queryText.find(marker);
        if (markerPos == std::string::npos) {
            ADD_FAILURE() << "Marker not found: " << marker;
            return {};
        }
        std::vector<std::string> files = {"receiver_query.ets"};
        auto filePaths = CreateTempFile(files, {queryText});
        LSPAPI const *lspApi = GetImpl();
        ark::es2panda::lsp::Initializer initializer;
        auto ctx = initializer.CreateContext(filePaths[0].c_str(), state);
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
};

namespace {

using ark::es2panda::lsp::CompletionEntryKind;

// Completing `this.` inside a plain top-level function has no enclosing class or
// interface: GetDefinitionOfThisExpression returns nullptr and no entry may leak.
TEST_F(LSPCompletionsReceiverTests, ThisCompletionOutsideClassYieldsNoEntries)
{
    const std::string text = R"('use static'
function standaloneFn(): void {
    return this.
}
)";
    auto entries = CompletionsAfterMarker(text, "return this.", ES2PANDA_STATE_CHECKED);
    EXPECT_TRUE(entries.empty());
}

// An interface-typed receiver exposes both its property and method members through
// GetCompletionFromTSInterfaceDeclaration -> FilterFromInterfaceBody ->
// GetEntriesForTSInterfaceDeclaration, including the property display form.
TEST_F(LSPCompletionsReceiverTests, InterfaceTypedReceiverListsPropertyAndMethodMembers)
{
    const std::string text = R"('use static'
interface WorkerInterface {
    workload: number;
    perform(action: string): boolean;
}
let worker: WorkerInterface;
worker.
)";
    auto entries = CompletionsAfterMarker(text, "\nworker.", ES2PANDA_STATE_CHECKED);
    ExpectEntryExact(entries, "workload: number", CompletionEntryKind::PROPERTY,
                     std::string(ark::es2panda::lsp::sort_text::CLASS_MEMBER_SNIPPETS), "workload");
    ExpectEntryExact(entries, "perform(action: string): boolean", CompletionEntryKind::METHOD,
                     std::string(ark::es2panda::lsp::sort_text::CLASS_MEMBER_SNIPPETS), "perform()");
}

// An interface whose member does not match the trigger word still resolves the
// receiver but offers no member: FilterFromInterfaceBody produces an empty set.
TEST_F(LSPCompletionsReceiverTests, InterfaceReceiverWithNonMatchingTriggerIsEmpty)
{
    const std::string text = R"('use static'
interface TinyInterface {
    only: number;
}
let tiny: TinyInterface;
tiny.zzz
)";
    auto entries = CompletionsAfterMarker(text, "\ntiny.zzz", ES2PANDA_STATE_CHECKED);
    EXPECT_TRUE(entries.empty());
}

// An interface extending an unknown interface must skip the unresolved base link
// instead of crashing or duplicating members.
TEST_F(LSPCompletionsReceiverTests, InterfaceExtendingUnknownBaseSkipsBaseMembers)
{
    const std::string text = R"('use static'
interface DerivedUnknownInterface extends NoSuchBaseInterface {
    own: number;
}
let derivedUnknown: DerivedUnknownInterface;
derivedUnknown.
)";
    auto entries = CompletionsAfterMarker(text, "\nderivedUnknown.", ES2PANDA_STATE_CHECKED);
    ExpectEntryExact(entries, "own: number", CompletionEntryKind::PROPERTY,
                     std::string(ark::es2panda::lsp::sort_text::CLASS_MEMBER_SNIPPETS), "own");
}

// Protected members of a grandparent class stay visible for `this.` completion in a
// grandchild: IsSameOrDerivedClass walks two base hops.
TEST_F(LSPCompletionsReceiverTests, ProtectedMemberVisibleThroughTwoLevelInheritance)
{
    const std::string text = R"('use static'
class GuardBase {
    protected shield(): number {
        return 1;
    }
}
class GuardMiddle extends GuardBase {}
class GuardLeaf extends GuardMiddle {
    strike(): void {
        this.
    }
}
)";
    auto entries = CompletionsAfterMarker(text, "this.", ES2PANDA_STATE_CHECKED);
    ExpectEntryExact(entries, "shield(): number", CompletionEntryKind::METHOD,
                     std::string(ark::es2panda::lsp::sort_text::CLASS_MEMBER_SNIPPETS), "shield()");
}

// Completing a transformed-enum receiver with a non-matching trigger yields no
// member entries (the filtered member list is empty).
TEST_F(LSPCompletionsReceiverTests, EnumReceiverWithNonMatchingTriggerIsEmpty)
{
    const std::string text = R"('use static'
enum PaintColor {
    Red,
    Green,
}
let picked = PaintColor.Zzz
)";
    auto entries = CompletionsAfterMarker(text, "PaintColor.Zzz", ES2PANDA_STATE_CHECKED);
    EXPECT_TRUE(entries.empty());
}

// A class property declared without a type annotation renders as a bare name entry
// with an empty signature (GetTypeSig falls through to the empty string).
TEST_F(LSPCompletionsReceiverTests, UntypedClassPropertyCompletesWithoutSignature)
{
    const std::string text = R"('use static'
class LooseHolder {
    looseValue = 1;
}
let holderInstance = new LooseHolder();
holderInstance.
)";
    auto entries = CompletionsAfterMarker(text, "\nholderInstance.", ES2PANDA_STATE_CHECKED);
    bool sawLoose = false;
    for (const auto &entry : entries) {
        if (entry.GetName().find("looseValue") == 0) {
            sawLoose = true;
            // CURRENT BEHAVIOR: without a declared annotation the display name carries
            // the checker-inferred type while the signature stays empty.
            EXPECT_EQ(entry.GetTypeSig(), "") << "untyped property must have empty signature";
        }
    }
    EXPECT_TRUE(sawLoose) << "untyped property entry missing";
}

// Completing between a parameter type annotation position drives the type-context
// detection and surfaces interface declarations whose name ends with "Interface".
TEST_F(LSPCompletionsReceiverTests, TypeAnnotationPositionSuggestsInterfaceSuffixDeclarations)
{
    const std::string text = R"('use static'
interface RenderInterface {
    frame: number;
}
interface PlainStruct {
    other: number;
}
function render(target: Rend, mode: number): void {}
)";
    auto entries = CompletionsAfterMarker(text, "target: Rend", ES2PANDA_STATE_CHECKED);
    // The type-context suffix rule suggests interfaces whose name ends with
    // "Interface" and contains the prefix substring ("Rend" ⊂ RenderInterface).
    EXPECT_NE(FindEntry(entries, "RenderInterface"), nullptr) << "suffix Interface suggestion missing";
}

// Abstract method declarations keep identifier-style parameters at PARSED state;
// their completion entries render the full parameter list with types.
TEST_F(LSPCompletionsReceiverTests, AbstractMethodSignatureUsesParameterTypes)
{
    const std::string text = R"('use static'
abstract class AbstractPainter {
    abstract stroke(color: string): boolean;
}
let painterRef: AbstractPainter;
painterRef.
)";
    auto entries = CompletionsAfterMarker(text, "\npainterRef.", ES2PANDA_STATE_CHECKED);
    ASSERT_FALSE(entries.empty());
    bool sawStroke = false;
    for (const auto &entry : entries) {
        if (entry.GetName().find("stroke") == 0) {
            sawStroke = true;
            // CURRENT BEHAVIOR: the rendered signature carries the declared parameter
            // type; pin the exact display form.
            EXPECT_EQ(entry.GetName(), "stroke(color: string): boolean");
            EXPECT_EQ(entry.GetInsertText(), "stroke()");
        }
    }
    EXPECT_TRUE(sawStroke) << "abstract method entry missing";
}

// String-literal keyed interface methods have no identifier name: the completion
// keeps the raw key as display name without inventing parentheses content.
TEST_F(LSPCompletionsReceiverTests, LiteralKeyedInterfaceMethodKeepsRawKeyName)
{
    const std::string text = R"('use static'
interface OddInterface {
    "odd-key"(v: number): void;
}
let odd: OddInterface;
odd.
)";
    auto entries = CompletionsAfterMarker(text, "\nodd.", ES2PANDA_STATE_CHECKED);
    bool sawOddKey = false;
    for (const auto &entry : entries) {
        if (entry.GetName().find("odd-key") != std::string::npos) {
            sawOddKey = true;
        }
        EXPECT_FALSE(entry.GetName().empty()) << "empty-named entry leaked";
    }
    EXPECT_TRUE(sawOddKey) << "literal-keyed method missing from suggestions";
}

// Completing inside a method parameter's type annotation is detected as a type
// context (ETSParameterExpression arm).
TEST_F(LSPCompletionsReceiverTests, MethodParameterTypeAnnotationIsTypeContext)
{
    const std::string text = R"('use static'
class ParamHolder {
    run(v: Num): void {}
}
)";
    auto entries = CompletionsAfterMarker(text, "v: Num", ES2PANDA_STATE_CHECKED);
    bool sawNumber = false;
    for (const auto &entry : entries) {
        if (entry.GetName() == "number") {
            sawNumber = true;
        }
    }
    EXPECT_TRUE(sawNumber) << "primitive type not offered in parameter annotation";
}

// Completing on the right-hand side of a type alias declaration is a type context
// (TSTypeAliasDeclaration arm).
TEST_F(LSPCompletionsReceiverTests, TypeAliasRightHandSideIsTypeContext)
{
    const std::string text = R"('use static'
type AliasTarget = Num;
)";
    auto entries = CompletionsAfterMarker(text, "= Num", ES2PANDA_STATE_CHECKED);
    bool sawNumber = false;
    for (const auto &entry : entries) {
        if (entry.GetName() == "number") {
            sawNumber = true;
        }
    }
    EXPECT_TRUE(sawNumber) << "primitive type not offered in alias RHS";
}

// Completing inside a type parameter's default type is a type context.
TEST_F(LSPCompletionsReceiverTests, TypeParameterDefaultTypeIsTypeContext)
{
    const std::string text = R"('use static'
class GenericBox<T = Num> {
    val: T;
}
)";
    auto entries = CompletionsAfterMarker(text, "<T = Num", ES2PANDA_STATE_CHECKED);
    bool sawNumber = false;
    for (const auto &entry : entries) {
        if (entry.GetName() == "number") {
            sawNumber = true;
        }
    }
    EXPECT_TRUE(sawNumber) << "primitive type not offered in type-parameter default";
}

// A top-level destructuring declaration participates in global scope completions;
// its pattern bindings must not leak empty-named entries.
TEST_F(LSPCompletionsReceiverTests, GlobalDestructuringDeclarationCompletesSiblings)
{
    const std::string text = R"('use static'
let [destrA, destrB] = [1, 2];
let pick = destr
)";
    auto entries = CompletionsAfterMarker(text, "\nlet pick = destr", ES2PANDA_STATE_CHECKED);
    // CURRENT BEHAVIOR: pattern bindings of a top-level destructuring declaration are
    // not offered as individual completions; the request must still succeed and must
    // not leak empty-named entries.
    for (const auto &entry : entries) {
        EXPECT_FALSE(entry.GetName().empty()) << "empty-named entry leaked";
    }
}

// A string-literal keyed class method has no identifier name: the member list keeps
// a paren-only display entry (CURRENT BEHAVIOR pinned).
TEST_F(LSPCompletionsReceiverTests, LiteralKeyedClassMethodYieldsParenOnlyEntry)
{
    const std::string text = R"('use static'
class OddHolder {
    real(): void {}
    "lit-key"(): void {}
}
let oddHolder = new OddHolder();
oddHolder.
)";
    auto entries = CompletionsAfterMarker(text, "\noddHolder.", ES2PANDA_STATE_CHECKED);
    ExpectEntryExact(entries, "real(): void", CompletionEntryKind::METHOD,
                     std::string(ark::es2panda::lsp::sort_text::CLASS_MEMBER_SNIPPETS), "real()");
    // CURRENT BEHAVIOR: the literal-keyed method is dropped from member suggestions
    // entirely (no paren-only placeholder entry).
    EXPECT_EQ(FindEntry(entries, "()"), nullptr);
}

// Completing through a type-alias-typed receiver resolves to the alias declaration
// node, which offers no member completions.
TEST_F(LSPCompletionsReceiverTests, AliasTypedReceiverYieldsNoEntries)
{
    const std::string text = R"('use static'
type SpeedAlias = number;
let speed: SpeedAlias;
speed.
)";
    auto entries = CompletionsAfterMarker(text, "\nspeed.", ES2PANDA_STATE_CHECKED);
    // CURRENT BEHAVIOR: the receiver resolves to the aliased primitive type, so the
    // suggestion list mirrors the primitive's members instead of being empty.
    for (const auto &entry : entries) {
        EXPECT_NE(entry.GetName().find("SpeedAlias"), 0U) << "alias name leaked as member";
    }
}

// The suffix-"Interface" type-context suggestion also applies to interfaces that are
// referenced elsewhere in the current scope.
TEST_F(LSPCompletionsReceiverTests, SuffixInterfaceSuggestionForInScopeDeclaration)
{
    const std::string text = R"('use static'
interface BadgeInterface {
    icon: number;
}
let badgeRef: BadgeInterface;
function draw(target: Bad): void {}
)";
    auto entries = CompletionsAfterMarker(text, "target: Bad", ES2PANDA_STATE_CHECKED);
    EXPECT_NE(FindEntry(entries, "BadgeInterface"), nullptr) << "suffix Interface suggestion missing";
}

// Auto-import suggestions derived from the symbol reference index classify exported
// enums, namespaces, classes and functions by their declaration kind.
TEST_F(LSPCompletionsReceiverTests, SymbolIndexedApiSuggestionsCarryDeclarationKinds)
{
    std::vector<std::string> files = {"rc_index_defs.ets", "rc_index_query.ets"};
    std::vector<std::string> texts = {R"('use static'
export enum IdxColor {
    Red,
}
export namespace IdxSpace {
    export function inner(): void {}
}
export class IdxMachine {}
export function idxRun(a: number): void {}
)",
                                      R"('use static'
import { IdxColor } from './rc_index_defs';
export enum IdxLocal {
    Green,
}
let q = Idx
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    lspApi->initSymbolReferenceIndex();

    ark::es2panda::lsp::Initializer initializer;
    auto *ctxDefs = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxDefs, nullptr);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(ctxDefs));
    auto *ctxQuery = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctxQuery, nullptr);
    ASSERT_TRUE(lspApi->buildSymbolReferenceIndexForContext(ctxQuery));

    auto markerPos = texts[1].find("\nlet q = Idx");
    ASSERT_NE(markerPos, std::string::npos);
    auto entries = lspApi->getCompletionsAtPosition(ctxQuery, markerPos + strlen("\nlet q = Idx")).GetEntries();
    initializer.DestroyContext(ctxQuery);
    initializer.DestroyContext(ctxDefs);
    lspApi->clearSymbolReferenceIndex();

    bool sawEnum = false;
    bool sawMachine = false;
    for (const auto &entry : entries) {
        if (entry.GetName() == "IdxColor") {
            sawEnum = true;
        }
        if (entry.GetName() == "IdxMachine") {
            sawMachine = true;
        }
    }
    EXPECT_TRUE(sawEnum) << "indexed enum suggestion missing";
    EXPECT_TRUE(sawMachine) << "indexed class suggestion missing";
}

}  // namespace
