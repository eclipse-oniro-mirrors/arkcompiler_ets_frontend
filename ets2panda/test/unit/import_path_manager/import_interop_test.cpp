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

#include "import_path_manager_test.h"

namespace ark::es2panda::util::test {
namespace {

using InteropImportPathTest = ImportPathManagerTest;

struct RelativeInteropCase {
    const char *name;
    const char *request;
    const char *source;
};

class InteropSourcePathTest : public InteropImportPathTest, public testing::WithParamInterface<RelativeInteropCase> {};

TEST_P(InteropSourcePathTest, MapsTheOriginalSourceToItsLogicalDependencyBeforeUsingFallbacks)
{
    const auto &sample = GetParam();
    Write("types/Entry.d.ats");
    Write("types/Peer.ats");
    Write("types/Peer.ts");
    Write("Peer.ats");
    Write("types/External.d.ats", "export declare const selected: int;");
    Write("src/nested/Entry.ts");
    Write(sample.source);
    const std::string fields = R"("dependencies":{
        "app/entry":{"language":"js", "ohmUrl":"entry", "path":"types/Entry.d.ats",
                     "sourceFilePath":"src/nested/Entry.ts"},
        "app/Peer":{"language":"js", "ohmUrl":"fallback"},
        "app/Peer.ts":{"language":"js", "ohmUrl":"explicit-fallback"},
        "Peer":{"language":"js", "ohmUrl":"parent-fallback"},
        "external-peer":{"language":"js", "ohmUrl":"selected", "path":"types/External.d.ats",
                         "sourceFilePath":)" +
                               JsonPath(Path(sample.source)) + "}}";
    ASSERT_NO_FATAL_FAILURE(Start(fields));
    auto *entry = Import("app/entry");
    ASSERT_NE(entry, nullptr);
    auto *peer = Import(sample.request, entry);
    ASSERT_NE(peer, nullptr);
    EXPECT_TRUE(peer->Is<ModuleKind::SOURCE_DECL>());
    EXPECT_EQ(peer->GetImportInfo().ResolvedSource(), "external-peer");
    EXPECT_EQ(peer->GetImportInfo().OhmUrl(), "selected");
    EXPECT_EQ(peer->SourceCode(), "export declare const selected: int;");
    EXPECT_TRUE(Manager()->CollectReverseDependencies("app/Peer").empty());
    EXPECT_EQ(Import("external-peer"), peer);
}

const RelativeInteropCase SOURCES[] = {
    {"JavaScript", "./Peer", "src/nested/Peer.js"},
    {"TypeScriptDeclaration", "./Peer", "src/nested/Peer.d.ts"},
    {"ExplicitTypeScript", "./Peer.ts", "src/nested/Peer.ts"},
    {"DirectoryIndex", "./Peer", "src/nested/Peer/index.ts"},
    {"ParentDirectory", "../Peer", "src/Peer.ts"},
    {"BackslashSeparatedParent", R"(..\Peer)", "src/Peer.ts"},
};

INSTANTIATE_TEST_SUITE_P(OriginalSources, InteropSourcePathTest, testing::ValuesIn(SOURCES),
                         [](const auto &info) { return info.param.name; });

TEST_F(InteropImportPathTest, SameRelativeTextFromTwoPackagesPreservesBothDependencyIdentities)
{
    Write("generated/Left.d.ats");
    Write("generated/Right.d.ats");
    Write("left/Peer.ts");
    Write("right/Peer.ts");
    ASSERT_NO_FATAL_FAILURE(Start(R"("dependencies":{
        "left":{"language":"js", "ohmUrl":"left", "path":"generated/Left.d.ats", "sourceFilePath":"left/Entry.ts"},
        "right":{"language":"js", "ohmUrl":"right", "path":"generated/Right.d.ats", "sourceFilePath":"right/Entry.ts"},
        "left-peer":{"language":"js", "ohmUrl":"left.peer", "sourceFilePath":"left/Peer.ts"},
        "right-peer":{"language":"js", "ohmUrl":"right.peer", "sourceFilePath":"right/Peer.ts"}})"));
    auto *left = Import("left");
    auto *right = Import("right");
    ASSERT_NE(left, nullptr);
    ASSERT_NE(right, nullptr);
    auto *fromLeft = Import("./Peer", left);
    auto *fromRight = Import("./Peer", right);
    ASSERT_NE(fromLeft, nullptr);
    ASSERT_NE(fromRight, nullptr);
    EXPECT_NE(fromLeft, fromRight);
    EXPECT_EQ(fromLeft->GetImportInfo().ResolvedSource(), "left-peer");
    EXPECT_EQ(fromRight->GetImportInfo().ResolvedSource(), "right-peer");
    EXPECT_EQ(Import("./Peer", left), fromLeft);
    EXPECT_EQ(Import("./Peer", right), fromRight);
    const auto &edges = Manager()->GetFileDependencies();
    auto leftEdges = edges.find(ArenaString(left->AbsoluteName().Utf8()));
    auto rightEdges = edges.find(ArenaString(right->AbsoluteName().Utf8()));
    ASSERT_NE(leftEdges, edges.end());
    ASSERT_NE(rightEdges, edges.end());
    EXPECT_EQ(leftEdges->second.size(), 1U);
    EXPECT_EQ(leftEdges->second.count(ArenaString("left-peer")), 1U);
    EXPECT_EQ(rightEdges->second.size(), 1U);
    EXPECT_EQ(rightEdges->second.count(ArenaString("right-peer")), 1U);
}

TEST_F(InteropImportPathTest, MissingMappedDeclarationDoesNotSelectAnAvailableLogicalFallback)
{
    Write("generated/Entry.d.ats");
    Write("src/Peer.ts");
    Write("generated/Fallback.d.ats");
    ASSERT_NO_FATAL_FAILURE(Start(R"("dependencies":{
        "pkg/entry":{"language":"js", "ohmUrl":"entry", "path":"generated/Entry.d.ats",
            "sourceFilePath":"src/Entry.ts"},
        "pkg/Peer":{"language":"js", "ohmUrl":"fallback", "path":"generated/Fallback.d.ats"},
        "external-peer":{"language":"js", "ohmUrl":"peer", "path":"generated/Peer.d.ats",
            "sourceFilePath":"src/Peer.ts"}})"));
    auto *entry = Import("pkg/entry");
    ASSERT_NE(entry, nullptr);
    const auto queueSize = Manager()->GetParseQueue().size();
    EXPECT_EQ(Import("./Peer", entry), nullptr);
    const auto &errors = Context()->diagnosticEngine->GetDiagnosticStorage(DiagnosticType::FATAL);
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front()->GetId(), diagnostic::INTEROP_DYNAMIC_FILE_NOT_FOUND.Id());
    EXPECT_NE(errors.front()->Message().find("./Peer"), std::string::npos);
    EXPECT_EQ(Manager()->GetParseQueue().size(), queueSize);
    EXPECT_TRUE(Manager()->CollectReverseDependencies("pkg/Peer").empty());
    EXPECT_TRUE(Manager()->CollectReverseDependencies("external-peer").empty());
    Context()->diagnosticEngine->ClearDiagnostics();

