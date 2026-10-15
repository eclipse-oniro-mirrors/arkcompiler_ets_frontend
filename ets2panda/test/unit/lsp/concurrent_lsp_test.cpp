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

#include "lsp/include/api.h"
#include "lsp/include/find_rename_locations.h"
#include "lsp/include/internal_api.h"
#include "lsp_api_test.h"
#include <gtest/gtest.h>
#include <future>
#include <thread>
#include <vector>

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::RenameLocation;

namespace {

class LspConcurrentTests : public LSPAPITests {};

// Helper: projection of Diagnostic references for comparison
static std::vector<std::string> DiagnosticMessageProjection(const DiagnosticReferences &refs)
{
    std::vector<std::string> messages;
    for (const auto &diag : refs.diagnostic) {
        messages.push_back(diag.message_);
    }
    std::sort(messages.begin(), messages.end());
    return messages;
}

// Helper: projection of RenameLocation for comparison
static std::vector<std::string> RenameLocationProjection(const std::vector<RenameLocation> &locs)
{
    std::vector<std::string> proj;
    for (const auto &loc : locs) {
        proj.push_back(loc.fileName + ":" + std::to_string(loc.start) + "-" + std::to_string(loc.end));
    }
    std::sort(proj.begin(), proj.end());
    return proj;
}

TEST_F(LspConcurrentTests, ConcurrentDiagnostics_NoCrash)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string sourceWithError = R"(
let a: number = "string_error";
function foo(x: number): void {}
foo("type_error");
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string sourceClean = R"(
let b: number = 42;
function bar(x: number): void { bar(x); }
)";

    const std::vector<std::string> files = {"concurrent_diag1.ets", "concurrent_diag2.ets"};
    const std::vector<std::string> sources = {sourceWithError, sourceClean};
    auto filePaths = CreateTempFile(files, sources);
    ASSERT_EQ(filePaths.size(), files.size());

    // Create independent checked contexts serially on the main thread before launching workers
    Initializer init1;
    Initializer init2;
    es2panda_Context *ctx1 = init1.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    es2panda_Context *ctx2 = init2.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    auto thread1 = std::async(std::launch::async, [ctx1]() {
        LSPAPI const *lspApi = GetImpl();
        DiagnosticReferences result = lspApi->getSemanticDiagnostics(ctx1);
        return DiagnosticMessageProjection(result);
    });

    auto thread2 = std::async(std::launch::async, [ctx2]() {
        LSPAPI const *lspApi = GetImpl();
        DiagnosticReferences result = lspApi->getSemanticDiagnostics(ctx2);
        return DiagnosticMessageProjection(result);
    });

    auto diag1 = thread1.get();
    auto diag2 = thread2.get();

    // Destroy all contexts on the main thread after the futures join
    init1.DestroyContext(ctx1);
    init2.DestroyContext(ctx2);

    // Thread 1 should have error messages related to type mismatch
    ASSERT_FALSE(diag1.empty());
    // Thread 2 has no errors (only possible unused warnings)
    // At minimum, thread 2 should have fewer diagnostics than thread 1
    ASSERT_GE(diag1.size(), diag2.size());
}

// One rename worker's context set: a trigger context plus one context per file.
// Ownership stays with the caller's independent Initializer; creation and
// destruction happen serially on the main thread.
struct RenameWorkerSet {
    es2panda_Context *triggerCtx = nullptr;
    std::vector<es2panda_Context *> fileContexts;
};

static RenameWorkerSet CreateRenameWorkerSet(Initializer &init, const std::vector<std::string> &filePaths,
                                             size_t triggerIndex)
{
    RenameWorkerSet set;
    set.triggerCtx = init.CreateContext(filePaths[triggerIndex].c_str(), ES2PANDA_STATE_CHECKED);
    for (const auto &fp : filePaths) {
        set.fileContexts.push_back(init.CreateContext(fp.c_str(), ES2PANDA_STATE_CHECKED));
    }
    return set;
}

static void DestroyRenameWorkerSet(Initializer &init, const RenameWorkerSet &set)
{
    for (auto *fc : set.fileContexts) {
        init.DestroyContext(fc);
    }
    init.DestroyContext(set.triggerCtx);
}

