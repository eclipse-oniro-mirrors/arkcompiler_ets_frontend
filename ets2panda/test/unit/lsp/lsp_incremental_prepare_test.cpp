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

#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "util/diagnosticEngine.h"

#include <algorithm>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <vector>

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::util::DiagnosticType;

class LspIncrementalPrepareTest : public LSPAPITests {};

constexpr int INCREMENTAL_PREPARE_REPARSE = 0;
constexpr int INCREMENTAL_PREPARE_REUSE_AST = 1;
constexpr int LSP_API_SUCCESS = 0;
constexpr size_t SOURCE_FILE_INDEX = 0;
constexpr size_t UNRELATED_FILE_INDEX = 1;
constexpr size_t CONSUMER_FILE_INDEX = 2;
constexpr bool FILE_CHANGED = true;
constexpr bool FILE_UNCHANGED = false;

bool HasDiagnosticContaining(const DiagnosticReferences &diagnostics, std::string_view text)
{
    return std::any_of(diagnostics.diagnostic.begin(), diagnostics.diagnostic.end(),
                       [text](const auto &diagnostic) { return diagnostic.message_.find(text) != std::string::npos; });
}

void WriteFile(const std::string &fileName, const std::string &text)
{
    std::ofstream out {fileName, std::ios::trunc};
    ASSERT_TRUE(out.is_open());
    out << text;
}

bool HasDiagnosticsOfType(es2panda_Context *ctx, DiagnosticType type)
{
    auto *rawCtx = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    return !rawCtx->diagnosticEngine->GetDiagnosticStorage(type).empty();
}