    Write("generated/Peer.d.ats", "export declare const repaired: int;");
    Manager()->ClearResolutionCaches();
    auto *peer = Import("./Peer", entry);
    ASSERT_NE(peer, nullptr);
    EXPECT_EQ(peer->GetImportInfo().ResolvedSource(), "external-peer");
    EXPECT_EQ(peer->SourceCode(), "export declare const repaired: int;");
    EXPECT_TRUE(Manager()->CollectReverseDependencies("pkg/Peer").empty());
}

TEST_F(InteropImportPathTest, ParserResolvesReExportFromGeneratedDeclarationAgainstItsOriginalDirectory)
{
    Write("Main.ats", R"(import { value } from "bridge";)");
    Write("declarations/Bridge.d.ats", R"(export { value } from "./Peer";)");
    Write("declarations/Peer.ats", "export const decoy: int = 0;");
    Write("source/Peer.ts", "export const value = 42;");
    Write("interfaces/Api.d.ats", "export declare const value: int;");
    ASSERT_NO_FATAL_FAILURE(Configure(R"("dependencies":{
        "bridge":{"language":"js", "ohmUrl":"bridge", "path":"declarations/Bridge.d.ats",
                  "sourceFilePath":"source/Bridge.ts"},
        "api":{"language":"js", "ohmUrl":"api", "path":"interfaces/Api.d.ats", "sourceFilePath":"source/Peer.ts"}})"));
    ASSERT_NO_FATAL_FAILURE(OpenFile("Main.ats"));
    impl_->ProceedToState(handle_, ES2PANDA_STATE_PARSED);
    ASSERT_EQ(impl_->ContextState(handle_), ES2PANDA_STATE_PARSED) << impl_->ContextErrorMessage(handle_);
    ASSERT_FALSE(Context()->diagnosticEngine->IsAnyError());
    const auto &edges = Manager()->GetFileDependencies();
    auto mainEdges = edges.find(ArenaString(Path("Main.ats").string()));
    ASSERT_NE(mainEdges, edges.end());
    EXPECT_EQ(mainEdges->second.count(ArenaString("bridge")), 1U);
    auto bridgeEdges = edges.find(ArenaString(Path("declarations/Bridge.d.ats").string()));
    ASSERT_NE(bridgeEdges, edges.end());
    EXPECT_EQ(bridgeEdges->second.count(ArenaString("api")), 1U);
    EXPECT_TRUE(Manager()->CollectReverseDependencies(Path("declarations/Peer.ats").string()).empty());
    const auto &queue = Manager()->GetParseQueue();
    auto imported = std::find_if(queue.begin(), queue.end(), [](const auto &item) {
        return item.program->GetImportInfo().ResolvedSource() == "api";
    });
    ASSERT_NE(imported, queue.end());
    EXPECT_TRUE(imported->isParsed);
    EXPECT_EQ(fs::path(imported->program->GetImportInfo().TextSource()), Path("interfaces/Api.d.ats"));
}

TEST_F(InteropImportPathTest, MissingDynamicAliasTargetDoesNotHideAStaticPackageWithTheSameSpecifier)
{
    Write("sdk/Peer.ats");
    Write("Entry.d.ats");
    ASSERT_NO_FATAL_FAILURE(Start(R"("atsDependencies":{"sdk":{"moduleRoot":"sdk"}},
        "dependencies":{"bridge":{"language":"js", "ohmUrl":"bridge", "path":"Entry.d.ats"},
        "foreign":{"language":"js", "ohmUrl":"foreign", "alias":["sdk/Peer"], "path":"foreign/Peer.d.ats"}})"));
    auto *entry = Import("bridge");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(Import("sdk/Peer", entry), nullptr);
    const auto &errors = Context()->diagnosticEngine->GetDiagnosticStorage(DiagnosticType::FATAL);
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front()->GetId(), diagnostic::INTEROP_DYNAMIC_FILE_NOT_FOUND.Id());
    Context()->diagnosticEngine->ClearDiagnostics();
    auto *local = Import("sdk/Peer");
    ASSERT_NE(local, nullptr);
    EXPECT_EQ(local->GetImportInfo().ResolvedSource(), Path("sdk/Peer.ats").string());
    EXPECT_TRUE(Manager()->CollectReverseDependencies("foreign").empty());

    Write("foreign/Peer.d.ats", "export declare class Peer {}");
    Manager()->ClearResolutionCaches();
    auto *foreign = Import("sdk/Peer", entry);
    ASSERT_NE(foreign, nullptr);
    EXPECT_NE(foreign, local);
    EXPECT_EQ(foreign->GetImportInfo().ResolvedSource(), "foreign");
    EXPECT_EQ(Import("sdk/Peer"), local);
}

TEST_F(InteropImportPathTest, UnmappedStaticSiblingOfADynamicSourceUsesStaticRulesForItsOwnImports)
{
    Write("generated/Bridge.d.ats");
    Write("source/Adapter.ats");
    Write("sdk/Item.ats");
    ASSERT_NO_FATAL_FAILURE(Start(R"("atsDependencies":{"sdk":{"moduleRoot":"sdk"}},
        "dependencies":{"bridge":{"language":"js", "ohmUrl":"bridge", "path":"generated/Bridge.d.ats",
                                  "sourceFilePath":"source/Bridge.ts"},
                        "foreign-item":{"language":"js", "ohmUrl":"item", "alias":["sdk/Item"]}})"));
    auto *bridge = Import("bridge");
    ASSERT_NE(bridge, nullptr);
    auto *adapter = Import("./Adapter", bridge);
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(adapter->Is<ModuleKind::MODULE>());
    EXPECT_FALSE(adapter->IsDeclForDynamicStaticInterop());
    EXPECT_EQ(adapter->GetImportInfo().ResolvedSource(), Path("source/Adapter.ats").string());
    auto *foreign = Import("sdk/Item", bridge);
    auto *local = Import("sdk/Item", adapter);
    ASSERT_NE(foreign, nullptr);
    ASSERT_NE(local, nullptr);
    EXPECT_EQ(foreign->GetImportInfo().ResolvedSource(), "foreign-item");
    EXPECT_EQ(local->GetImportInfo().ResolvedSource(), Path("sdk/Item.ats").string());
    EXPECT_EQ(Import("sdk/Item", bridge), foreign);
}

}  // namespace
}  // namespace ark::es2panda::util::test
