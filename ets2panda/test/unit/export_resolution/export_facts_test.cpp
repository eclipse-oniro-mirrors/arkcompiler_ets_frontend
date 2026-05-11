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

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string_view>

#include <gtest/gtest.h>

#include "ir/ets/etsImportDeclaration.h"
#include "test/utils/checker_test.h"
#include "varbinder/exportFacts.h"

namespace ark::es2panda::compiler::test {

namespace fs = std::filesystem;

bool EndsWith(std::string_view value, std::string_view suffix)
{
    return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
}

class ExportFactsTest : public ::test::utils::CheckerTest {
public:
    void SetUp() override
    {
        const auto *testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
        workDir_ = fs::temp_directory_path() / "es2panda_export_facts" / testInfo->name();
        fs::remove_all(workDir_);
        fs::create_directories(workDir_);
    }

    void TearDown() override
    {
        fs::remove_all(workDir_);
    }

protected:
    void WriteFile(std::string_view relativePath, std::string_view content)
    {
        if (relativePath.empty()) {
            ADD_FAILURE() << "relativePath must not be empty";
            return;
        }
        auto absolutePath = workDir_;
        absolutePath.append(std::string {relativePath});
        fs::create_directories(absolutePath.parent_path());

        std::ofstream output(absolutePath);
        ASSERT_TRUE(output.is_open());
        output << content;
    }

    void InitializeFromFile(std::string_view relativePath, std::string_view content)
    {
        if (relativePath.empty()) {
            ADD_FAILURE() << "relativePath must not be empty";
            return;
        }
        WriteFile(relativePath, content);
        auto absolutePath = workDir_;
        absolutePath.append(std::string {relativePath});
        InitializeChecker(absolutePath.string(), content);
    }

    parser::Program *CurrentProgram()
    {
        return Checker()->VarBinder() == nullptr ? nullptr : Checker()->VarBinder()->Program();
    }

    const varbinder::ExportFactStore::ExportFactSnapshot &Snapshot()
    {
        auto *program = CurrentProgram();
        EXPECT_NE(program, nullptr);
        return program->VarBinder()->AsETSBinder()->GetExportFacts(program);
    }

    const varbinder::ExportFactStore &Store()
    {
        auto *program = CurrentProgram();
        EXPECT_NE(program, nullptr);
        return program->VarBinder()->AsETSBinder()->GetExportFactsStore();
    }

    void ExpectLocalExportFact(const varbinder::ExportFactStore::ExportFactSnapshot &snapshot, parser::Program *program)
    {
        ASSERT_EQ(snapshot.locals.size(), 1U);
        const auto &local = snapshot.locals[0];
        EXPECT_TRUE(local.exportedName.Is("LocalExport"));
        EXPECT_TRUE(local.importedName.Empty());
        EXPECT_FALSE(local.isTypeOnly);
        EXPECT_EQ(local.sourceProgram, program);
        EXPECT_EQ(local.importDecl, nullptr);
        EXPECT_NE(local.origin, nullptr);
        EXPECT_NE(local.variable, nullptr);
    }

    void ExpectNamedReExportFacts(const varbinder::ExportFactStore::ExportFactSnapshot &snapshot)
    {
        ASSERT_EQ(snapshot.namedReExports.size(), 2U);
        const auto named = std::find_if(snapshot.namedReExports.begin(), snapshot.namedReExports.end(),
                                        [](const auto &fact) { return fact.exportedName.Is("Renamed"); });
        ASSERT_NE(named, snapshot.namedReExports.end());
        EXPECT_TRUE(named->importedName.Is("Original"));
        EXPECT_FALSE(named->isTypeOnly);
        EXPECT_NE(named->importDecl, nullptr);
        EXPECT_TRUE(EndsWith(named->importDecl->ResolvedSource(), "/source.ets"));
        EXPECT_NE(named->origin, nullptr);
        EXPECT_EQ(named->variable, nullptr);

        const auto typeOnly = std::find_if(snapshot.namedReExports.begin(), snapshot.namedReExports.end(),
                                           [](const auto &fact) { return fact.exportedName.Is("ExportedType"); });
        ASSERT_NE(typeOnly, snapshot.namedReExports.end());
        EXPECT_TRUE(typeOnly->importedName.Is("TypeSource"));
        EXPECT_NE(typeOnly->importDecl, nullptr);
        EXPECT_TRUE(EndsWith(typeOnly->importDecl->ResolvedSource(), "/source.ets"));
        EXPECT_NE(typeOnly->origin, nullptr);
        EXPECT_EQ(typeOnly->variable, nullptr);
    }

