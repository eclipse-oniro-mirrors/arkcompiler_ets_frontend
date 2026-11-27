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

#include <set>

#include "import_path_manager_test.h"
#include "ir/ets/etsPackageDeclaration.h"
#include "ir/statements/blockStatement.h"

namespace ark::es2panda::util::test {
namespace {

// The compiler host owns these caches. Restore its context before the local cache is destroyed.
class ScopedHostCache {
public:
    explicit ScopedHostCache(public_lib::Context *context) : context_(context), previous_(context->globalContext)
    {
        context_->globalContext = &host;
    }

    NO_COPY_SEMANTIC(ScopedHostCache);
    NO_MOVE_SEMANTIC(ScopedHostCache);

    ~ScopedHostCache()
    {
        context_->globalContext = previous_;
    }

    public_lib::GlobalContext host {};
    parser::Program::ExternalPrograms stdlib;

private:
    public_lib::Context *context_;
    public_lib::GlobalContext *previous_;
};

class ImportCacheLookupTest : public ImportPathManagerTest {
protected:
    void ParseWorkspace(std::string_view fields = {})
    {
        ASSERT_NO_FATAL_FAILURE(Configure(fields));
        ASSERT_NO_FATAL_FAILURE(OpenFile("Main.ats"));
        impl_->ProceedToState(handle_, ES2PANDA_STATE_PARSED);
        ASSERT_EQ(impl_->ContextState(handle_), ES2PANDA_STATE_PARSED) << impl_->ContextErrorMessage(handle_);
    }