void PrepareAndCheck(const es2panda_Impl *publicApi, es2panda_Context *&ctx, const std::string &fileName,
                     const std::string &text, bool isChanged)
{
    ASSERT_EQ(publicApi->IncrementalPrepareProgram(ctx, fileName.c_str(), text.c_str(), isChanged),
              INCREMENTAL_PREPARE_REPARSE);
    ctx = publicApi->ProceedToState(ctx, ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
}

void WritePrepareAndCheck(const es2panda_Impl *publicApi, es2panda_Context *&ctx, const std::string &fileName,
                          const std::string &text)
{
    ASSERT_NO_FATAL_FAILURE(WriteFile(fileName, text));
    ASSERT_NO_FATAL_FAILURE(PrepareAndCheck(publicApi, ctx, fileName, text, FILE_CHANGED));
}

TEST_F(LspIncrementalPrepareTest, IncrementalPrepareInvalidatesChangedExportWithoutClearingUnrelatedProgram)
{
    std::vector<std::string> files = {
        "source.ets",
        "unrelated.ets",
        "consumer.ets",
    };
    std::vector<std::string> texts = {
        R"ETS(export class Source {})ETS",
        R"ETS(export class Unrelated {})ETS",
        R"ETS(
            import { Source } from "./source.ets";
            import { Unrelated } from "./unrelated.ets";
            let value: Source | null = null;
            let unrelated: Unrelated | null = null;
            console.log(value, unrelated);
        )ETS",
    };
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    const auto &sourceFile = filePaths[SOURCE_FILE_INDEX];
    const auto &unrelatedFile = filePaths[UNRELATED_FILE_INDEX];
    const auto &consumerFile = filePaths[CONSUMER_FILE_INDEX];
    const auto &unrelatedText = texts[UNRELATED_FILE_INDEX];
    const auto &consumerText = texts[CONSUMER_FILE_INDEX];

    auto *publicApi = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    auto *lspApi = GetImpl();
    Initializer initializer;
    auto *ctx = initializer.CreateContext(consumerFile.c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    ASSERT_EQ(publicApi->ContextState(ctx), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(lspApi->getSemanticDiagnostics(ctx).diagnostic.size(), 0U);

    const std::string changedSource = R"ETS(export class RenamedSource {})ETS";
    ASSERT_NO_FATAL_FAILURE(WriteFile(sourceFile, changedSource));

    ASSERT_EQ(publicApi->IncrementalPrepareProgram(ctx, sourceFile.c_str(), changedSource.c_str(), FILE_CHANGED),
              INCREMENTAL_PREPARE_REPARSE);

    ASSERT_EQ(lspApi->DeleteDependantProgramsForFiles(ctx, sourceFile.c_str()), LSP_API_SUCCESS);
    ASSERT_EQ(publicApi->IncrementalPrepareProgram(ctx, unrelatedFile.c_str(), unrelatedText.c_str(), FILE_UNCHANGED),
              INCREMENTAL_PREPARE_REUSE_AST);
    ASSERT_EQ(publicApi->IncrementalPrepareProgram(ctx, consumerFile.c_str(), consumerText.c_str(), FILE_UNCHANGED),
              INCREMENTAL_PREPARE_REPARSE);
    initializer.DestroyContext(ctx);

    auto *verifyCtx = initializer.CreateContext(consumerFile.c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(verifyCtx, nullptr);
    const auto diagnostics = lspApi->getSemanticDiagnostics(verifyCtx);
    EXPECT_GT(diagnostics.diagnostic.size(), 0U);
    EXPECT_TRUE(HasDiagnosticContaining(diagnostics, "Source"));
    EXPECT_FALSE(HasDiagnosticContaining(diagnostics, "Unrelated"));

    initializer.DestroyContext(verifyCtx);
}

TEST_F(LspIncrementalPrepareTest, IncrementalPrepareChangedFileCanProceedToChecked)
{
    std::vector<std::string> files = {"changed.ets"};
    std::vector<std::string> texts = {
        R"ETS(
            export function value(): int {
                return 1;
            }
        )ETS",
    };
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    const auto &changedFile = filePaths[SOURCE_FILE_INDEX];
    const auto &initialText = texts[SOURCE_FILE_INDEX];

    auto *publicApi = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    auto *lspApi = GetImpl();
    ark::es2panda::EHeap::Scope eheapScope;
    Initializer initializer;
    std::vector<std::string> activeTexts {initialText};
    auto *ctx = initializer.CreateContextWithCache(changedFile.c_str(), ES2PANDA_STATE_CHECKED, activeTexts);
    ASSERT_NE(ctx, nullptr);
    ASSERT_EQ(publicApi->ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const std::string changedText = R"ETS(
        export function value(): int {
            return 2;
        }
    )ETS";
    ASSERT_NO_FATAL_FAILURE(WriteFile(changedFile, changedText));

    ASSERT_EQ(publicApi->IncrementalPrepareProgram(ctx, changedFile.c_str(), changedText.c_str(), FILE_CHANGED),
              INCREMENTAL_PREPARE_REPARSE);
    ctx = publicApi->ProceedToState(ctx, ES2PANDA_STATE_CHECKED);
    const auto diagnostics = lspApi->getSemanticDiagnostics(ctx);
    ASSERT_EQ(publicApi->ContextState(ctx), ES2PANDA_STATE_CHECKED)
        << publicApi->ContextErrorMessage(ctx) << " diagnostic count=" << diagnostics.diagnostic.size();

    initializer.DestroyContext(ctx);
}

TEST_F(LspIncrementalPrepareTest, IncrementalPrepareReparsesWhenSourceDiffersDespiteUnchangedFlag)
{
    std::vector<std::string> files = {
        "provider.ets",
        "consumer.ets",
    };
    std::vector<std::string> texts = {
        R"ETS(export function A(): string { return "ok"; })ETS",
        R"ETS(
            import { A } from "./provider.ets";
            let x: string = A();
        )ETS",
    };
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    const auto &providerFile = filePaths[0];
    const auto &consumerFile = filePaths[1];

    auto *publicApi = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    Initializer initializer;
    auto *ctx = initializer.CreateContext(consumerFile.c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    const std::string modifiedProvider = R"ETS(export function A(): number { return 1; })ETS";
    ASSERT_NO_FATAL_FAILURE(WriteFile(providerFile, modifiedProvider));

    // Even if caller passes FILE_UNCHANGED (e.g. after cache recycle + immediate save),
    // C++ must detect that targetProgram->SourceCode() differs from modifiedProvider and force reparse.
    ASSERT_EQ(publicApi->IncrementalPrepareProgram(ctx, providerFile.c_str(), modifiedProvider.c_str(), FILE_UNCHANGED),
              INCREMENTAL_PREPARE_REPARSE);

    initializer.DestroyContext(ctx);
}

TEST_F(LspIncrementalPrepareTest, IncrementalPrepareReparsesCrlfToLfAndAlignsDefinitionOffset)
{
    const std::string crlfText = "function targetFn(): int {\r\n    return 1;\r\n}\r\nlet res = targetFn();\r\n";
    const std::string lfText = "function targetFn(): int {\n    return 1;\n}\nlet res = targetFn();\n";
    auto filePaths = CreateTempFile({"crlf_def.ets"}, {crlfText});
    ASSERT_EQ(filePaths.size(), 1U);
    const auto &crlfFile = filePaths[0];

    auto *publicApi = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    auto *lspApi = GetImpl();
    ark::es2panda::EHeap::Scope eheapScope;
    Initializer initializer;
    std::vector<std::string> activeTexts {crlfText};
    auto *ctx = initializer.CreateContextWithCache(crlfFile.c_str(), ES2PANDA_STATE_CHECKED, activeTexts);
    ASSERT_NE(ctx, nullptr);
    ASSERT_EQ(publicApi->ContextState(ctx), ES2PANDA_STATE_CHECKED);

    ASSERT_EQ(publicApi->IncrementalPrepareProgram(ctx, crlfFile.c_str(), lfText.c_str(), FILE_UNCHANGED),
              INCREMENTAL_PREPARE_REPARSE);
    ctx = publicApi->ProceedToState(ctx, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(publicApi->ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const std::string targetName = "targetFn";
    const size_t declPos = lfText.find(targetName);
    const size_t callPos = lfText.rfind(targetName);
    ASSERT_NE(declPos, std::string::npos);
    ASSERT_NE(callPos, std::string::npos);
    ASSERT_NE(declPos, callPos);

    const auto def = lspApi->getDefinitionAtPosition(ctx, callPos);
    EXPECT_EQ(def.fileName, crlfFile);
    EXPECT_EQ(def.start, declPos);
    EXPECT_EQ(def.length, targetName.size());

    ASSERT_EQ(publicApi->IncrementalPrepareProgram(ctx, crlfFile.c_str(), lfText.c_str(), FILE_UNCHANGED),
              INCREMENTAL_PREPARE_REUSE_AST);

    initializer.DestroyContext(ctx);
}

TEST_F(LspIncrementalPrepareTest, IncrementalPrepareReimportingDeletedDependantAvoidsRuntimeNameClash)
{
    constexpr size_t leafFileIndex = 0;
    constexpr size_t entryFileIndex = 2;
    const std::vector<std::string> files = {"clash_leaf.ets", "clash_middle.ets", "clash_entry.ets"};
    const std::vector<std::string> texts = {
        "export function foo(): number { return 1; }\n",
        "import { foo } from './clash_leaf.ets';\n"
        "export function bar(): void { let x: number = foo(); console.log(x); }\n",
        "import { bar } from './clash_middle.ets';\nexport function main(): void { bar(); }\n"
        "export function missRet(flag: boolean): number { if (flag) { return 1; } }\n"};
    const auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    const auto &leafFile = filePaths[leafFileIndex];
    const auto &entryFile = filePaths[entryFileIndex];
    const auto &entryText = texts[entryFileIndex];

    auto *publicApi = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    auto *lspApi = GetImpl();
    ASSERT_NE(publicApi, nullptr);
    ASSERT_NE(lspApi, nullptr);

    ark::es2panda::EHeap::Scope eheapScope;
    Initializer initializer;
    std::vector<std::string> activeTexts {entryText};
    auto *ctx = initializer.CreateContextWithCache(entryFile.c_str(), ES2PANDA_STATE_CHECKED, activeTexts);
    ASSERT_NE(ctx, nullptr);
    EXPECT_FALSE(HasDiagnosticsOfType(ctx, DiagnosticType::FATAL));
    EXPECT_TRUE(HasDiagnosticContaining(lspApi->getSemanticDiagnostics(ctx), "Not all code paths return a value"));

    const std::string leafUpdatedText = "export function foo(): string { return 'hello'; }\n";
    ASSERT_NO_FATAL_FAILURE(WritePrepareAndCheck(publicApi, ctx, leafFile, leafUpdatedText));

    // Deleting leaf's dependants drops middle and entry; re-preparing entry must re-import middle without clashing.
    ASSERT_EQ(lspApi->DeleteDependantProgramsForFiles(ctx, leafFile.c_str()), LSP_API_SUCCESS);
    ASSERT_NO_FATAL_FAILURE(PrepareAndCheck(publicApi, ctx, entryFile, entryText, FILE_UNCHANGED));
    EXPECT_FALSE(HasDiagnosticsOfType(ctx, DiagnosticType::FATAL));
    EXPECT_TRUE(HasDiagnosticContaining(lspApi->getSemanticDiagnostics(ctx), "Not all code paths return a value"));
    initializer.DestroyContext(ctx);
}

TEST_F(LspIncrementalPrepareTest, IncrementalPrepareCircularDependencyPreservesReverseInvalidation)
{
    constexpr size_t xFileIndex = 0;
    constexpr size_t yFileIndex = 1;
    const std::vector<std::string> files = {"cycle_x.ets", "cycle_y.ets"};
    const std::vector<std::string> texts = {
        "import { g } from './cycle_y.ets';\nexport function f(): number { return 1; }\n"
        "export function useG(): void { let v: number = g(); console.log(v); }\n",
        "import { f } from './cycle_x.ets';\nexport function g(): number { return 1; }\n"
        "export function useF(): void { let v: number = f(); console.log(v); }\n"};
    const auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    const auto &xFile = filePaths[xFileIndex];
    const auto &yFile = filePaths[yFileIndex];
    const auto &yInitialText = texts[yFileIndex];

    auto *publicApi = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    auto *lspApi = GetImpl();
    ASSERT_NE(publicApi, nullptr);
    ASSERT_NE(lspApi, nullptr);

    ark::es2panda::EHeap::Scope eheapScope;
    Initializer initializer;
    std::vector<std::string> activeTexts {texts[xFileIndex]};
    auto *ctx = initializer.CreateContextWithCache(xFile.c_str(), ES2PANDA_STATE_CHECKED, activeTexts);
    ASSERT_NE(ctx, nullptr);
    EXPECT_FALSE(HasDiagnosticsOfType(ctx, DiagnosticType::FATAL));
    EXPECT_FALSE(HasDiagnosticsOfType(ctx, DiagnosticType::SEMANTIC));

    // Round 1: change X and evict Y while keeping X's outgoing edge to Y in the dependency graph.
    const std::string xUpdatedText =
        "import { g } from './cycle_y.ets';\nexport function f(): string { return 'a'; }\n"
        "export function useG(): void { let v: number = g(); console.log(v); }\n";
    ASSERT_NO_FATAL_FAILURE(WritePrepareAndCheck(publicApi, ctx, xFile, xUpdatedText));
    ASSERT_EQ(lspApi->DeleteDependantProgramsForFiles(ctx, xFile.c_str()), LSP_API_SUCCESS);
    ASSERT_EQ(publicApi->IncrementalPrepareProgram(ctx, xFile.c_str(), xUpdatedText.c_str(), FILE_UNCHANGED),
              INCREMENTAL_PREPARE_REUSE_AST);
    ASSERT_NO_FATAL_FAILURE(PrepareAndCheck(publicApi, ctx, yFile, yInitialText, FILE_UNCHANGED));
    EXPECT_TRUE(HasDiagnosticContaining(lspApi->getSemanticDiagnostics(ctx),
                                        "Type 'String' cannot be assigned to type 'Double'"));

    // Round 2: fix f() usage in Y, change g(), evict X, and verify X reparses and reports g() mismatch.
    const std::string yUpdatedText =
        "import { f } from './cycle_x.ets';\nexport function g(): string { return 'b'; }\n"
        "export function useF(): void { let v: string = f(); console.log(v); }\n";
    ASSERT_NO_FATAL_FAILURE(WritePrepareAndCheck(publicApi, ctx, yFile, yUpdatedText));
    EXPECT_FALSE(HasDiagnosticsOfType(ctx, DiagnosticType::FATAL));
    EXPECT_FALSE(HasDiagnosticsOfType(ctx, DiagnosticType::SEMANTIC));
    ASSERT_EQ(lspApi->DeleteDependantProgramsForFiles(ctx, yFile.c_str()), LSP_API_SUCCESS);
    ASSERT_NO_FATAL_FAILURE(PrepareAndCheck(publicApi, ctx, xFile, xUpdatedText, FILE_UNCHANGED));
    EXPECT_FALSE(HasDiagnosticsOfType(ctx, DiagnosticType::FATAL));
    EXPECT_TRUE(HasDiagnosticContaining(lspApi->getSemanticDiagnostics(ctx),
                                        "Type 'String' cannot be assigned to type 'Double'"));
    initializer.DestroyContext(ctx);
}

TEST_F(LspIncrementalPrepareTest, IncrementalPrepareThreeFileCyclePreservesTransitiveInvalidation)
{
    constexpr size_t xFileIndex = 0;
    constexpr size_t zFileIndex = 2;
    const std::vector<std::string> files = {"cycle3_x.ets", "cycle3_y.ets", "cycle3_z.ets"};
    const std::vector<std::string> texts = {
        "import { mid } from './cycle3_y.ets';\nexport function root(): number { return 1; }\n"
        "export function useMid(): void { let m: number = mid(); console.log(m); }\n",
        "import { leaf } from './cycle3_z.ets';\nexport function mid(): number { return leaf(); }\n",
        "import { root } from './cycle3_x.ets';\nexport function leaf(): number { return 1; }\n"
        "export function useRoot(): void { let r: number = root(); console.log(r); }\n"};
    const auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    const auto &xFile = filePaths[xFileIndex];
    const auto &zFile = filePaths[zFileIndex];

    auto *publicApi = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    auto *lspApi = GetImpl();
    ASSERT_NE(publicApi, nullptr);
    ASSERT_NE(lspApi, nullptr);

    ark::es2panda::EHeap::Scope eheapScope;
    Initializer initializer;
    std::vector<std::string> activeTexts {texts[xFileIndex]};
    auto *ctx = initializer.CreateContextWithCache(xFile.c_str(), ES2PANDA_STATE_CHECKED, activeTexts);
    ASSERT_NE(ctx, nullptr);

    // Round 1: edit X and evict Y and Z without reopening Y.
    const std::string xUpdatedText =
        "import { mid } from './cycle3_y.ets';\nexport function root(): string { return 'x'; }\n"
        "export function useMid(): void { let m: number = mid(); console.log(m); }\n";
    ASSERT_NO_FATAL_FAILURE(WritePrepareAndCheck(publicApi, ctx, xFile, xUpdatedText));
    ASSERT_EQ(lspApi->DeleteDependantProgramsForFiles(ctx, xFile.c_str()), LSP_API_SUCCESS);

    // Round 2: edit Z directly; transitive edge Z <- Y <- X must survive so X gets invalidated and reparses.
    const std::string zUpdatedText =
        "import { root } from './cycle3_x.ets';\nexport function leaf(): string { return 'z'; }\n"
        "export function useRoot(): void { let r: string = root(); console.log(r); }\n";
    ASSERT_NO_FATAL_FAILURE(WritePrepareAndCheck(publicApi, ctx, zFile, zUpdatedText));
    ASSERT_EQ(lspApi->DeleteDependantProgramsForFiles(ctx, zFile.c_str()), LSP_API_SUCCESS);
    ASSERT_NO_FATAL_FAILURE(PrepareAndCheck(publicApi, ctx, xFile, xUpdatedText, FILE_UNCHANGED));
    EXPECT_FALSE(HasDiagnosticsOfType(ctx, DiagnosticType::FATAL));
    initializer.DestroyContext(ctx);
}

TEST_F(LspIncrementalPrepareTest, IncrementalPrepareReparsesLazyCheckedExternalDependant)
{
    constexpr size_t leafFileIndex = 0;
    constexpr size_t middleFileIndex = 1;
    constexpr size_t entryFileIndex = 2;
    const std::vector<std::string> files = {"lazy_leaf.ets", "lazy_middle.ets", "lazy_entry.ets"};
    const std::vector<std::string> texts = {
        "export class Leaf { foo(): number { return 1; } }\n",
        "import { Leaf } from './lazy_leaf.ets';\n"
        "export class Middle { bar(): void { let x: number = new Leaf().foo(); console.log(x); } }\n",
        "import { Middle } from './lazy_middle.ets';\nexport function main(): void { new Middle().bar(); }\n"};
    const auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    const auto &leafFile = filePaths[leafFileIndex];
    const auto &middleFile = filePaths[middleFileIndex];
    const auto &entryFile = filePaths[entryFileIndex];

    auto *publicApi = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    auto *lspApi = GetImpl();
    ASSERT_NE(publicApi, nullptr);
    ASSERT_NE(lspApi, nullptr);

    ark::es2panda::EHeap::Scope eheapScope;
    Initializer initializer;
    std::vector<std::string> activeTexts {texts[entryFileIndex]};
    auto *ctx = initializer.CreateContextWithCache(entryFile.c_str(), ES2PANDA_STATE_CHECKED, activeTexts);
    ASSERT_NE(ctx, nullptr);

    const std::string leafUpdatedText = "export class Leaf { foo(): string { return 'hello'; } }\n";
    ASSERT_NO_FATAL_FAILURE(WritePrepareAndCheck(publicApi, ctx, leafFile, leafUpdatedText));
    ASSERT_EQ(lspApi->DeleteDependantProgramsForFiles(ctx, leafFile.c_str()), LSP_API_SUCCESS);

    // Recompiling entry first re-imports middle only as a lazy-checked external program.
    ASSERT_NO_FATAL_FAILURE(PrepareAndCheck(publicApi, ctx, entryFile, texts[entryFileIndex], FILE_UNCHANGED));
    // Querying middle afterwards must reparse it because its AST never completed full analysis.
    ASSERT_NO_FATAL_FAILURE(PrepareAndCheck(publicApi, ctx, middleFile, texts[middleFileIndex], FILE_UNCHANGED));
    EXPECT_FALSE(HasDiagnosticsOfType(ctx, DiagnosticType::FATAL));
    EXPECT_TRUE(HasDiagnosticContaining(lspApi->getSemanticDiagnostics(ctx),
                                        "Type 'String' cannot be assigned to type 'Double'"));
    ASSERT_EQ(
        publicApi->IncrementalPrepareProgram(ctx, middleFile.c_str(), texts[middleFileIndex].c_str(), FILE_UNCHANGED),
        INCREMENTAL_PREPARE_REUSE_AST);
    initializer.DestroyContext(ctx);
}

}  // namespace