// Worker preparation: launch an LSP-only rename worker for one context set.
static std::future<std::vector<std::string>> LaunchRenameWorker(es2panda_Context *triggerCtx,
                                                                std::vector<es2panda_Context *> &fileContexts,
                                                                size_t pos)
{
    return std::async(std::launch::async, [triggerCtx, &fileContexts, pos]() {
        LSPAPI const *lspApi = GetImpl();
        auto result = lspApi->findRenameLocations(fileContexts, triggerCtx, pos);
        return RenameLocationProjection(result);
    });
}

// Cross-file import topology + multi-file findRenameLocations overload with exact result-set
// equality; extreme_combination_test.cpp covers the same-file overload by size only.
TEST_F(LspConcurrentTests, ConcurrentRename_NoCrash)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string sourceA = R"(
export function foo(): void { foo(); }
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string sourceB = R"(
import { foo } from "./concurrent_rename1.ets";
foo();
)";

    const std::vector<std::string> files = {"concurrent_rename1.ets", "concurrent_rename2.ets"};
    const std::vector<std::string> sources = {sourceA, sourceB};
    auto filePaths = CreateTempFile(files, sources);
    ASSERT_EQ(filePaths.size(), files.size());

    // Two independent Initializers, one per worker. All contexts are created and
    // destroyed serially on the main thread; workers only call the LSP API.
    Initializer initA;
    Initializer initB;
    auto setA = CreateRenameWorkerSet(initA, filePaths, 0);
    auto setB = CreateRenameWorkerSet(initB, filePaths, 1);

    const size_t fooPosA = sourceA.find("foo");
    ASSERT_NE(fooPosA, std::string::npos);
    auto thread1 = LaunchRenameWorker(setA.triggerCtx, setA.fileContexts, fooPosA + 1);

    const size_t fooPosB = sourceB.find("foo");
    ASSERT_NE(fooPosB, std::string::npos);
    auto thread2 = LaunchRenameWorker(setB.triggerCtx, setB.fileContexts, fooPosB + 1);

    auto rename1 = thread1.get();
    auto rename2 = thread2.get();

    // Destroy all contexts on the main thread after the futures join
    DestroyRenameWorkerSet(initA, setA);
    DestroyRenameWorkerSet(initB, setB);

    ASSERT_FALSE(rename1.empty());
    ASSERT_FALSE(rename2.empty());
    // Both workers operate on the same function name "foo" in the same two files
    ASSERT_EQ(rename1, rename2);
}

TEST_F(LspConcurrentTests, ConcurrentGetDefinition_NoCrash)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string sourceA = R"(
export function foo(): void {}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string sourceB = R"(
import { foo } from "./concurrent_def1.ets";
let x = foo;
)";

    const std::vector<std::string> files = {"concurrent_def1.ets", "concurrent_def2.ets"};
    const std::vector<std::string> sources = {sourceA, sourceB};
    auto filePaths = CreateTempFile(files, sources);
    ASSERT_EQ(filePaths.size(), files.size());

    // Create independent checked contexts serially on the main thread before launching workers
    Initializer init1;
    Initializer init2;
    es2panda_Context *ctx1 = init1.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    es2panda_Context *ctx2 = init2.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t defFooPos1 = sourceB.find("foo");
    ASSERT_NE(defFooPos1, std::string::npos);
    auto thread1 = std::async(std::launch::async, [ctx1, defFooPos1]() {
        LSPAPI const *lspApi = GetImpl();
        size_t pos = defFooPos1 + 1;  // position on "foo" reference
        DefinitionInfo result = lspApi->getDefinitionAtPosition(ctx1, pos);
        return result;
    });

    const size_t defFooPos2 = sourceB.find("foo");
    ASSERT_NE(defFooPos2, std::string::npos);
    auto thread2 = std::async(std::launch::async, [ctx2, defFooPos2]() {
        LSPAPI const *lspApi = GetImpl();
        size_t pos = defFooPos2 + 1;
        DefinitionInfo result = lspApi->getDefinitionAtPosition(ctx2, pos);
        return result;
    });

    DefinitionInfo def1 = thread1.get();
    DefinitionInfo def2 = thread2.get();

    // Destroy all contexts on the main thread after the futures join
    init1.DestroyContext(ctx1);
    init2.DestroyContext(ctx2);

    // Both independent contexts for the same file should produce the same definition
    ASSERT_EQ(def1.fileName, def2.fileName);
    ASSERT_EQ(def1.start, def2.start);
    ASSERT_EQ(def1.length, def2.length);
    // Verify that the definition points to a real location (non-empty fileName)
    ASSERT_FALSE(def1.fileName.empty());
}

}  // namespace
