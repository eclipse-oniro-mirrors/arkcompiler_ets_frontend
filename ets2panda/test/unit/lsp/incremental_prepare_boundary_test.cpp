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

#include <cstddef>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::lsp::Initializer;

class LspIncrementalPrepareBoundaryTests : public LSPAPITests {};

void CollectRawChange(std::vector<FileTextChanges> &result, const ark::es2panda::SourceFile *sourceFile,
                      const std::string &fileName, const std::string &source, const std::string &newText)
{
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext context {{}, formatContext, {}};
    auto tracker = ark::es2panda::lsp::ChangeTracker::FromContext(context);
    TextSpan span = {0, source.size()};
    TextChange change = {span, newText};
    FileTextChanges fileChange = {fileName, {change}};
    tracker.PushRaw(sourceFile, fileChange);
    result = tracker.GetChanges();
}

// P1E-IPB-01: MultiFileChanges
// Verifies ChangeTracker::With across two real source files yields per-file
// FileTextChanges. Two contexts (fileA, fileB), one TextChangesContext, PushRaw
// per file, GetChanges groups by file path.
TEST_F(LspIncrementalPrepareBoundaryTests, MultiFileChanges)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string fileA = "multiFileA.ets";
    const std::string fileB = "multiFileB.ets";
    const std::string sourceA = "function a() { return 1; }";
    const std::string sourceB = "function b() { return 2; }";

    es2panda_Context *ctxA = initializer.CreateContext(fileA.c_str(), ES2PANDA_STATE_CHECKED, sourceA.c_str());
    ASSERT_NE(ctxA, nullptr);
    es2panda_Context *ctxB = initializer.CreateContext(fileB.c_str(), ES2PANDA_STATE_CHECKED, sourceB.c_str());
    ASSERT_NE(ctxB, nullptr);

    auto *contextA = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctxA);
    auto *contextB = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctxB);
    const auto *sourceFileA = contextA->sourceFile;
    const auto *sourceFileB = contextB->sourceFile;

    // Build TextChangesContext and use ChangeTracker::With
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};

    TextSpan spanA = {0, sourceA.size()};
    TextChange changeA = {spanA, "function a() { return 10; }"};
    FileTextChanges fileChangeA = {fileA, {changeA}};

    TextSpan spanB = {0, sourceB.size()};
    TextChange changeB = {spanB, "function b() { return 20; }"};
    FileTextChanges fileChangeB = {fileB, {changeB}};

    auto result = ark::es2panda::lsp::ChangeTracker::With(changeText, [&](ark::es2panda::lsp::ChangeTracker &tracker) {
        tracker.PushRaw(sourceFileA, fileChangeA);
        tracker.PushRaw(sourceFileB, fileChangeB);
    });

    // GetChanges groups by sourceFile->filePath; should be 2 entries
    ASSERT_EQ(result.size(), 2U);

    // Verify each file has one TextChange
    bool foundA = false;
    bool foundB = false;
    for (const auto &fc : result) {
        ASSERT_EQ(fc.textChanges.size(), 1U);
        if (fc.fileName == std::string(sourceFileA->filePath)) {
            foundA = true;
            EXPECT_EQ(fc.textChanges[0].newText, "function a() { return 10; }");
        } else if (fc.fileName == std::string(sourceFileB->filePath)) {
            foundB = true;
            EXPECT_EQ(fc.textChanges[0].newText, "function b() { return 20; }");
        }
    }
    EXPECT_TRUE(foundA);
    EXPECT_TRUE(foundB);

    initializer.DestroyContext(ctxA);
    initializer.DestroyContext(ctxB);
}