    void Evict(parser::Program *program)
    {
        Manager()->RemoveProgramFromResolvedSources(ArenaString(program->GetImportInfo().Key()));
        ASSERT_EQ(Manager()->SearchResolvedExact(program->GetImportInfo()), nullptr);
    }
};

TEST_F(ImportCacheLookupTest, HostWithoutAnAstCacheLoadsAnUnseenFileAndReusesItOnTheNextLookup)
{
    Write("Unseen.ats", "export const version: int = 3;");
    ASSERT_NO_FATAL_FAILURE(Start());
    ScopedHostCache cache(Context());
    const auto before = Manager()->GetParseQueue().size();
    auto *leaf = Import("./Unseen");
    ASSERT_NE(leaf, nullptr);
    EXPECT_EQ(leaf->SourceCode(), "export const version: int = 3;");
    EXPECT_EQ(Manager()->GetParseQueue().size(), before + 1);
    EXPECT_EQ(Import("./Unseen"), leaf);
    EXPECT_EQ(Manager()->GetParseQueue().size(), before + 1);
}

TEST_F(ImportCacheLookupTest, IncrementalHostCacheHitMarksTheRestoredQueueEntryAsAlreadyParsed)
{
    Write("Main.ats", "import { value } from './Leaf';");
    Write("Leaf.ats");
    ASSERT_NO_FATAL_FAILURE(ParseWorkspace());
    auto *leaf = Import("./Leaf");
    ASSERT_NE(leaf, nullptr);
    ASSERT_NE(leaf->Ast(), nullptr);
    ASSERT_NO_FATAL_FAILURE(Evict(leaf));
    ScopedHostCache cache(Context());
    const auto key = Path("Leaf.ats").string();
    cache.host.cachedExternalPrograms.emplace(key, leaf);
    const auto queueSize = Manager()->GetParseQueue().size();
    auto *restored = Manager()->FindOrIntroduceProgramForIncremental(key);
    EXPECT_EQ(restored, leaf);
    EXPECT_EQ(Manager()->SearchResolvedExact(leaf->GetImportInfo()), leaf);
    ASSERT_EQ(Manager()->GetParseQueue().size(), queueSize + 1);
    EXPECT_EQ(Manager()->GetParseQueue().back().program, leaf);
    EXPECT_TRUE(Manager()->GetParseQueue().back().isParsed);
    const auto &external = Main()->GetExternalPrograms()->Get<ModuleKind::MODULE>();
    EXPECT_EQ(std::count(external.begin(), external.end(), leaf), 1);
}

TEST_F(ImportCacheLookupTest, EmptyHostEntryMasksAnOlderStdlibAstForTheSameKey)
{
    Write("Main.ats", "import { value } from './Leaf';");
    Write("Leaf.ats");
    ASSERT_NO_FATAL_FAILURE(ParseWorkspace());
    auto *leaf = Import("./Leaf");
    ASSERT_NE(leaf, nullptr);
    ASSERT_NE(leaf->Ast(), nullptr);
    ASSERT_NO_FATAL_FAILURE(Evict(leaf));
    ScopedHostCache cache(Context());
    cache.stdlib.Add(leaf);
    cache.host.stdLibAstCache = &cache.stdlib;
    auto key = Path("Leaf.ats").string();
    cache.host.cachedExternalPrograms.emplace(key, nullptr);

    auto *fresh = Manager()->FindOrIntroduceProgramForIncremental(key);
    ASSERT_NE(fresh, nullptr);
    EXPECT_NE(fresh, leaf);
    EXPECT_EQ(fresh->SourceCode(), leaf->SourceCode());
    EXPECT_EQ(Manager()->SearchResolved(fresh->GetImportInfo()), fresh);
    ASSERT_FALSE(Manager()->GetParseQueue().empty());
    EXPECT_EQ(Manager()->GetParseQueue().back().program, fresh);
    EXPECT_FALSE(Manager()->GetParseQueue().back().isParsed);

    ASSERT_NO_FATAL_FAILURE(Evict(fresh));
    cache.host.cachedExternalPrograms.erase(key);
    EXPECT_EQ(Manager()->FindOrIntroduceProgramForIncremental(key), leaf);
}

TEST_F(ImportCacheLookupTest, StdlibAstLookupDistinguishesParsedSourceAndBothKindsOfTextDeclaration)
{
    Write("Main.ats", R"(import { live } from "./Live";
        import { api } from "./Api";
        import { cached } from "./Cached";)");
    Write("Live.ats", "export const live: int = 1;");
    Write("Api.d.ats", "export declare const api: int;");
    Write("Cached.ats", "export const cached: int = 1;");
    Write("cache/Cached.etscache", "export declare const cached: int;");
    ASSERT_NO_FATAL_FAILURE(ParseWorkspace(CacheDirectory()));
    auto *live = Import("./Live");
    auto *api = Import("./Api");
    auto *cached = Import("./Cached");
    ASSERT_NE(live, nullptr);
    ASSERT_NE(api, nullptr);
    ASSERT_NE(cached, nullptr);
    ASSERT_TRUE(api->Is<ModuleKind::SOURCE_DECL>());
    ASSERT_TRUE(cached->Is<ModuleKind::ETSCACHE_DECL>());

    ScopedHostCache cache(Context());
    cache.host.stdLibAstCache = &cache.stdlib;
    for (auto *program : {live, api, cached}) {
        ASSERT_NE(program->Ast(), nullptr);
        cache.stdlib.Add(program);
        ASSERT_NO_FATAL_FAILURE(Evict(program));
    }
    for (auto *program : {cached, api, live}) {
        SCOPED_TRACE(std::string(program->GetImportInfo().Key()));
        auto *restored = Manager()->FindOrIntroduceProgramForIncremental(program->GetImportInfo().Key());
        EXPECT_EQ(restored, program);
        EXPECT_EQ(Manager()->SearchResolvedExact(program->GetImportInfo()), program);
        EXPECT_EQ(Manager()->GetParseQueue().back().program, program);
        EXPECT_TRUE(Manager()->GetParseQueue().back().isParsed);
    }
}

TEST_F(ImportCacheLookupTest, IncrementalDirectoryLookupExposesItsFractionsWhenLaterImported)
{
    Write("pkg/First.ats");
    Write("pkg/Second.ets");
    Write("pkg/nested/Other.ats");
    ASSERT_NO_FATAL_FAILURE(Start());
    const auto key = Path("pkg").string();
    auto *package = Manager()->FindOrIntroduceProgramForIncremental(key);
    ASSERT_NE(package, nullptr);
    ASSERT_TRUE(package->Is<ModuleKind::PACKAGE>());
    EXPECT_TRUE(package->GetImportInfo().PointsToPackage());
    auto &external = Main()->GetExternalPrograms()->Get<ModuleKind::PACKAGE>();
    EXPECT_TRUE(external.empty());
    const auto &fractions = package->As<ModuleKind::PACKAGE>()->GetUnmergedPackagePrograms();
    std::set<std::string> paths;
    for (auto *fraction : fractions) {
        paths.emplace(fraction->GetImportInfo().Key());
        EXPECT_EQ(fraction->ModuleName(), package->ModuleName());
    }
    EXPECT_EQ(paths, (std::set<std::string> {Path("pkg/First.ats").string(), Path("pkg/Second.ets").string()}));
    EXPECT_EQ(Import("./pkg"), package);
    EXPECT_EQ(Import("./pkg"), package);
    ASSERT_EQ(external.size(), 1U);
    EXPECT_EQ(external.front(), package);
}

TEST_F(ImportCacheLookupTest, OnlyTheCurrentExactDeclarationReportsReplacementAfterReintroduction)
{
    Write("Api.d.ats", "export declare const original: int;");
    ASSERT_NO_FATAL_FAILURE(Start());
    auto *old = Import("./Api.d.ats");
    ASSERT_NE(old, nullptr);
    ASSERT_NO_FATAL_FAILURE(Evict(old));
    Write("Api.d.ats", "export declare const modified: int;");
    auto *fresh = Import("./Api.d.ats");
    ASSERT_NE(fresh, nullptr);
    ASSERT_NE(fresh, old);
    EXPECT_EQ(fresh->SourceCode(), "export declare const original: int;");
    Write("Api.ats");
    auto *implementation = Import("./Api.ats");
    ASSERT_NE(implementation, nullptr);
    EXPECT_EQ(Manager()->SearchResolved(fresh->GetImportInfo()), implementation);
    EXPECT_EQ(Manager()->SearchResolvedExact(old->GetImportInfo()), fresh);
    EXPECT_TRUE(Manager()->IsReplacedExactSource(fresh));
    EXPECT_FALSE(Manager()->IsReplacedExactSource(old));
}

TEST_F(ImportCacheLookupTest, RemovingTheSecondaryIndexKeepsThePrimaryIndexAndItsDeclarationRegistered)
{
    Write("one/index.ats");
    Write("two/index.ats");
    Write("one/index.d.ats");
    ASSERT_NO_FATAL_FAILURE(Start(R"("paths":{"lib":["one","two"]})"));
    auto *declaration = Import("./one/index.d.ats");
    auto *primary = Import("./one/index.ats");
    auto *secondary = Import("./two/index.ats");
    ASSERT_NE(primary, nullptr);
    ASSERT_NE(secondary, nullptr);
    ASSERT_NE(declaration, nullptr);
    ASSERT_EQ(primary->ModuleName(), secondary->ModuleName());
    ASSERT_NO_FATAL_FAILURE(Evict(secondary));
    EXPECT_EQ(Manager()->SearchResolved(declaration->GetImportInfo()), primary);
    EXPECT_EQ(Manager()->SearchResolvedExact(primary->GetImportInfo()), primary);
    ASSERT_NO_FATAL_FAILURE(Evict(primary));
    EXPECT_EQ(Manager()->SearchResolved(declaration->GetImportInfo()), declaration);
}

TEST_F(ImportCacheLookupTest, EntryPackageRegistrationIsIdempotentAndItsLogicalNameCanBeEvicted)
{
    Write("Main.ats", "package Example; export const value: int = 1;");
    ASSERT_NO_FATAL_FAILURE(Configure("", {"--simultaneous"}));
    const auto input = Path("Main.ats").string();
    const char *files[] = {input.c_str()};
    handle_ = impl_->CreateContextSimultaneousMode(config_, 1, files);
    ASSERT_NE(handle_, nullptr);
    impl_->ProceedToState(handle_, ES2PANDA_STATE_PARSED);
    ASSERT_EQ(impl_->ContextState(handle_), ES2PANDA_STATE_PARSED) << impl_->ContextErrorMessage(handle_);
    ExpectDiagnostic(diagnostic::PACKAGE_DEPRECATED);
    Context()->diagnosticEngine->ClearDiagnostics();

    ImportInfo entryKey {*Manager(), input};
    auto *fraction = Manager()->SearchResolvedExact(entryKey);
    ASSERT_NE(fraction, nullptr);
    auto *package = Manager()->SearchResolved(entryKey);
    ASSERT_NE(package, nullptr);
    ASSERT_TRUE(package->Is<ModuleKind::PACKAGE>());
    ASSERT_NE(fraction->Ast(), nullptr);
    const auto &statements = fraction->Ast()->Statements();
    const auto decl =
        std::find_if(statements.begin(), statements.end(), [](auto *node) { return node->IsETSPackageDeclaration(); });
    ASSERT_NE(decl, statements.end());
    EXPECT_EQ(Manager()->EnsurePackageIsRegisteredByPackageFraction(fraction, (*decl)->AsETSPackageDeclaration()),
              package);
    const auto &fractions = package->As<ModuleKind::PACKAGE>()->GetUnmergedPackagePrograms();
    ASSERT_EQ(fractions.size(), 1U);
    EXPECT_EQ(fractions.front(), fraction);
    ImportInfo logical {*Manager(), "Example"};
    EXPECT_EQ(Manager()->SearchResolved(logical), package);
    EXPECT_EQ(Manager()->SearchResolvedExact(logical), nullptr);
    Manager()->RemoveProgramFromResolvedSources(ArenaString("Example"));
    EXPECT_EQ(Manager()->SearchResolved(logical), nullptr);
    EXPECT_EQ(Manager()->SearchResolvedExact(fraction->GetImportInfo()), fraction);
}

TEST_F(ImportCacheLookupTest, CachedDirectoryAstRestoresLogicalAndPhysicalKeysWithoutRediscoveringFractions)
{
    Write("models/One.ats");
    Write("models/Two.ets");
    Write("Unrelated.ats");
    ASSERT_NO_FATAL_FAILURE(Start());
    auto *package = Import("./models");
    ASSERT_NE(package, nullptr);
    ASSERT_TRUE(package->Is<ModuleKind::PACKAGE>());
    ASSERT_NE(package->Ast(), nullptr);
    const auto &fractions = package->As<ModuleKind::PACKAGE>()->GetUnmergedPackagePrograms();
    ASSERT_EQ(fractions.size(), 2U);
    ImportInfo logical {*Manager(), package->ModuleName()};
    ASSERT_NO_FATAL_FAILURE(Evict(package));
    EXPECT_EQ(Manager()->SearchResolved(logical), nullptr);

    ScopedHostCache cache(Context());
    cache.stdlib.Add(package);
    cache.host.stdLibAstCache = &cache.stdlib;
    auto *unrelated = Import("./Unrelated");
    ASSERT_NE(unrelated, nullptr);
    EXPECT_FALSE(unrelated->GetImportInfo().PointsToPackage());
    Write("models/New.ats");
    Manager()->ClearResolutionCaches();
    const auto queueSize = Manager()->GetParseQueue().size();
    EXPECT_EQ(Manager()->FindOrIntroduceProgramForIncremental(Path("models").string()), package);
    EXPECT_EQ(Manager()->SearchResolvedExact(logical), package);
    EXPECT_EQ(Manager()->SearchResolvedExact(package->GetImportInfo()), package);
    EXPECT_EQ(fractions.size(), 2U);
    EXPECT_EQ(Manager()->GetParseQueue().size(), queueSize);
}

}  // namespace
}  // namespace ark::es2panda::util::test