    void ExpectStarAndNamespaceExportFacts(const varbinder::ExportFactStore::ExportFactSnapshot &snapshot)
    {
        ASSERT_EQ(snapshot.starExports.size(), 1U);
        ASSERT_EQ(snapshot.namespaceExports.size(), 1U);

        const auto &star = snapshot.starExports[0];
        EXPECT_TRUE(star.exportedName.Empty());
        EXPECT_TRUE(star.importedName.Empty());
        EXPECT_FALSE(star.isTypeOnly);
        EXPECT_NE(star.importDecl, nullptr);
        EXPECT_TRUE(EndsWith(star.importDecl->ResolvedSource(), "/source.ets"));
        EXPECT_NE(star.origin, nullptr);

        const auto &namespaceExport = snapshot.namespaceExports[0];
        EXPECT_TRUE(namespaceExport.exportedName.Is("NamespaceAlias"));
        EXPECT_TRUE(namespaceExport.importedName.Empty());
        EXPECT_FALSE(namespaceExport.isTypeOnly);
        EXPECT_NE(namespaceExport.importDecl, nullptr);
        EXPECT_TRUE(EndsWith(namespaceExport.importDecl->ResolvedSource(), "/source.ets"));
        EXPECT_NE(namespaceExport.origin, nullptr);
        EXPECT_NE(namespaceExport.variable, nullptr);
    }

private:
    fs::path workDir_;
};

TEST_F(ExportFactsTest, GetExportFactsReturnsReadableSnapshot)
{
    InitializeFromFile("main.ets", R"ETS(
        class LocalExport {}
    )ETS");

    const auto &snapshot = Snapshot();

    EXPECT_TRUE(snapshot.locals.empty());
    EXPECT_TRUE(snapshot.namedReExports.empty());
    EXPECT_TRUE(snapshot.starExports.empty());
    EXPECT_TRUE(snapshot.namespaceExports.empty());
}

TEST_F(ExportFactsTest, CollectsLocalAndReExportFacts)
{
    WriteFile("source.ets", R"ETS(
        export class Original {}
        export type TypeSource = int;
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        export class LocalExport {}
        export { Original as Renamed } from "./source.ets";
        export * from "./source.ets";
        export * as NamespaceAlias from "./source.ets";
        export type { TypeSource as ExportedType } from "./source.ets";
    )ETS");

    const auto &snapshot = Snapshot();
    auto *program = CurrentProgram();
    ASSERT_NE(program, nullptr);

    ExpectLocalExportFact(snapshot, program);
    ExpectNamedReExportFacts(snapshot);
    ExpectStarAndNamespaceExportFacts(snapshot);

    const auto *mainSurface = Store().FindSurfaceByResolvedSource(program->GetImportInfo().ResolvedSource());
    ASSERT_NE(mainSurface, nullptr);
    EXPECT_EQ(mainSurface->program, program);
}

TEST_F(ExportFactsTest, CollectsLocalSelectiveExportAliasByExportedName)
{
    InitializeFromFile("main.ets", R"ETS(
        class LocalA {}
        export { LocalA as A };
    )ETS");

    const auto &snapshot = Snapshot();
    EXPECT_EQ(snapshot.locals.size(), 1U);

    const auto alias = std::find_if(snapshot.locals.begin(), snapshot.locals.end(),
                                    [](const auto &fact) { return fact.exportedName.Is("A"); });
    ASSERT_NE(alias, snapshot.locals.end());
    EXPECT_TRUE(alias->localName.Is("LocalA"));
    EXPECT_NE(alias->variable, nullptr);
    ASSERT_NE(alias->variable->Declaration(), nullptr);
    ASSERT_NE(alias->variable->Declaration()->Node(), nullptr);
    EXPECT_TRUE(alias->variable->Declaration()->Node()->IsClassDefinition())
        << static_cast<int>(alias->variable->Declaration()->Node()->Type());
    EXPECT_NE(alias->origin, nullptr);

    const auto leakedLocal = std::find_if(snapshot.locals.begin(), snapshot.locals.end(),
                                          [](const auto &fact) { return fact.exportedName.Is("LocalA"); });
    EXPECT_EQ(leakedLocal, snapshot.locals.end());
}

TEST_F(ExportFactsTest, CollectsSameNameSelectiveExportAsClassDefinition)
{
    InitializeFromFile("main.ets", R"ETS(
        class A {}
        export { A };
    )ETS");

    const auto &snapshot = Snapshot();

    const auto alias = std::find_if(snapshot.locals.begin(), snapshot.locals.end(),
                                    [](const auto &fact) { return fact.exportedName.Is("A"); });
    ASSERT_NE(alias, snapshot.locals.end());
    ASSERT_NE(alias->variable, nullptr);
    ASSERT_NE(alias->variable->Declaration(), nullptr);
    ASSERT_NE(alias->variable->Declaration()->Node(), nullptr);
    EXPECT_TRUE(alias->variable->Declaration()->Node()->IsClassDefinition())
        << static_cast<int>(alias->variable->Declaration()->Node()->Type());
}

TEST_F(ExportFactsTest, CollectsExportDeclareNamespace)
{
    InitializeFromFile("main.ets", R"ETS(
        export declare namespace ns {}
    )ETS");

    const auto &snapshot = Snapshot();

    const auto exportedNamespace = std::find_if(snapshot.locals.begin(), snapshot.locals.end(),
                                                [](const auto &fact) { return fact.exportedName.Is("ns"); });
    ASSERT_NE(exportedNamespace, snapshot.locals.end());
    EXPECT_TRUE(exportedNamespace->localName.Is("ns"));
    EXPECT_NE(exportedNamespace->variable, nullptr);
    EXPECT_NE(exportedNamespace->origin, nullptr);
}

}  // namespace ark::es2panda::compiler::test