// P1E-IPB-02: ConcurrentIndependentTrackers
// Verifies two threads with separate contexts/trackers produce independent
// deterministic changes. Each thread builds its own TextChangesContext and
// ChangeTracker, pushes a raw change, and collects GetChanges.
// No shared state between threads.
TEST_F(LspIncrementalPrepareBoundaryTests, ConcurrentIndependentTrackers)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string fileA = "concurrentA.ets";
    const std::string fileB = "concurrentB.ets";
    const std::string sourceA = "let x = 1;";
    const std::string sourceB = "let y = 2;";

    es2panda_Context *ctxA = initializer.CreateContext(fileA.c_str(), ES2PANDA_STATE_CHECKED, sourceA.c_str());
    ASSERT_NE(ctxA, nullptr);
    es2panda_Context *ctxB = initializer.CreateContext(fileB.c_str(), ES2PANDA_STATE_CHECKED, sourceB.c_str());
    ASSERT_NE(ctxB, nullptr);

    auto *contextA = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctxA);
    auto *contextB = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctxB);
    const auto *sourceFileA = contextA->sourceFile;
    const auto *sourceFileB = contextB->sourceFile;

    std::vector<FileTextChanges> resultA;
    std::vector<FileTextChanges> resultB;

    std::thread t1([&resultA, sourceFileA, &fileA, &sourceA]() {
        CollectRawChange(resultA, sourceFileA, fileA, sourceA, "let x = 100;");
    });

    std::thread t2([&resultB, sourceFileB, &fileB, &sourceB]() {
        CollectRawChange(resultB, sourceFileB, fileB, sourceB, "let y = 200;");
    });

    t1.join();
    t2.join();

    // Each thread's result is independent
    ASSERT_EQ(resultA.size(), 1U);
    ASSERT_EQ(resultB.size(), 1U);

    EXPECT_EQ(resultA[0].textChanges.size(), 1U);
    EXPECT_EQ(resultA[0].textChanges[0].newText, "let x = 100;");
    EXPECT_EQ(std::string(resultA[0].fileName), std::string(sourceFileA->filePath));

    EXPECT_EQ(resultB[0].textChanges.size(), 1U);
    EXPECT_EQ(resultB[0].textChanges[0].newText, "let y = 200;");
    EXPECT_EQ(std::string(resultB[0].fileName), std::string(sourceFileB->filePath));

    initializer.DestroyContext(ctxA);
    initializer.DestroyContext(ctxB);
}

// P1E-IPB-03: ChangedExportDeletion
// Verifies that replacing an export declaration node yields a FileTextChanges
// whose text contains the expected export fragment. Uses ReplaceNodeWithText
// on an export class declaration node inside ChangeTracker::With.
TEST_F(LspIncrementalPrepareBoundaryTests, ChangedExportDeletion)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string fileName = "changedExportDeletion.ets";
    const std::string source = "export class A {}";
    es2panda_Context *ctx = initializer.CreateContext(fileName.c_str(), ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto *sourceFile = context->sourceFile;
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the class declaration (export class A {})
    auto *classNode = ast->FindChild([](ark::es2panda::ir::AstNode *node) { return node->IsClassDeclaration(); });
    ASSERT_NE(classNode, nullptr);

    // Use ChangeTracker::With to replace the export class node
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};

    const std::string replacementText = "export class B {}";
    auto result = ark::es2panda::lsp::ChangeTracker::With(changeText, [&](ark::es2panda::lsp::ChangeTracker &tracker) {
        tracker.ReplaceNodeWithText(ctx, classNode, replacementText);
    });

    // GetChanges should produce one FileTextChanges
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0].textChanges.size(), 1U);
    // The newText should contain the export fragment
    EXPECT_NE(result[0].textChanges[0].newText.find("export class B"), std::string::npos);
    // The file name should match the context's source file path
    EXPECT_EQ(result[0].fileName, std::string(sourceFile->filePath));

    initializer.DestroyContext(ctx);
}

// P1E-IPB-04: NoChangeEmptyResult
// Verifies that ChangeTracker::With with a callback that records no changes
// returns an empty vector.
TEST_F(LspIncrementalPrepareBoundaryTests, NoChangeEmptyResult)
{
    ark::es2panda::lsp::Initializer initializer;
    const std::string fileName = "noChangeEmptyResult.ets";
    const std::string source = "class C {}";
    es2panda_Context *ctx = initializer.CreateContext(fileName.c_str(), ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};

    // Callback does nothing -> no changes recorded
    auto result =
        ark::es2panda::lsp::ChangeTracker::With(changeText, [](ark::es2panda::lsp::ChangeTracker & /*tracker*/) {});

    // Expect empty result
    EXPECT_EQ(result.size(), 0U);

    initializer.DestroyContext(ctx);
}

}  // namespace
