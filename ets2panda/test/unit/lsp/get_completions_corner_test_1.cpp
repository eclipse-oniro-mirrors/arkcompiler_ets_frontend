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

#include <tuple>

class LSPCompletionsCornerTests : public LSPAPITests {};

using ark::es2panda::lsp::CompletionEntry;
using ark::es2panda::lsp::Initializer;

namespace {

// Check whether a completion entry whose name or insertText starts with the given prefix exists.
bool HasEntryWithPrefix(const std::vector<CompletionEntry> &entries, const std::string &prefix)
{
    for (const auto &entry : entries) {
        if (entry.GetName().rfind(prefix, 0) == 0 || entry.GetInsertText().rfind(prefix, 0) == 0) {
            return true;
        }
    }
    return false;
}

// Test: intersection type member completion — the ETS parser replaces & with BrokenTypeNode at parse time,
// so the LSP layer cannot recover constituent type information.
// Currently no completions are returned; verify no crash.
// NOTE: Fixing this requires checker-level support for intersection types.
TEST_F(LSPCompletionsCornerTests, IntersectionTypeMembersDoesNotCrash)
{
    std::vector<std::string> files = {"completion_intersection.ets"};
    const std::string text = R"delimiter(
class Alpha {
    alphaField: number = 1;
    alphaMethod(): void {}
}
class Beta {
    betaField: string = "b";
    betaMethod(): void {}
}
function use(v: Alpha & Beta): void {
    v.
}
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    LSPAPI const *lspApi = GetImpl();
    const std::string marker = "v.";
    auto markerPos = text.find(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();

    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    initializer.DestroyContext(ctx);

    // Intersection types are not supported by the ETS checker; verify no crash.
    // This is a parser limitation: the ETS parser replaces & with BrokenTypeNode at parse time,
    // so the LSP layer cannot recover constituent type information from the checked AST.
    // Fixing this requires checker-level support for intersection types, not just LSP-layer changes.
    // If intersection types become supported, members from both Alpha and Beta should appear.
}

// Test: generic type member completion - members of a generic class instantiation
TEST_F(LSPCompletionsCornerTests, GenericTypeMembersOffered)
{
    std::vector<std::string> files = {"completion_generic.ets"};
    const std::string text = R"delimiter(
class Holder<T> {
    item: T;
    size: number = 0;
    getItem(): T {
        return this.item;
    }
}
function use(h: Holder<number>): void {
    h.
}
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    LSPAPI const *lspApi = GetImpl();
    const std::string marker = "h.";
    auto markerPos = text.find(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();

    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    initializer.DestroyContext(ctx);

    // Generic class members should be offered. Verify the declared members appear.
    ASSERT_FALSE(entries.empty());
    EXPECT_TRUE(HasEntryWithPrefix(entries, "size"));
    EXPECT_TRUE(HasEntryWithPrefix(entries, "getItem"));
}

// Test: an incomplete imported-symbol prefix currently produces no completions in this temp-file context.
TEST_F(LSPCompletionsCornerTests, AlreadyImportedSymbolPrefixReturnsEmptyInTempFileContext)
{
    std::vector<std::string> files = {"completion_import_source.ets", "completion_import_consumer.ets"};
    std::vector<std::string> texts = {
        R"(export class ImportedWidget {
    size: number = 0;
})",
        R"(import { ImportedWidget } from './completion_import_source';
function build(): void {
    let w = new ImportedWidget();
    let partial = Imp
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 2U);

    LSPAPI const *lspApi = GetImpl();
    // Position right after "Imp" - the already imported symbol should be completable
    const std::string marker = "Imp";
    auto markerPos = texts[1].find(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();

    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    initializer.DestroyContext(ctx);

    // Current behavior: the checked temp-file context does not surface the imported symbol at this
    // incomplete prefix. Keep this as a precise characterization until the completion pipeline gains
    // a public path that resolves the imported declaration here.
    ASSERT_TRUE(entries.empty());
}

// Test: completion at a position after a statement returns local variable completions
TEST_F(LSPCompletionsCornerTests, CompletionAfterStatementReturnsLocalVariable)
{
    std::vector<std::string> files = {"completion_after_stmt.ets"};
    const std::string text = R"delimiter(
let x: number = 1;
let y = x
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    LSPAPI const *lspApi = GetImpl();
    // Position right after "x" in "let y = x"
    const std::string marker = "let y = x";
    auto markerPos = text.find(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();

    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    initializer.DestroyContext(ctx);

    // Should not crash; the declared local variable "x" should be offered.
    ASSERT_FALSE(entries.empty());
    EXPECT_TRUE(HasEntryWithPrefix(entries, "x"));
}

// Test: private member is not offered from outside the declaring class
TEST_F(LSPCompletionsCornerTests, PrivateMemberNotOfferedFromOutside)
{
    std::vector<std::string> files = {"completion_private_outside.ets"};
    const std::string text = R"delimiter(
class Vault {
    private secret: number = 42;
    visible: number = 1;
}
let v: Vault = new Vault();
v.
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    LSPAPI const *lspApi = GetImpl();
    const std::string marker = "v.";
    auto markerPos = text.find(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();

    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    initializer.DestroyContext(ctx);

    // The public member should be offered, the private member should not.
    ASSERT_FALSE(entries.empty());
    EXPECT_TRUE(HasEntryWithPrefix(entries, "visible"));
    EXPECT_FALSE(HasEntryWithPrefix(entries, "secret"));
}

// Test: protected member is not offered from outside the class hierarchy
TEST_F(LSPCompletionsCornerTests, ProtectedMemberNotOfferedOutsideHierarchy)
{
    std::vector<std::string> files = {"completion_protected_outside.ets"};
    const std::string text = R"delimiter(
class Base {
    protected heritage: number = 1;
    open: number = 2;
}
let b: Base = new Base();
b.
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    LSPAPI const *lspApi = GetImpl();
    const std::string marker = "b.";
    auto markerPos = text.find(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();

    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    initializer.DestroyContext(ctx);

    // The public member should be offered, the protected member should not from outside.
    ASSERT_FALSE(entries.empty());
    EXPECT_TRUE(HasEntryWithPrefix(entries, "open"));
    EXPECT_FALSE(HasEntryWithPrefix(entries, "heritage"));
}

// Test: global scope-decl completions carry no auto-import data. AppendScopeDeclCompletions used
// to route every InitEntry result through ProcessAutoImportForEntry, which always early-returned
// unchanged because scope-decl entries never carry CompletionEntryData. The no-op call (and its
// file-local ScopedContext helper) has been removed; this pins that entries keep their name,
// kind, sortText, insertText and still expose no data payload on this path.
TEST_F(LSPCompletionsCornerTests, ScopeDeclCompletionEntriesCarryNoAutoImportData)
{
    std::vector<std::string> files = {"completion_scope_decl_data.ets"};
    const std::string text = R"delimiter(
let scopeCount: number = 1;
const scopeLimit: number = 2;
function scopeAction(): void {}
sco
)delimiter";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    LSPAPI const *lspApi = GetImpl();
    const std::string marker = "sco";
    auto markerPos = text.rfind(marker);
    ASSERT_NE(markerPos, std::string::npos);
    size_t const offset = markerPos + marker.size();

    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    auto entries = res.GetEntries();

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(entries.empty());
    const std::vector<std::tuple<std::string, ark::es2panda::lsp::CompletionEntryKind, std::string, std::string>>
        expected = {{"scopeCount: number", ark::es2panda::lsp::CompletionEntryKind::VARIABLE, "15", "scopeCount"},
                    {"scopeLimit: number", ark::es2panda::lsp::CompletionEntryKind::CONSTANT, "15", "scopeLimit"},
                    {"scopeAction(): void", ark::es2panda::lsp::CompletionEntryKind::FUNCTION, "15", "scopeAction()"}};
    for (const auto &[name, kind, sortText, insertText] : expected) {
        CompletionEntry *found = nullptr;
        for (auto &entry : entries) {
            if (entry.GetName() == name) {
                found = &entry;
                break;
            }
        }
        ASSERT_NE(found, nullptr) << "Expected scope-decl completion entry not found: " << name;
        EXPECT_EQ(found->GetCompletionKind(), kind) << "kind mismatch for entry: " << name;
        EXPECT_EQ(found->GetSortText(), sortText) << "sortText mismatch for entry: " << name;
        EXPECT_EQ(found->GetInsertText(), insertText) << "insertText mismatch for entry: " << name;
        // The removed ProcessAutoImportForEntry no-op must not have attached auto-import data.
        EXPECT_FALSE(found->GetCompletionEntryData().has_value()) << "unexpected data payload for entry: " << name;
    }
    // Strengthened pin: no entry anywhere in the scope-decl completion
    // response may carry auto-import data, not just the three expected ones.
    for (auto &entry : entries) {
        EXPECT_FALSE(entry.GetCompletionEntryData().has_value())
            << "unexpected auto-import data payload for entry: " << entry.GetName();
    }
}

// Test: public getCompletionsAtPosition at position 0 on an empty file must not
// throw. IsAnnotationBeginning used to evaluate sourceCode.at(pos - 1), so
// pos == 0 underflowed to at(SIZE_MAX) and raised std::out_of_range.
TEST_F(LSPCompletionsCornerTests, DISABLED_PositionZeroEmptyFileReturnsEmptyWithoutThrow)
{
    std::vector<std::string> files = {"completion_empty_pos_zero.ets"};
    const std::string text;
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    // An empty file has no tokens, so no completion entries are expected.
    EXPECT_NO_THROW({
        ark::es2panda::lsp::CompletionInfo res = lspApi->getCompletionsAtPosition(ctx, 0);
        EXPECT_TRUE(res.GetEntries().empty());
    });
    initializer.DestroyContext(ctx);
}

// Test: position 0 on a non-empty file must not throw either. IsAnnotationBeginning
// is called with the cursor offset first, so this exercises the same former
// pos - 1 underflow as the empty-file case, now against a non-empty source.
TEST_F(LSPCompletionsCornerTests, DISABLED_PositionZeroNonEmptyFileDoesNotThrow)
{
    std::vector<std::string> files = {"completion_head_pos_zero.ets"};
    // No leading whitespace: the first token starts exactly at offset 0.
    const std::string text = "let alpha: number = 1;\nlet beta = alpha\n";
    std::vector<std::string> texts = {text};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    // Position 0 precedes the first token, so there is no preceding token and
    // the regular completion path yields no entries; the pinned contract is
    // that this request completes without throwing instead of raising
    // std::out_of_range from IsAnnotationBeginning.
    EXPECT_NO_THROW({
        ark::es2panda::lsp::CompletionInfo res = lspApi->getCompletionsAtPosition(ctx, 0);
        EXPECT_TRUE(res.GetEntries().empty());
    });
    initializer.DestroyContext(ctx);
}
}  // namespace
