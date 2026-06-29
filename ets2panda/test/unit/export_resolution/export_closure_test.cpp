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

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "test/utils/checker_test.h"

namespace ark::es2panda::compiler::test {

namespace fs = std::filesystem;

class ExportResolutionTest : public ::test::utils::CheckerTest {
public:
    void SetUp() override
    {
        const auto *testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
        workDir_ = fs::temp_directory_path() / "es2panda_export_closure" / testInfo->name();
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

    void WriteImportMatrixFiles()
    {
        WriteFile("source.ets", R"ETS(
            export default function DefaultValue(): int { return 0; }
            export function a(): int { return 1; }
            export function b(): int { return 2; }
            export class TypeOnly {}
        )ETS");
    }

    void WriteNamedOnlyImportMatrixFiles()
    {
        WriteFile("source.ets", R"ETS(
            export function a(): int { return 1; }
            export function b(): int { return 2; }
            export class TypeOnly {}
        )ETS");
    }

    void WriteDefaultOnlyImportMatrixFiles()
    {
        WriteFile("source.ets", R"ETS(
            export default function value(): int { return 0; }
        )ETS");
    }

private:
    fs::path workDir_;
};

TEST_F(ExportResolutionTest, MergesSameSourceExportsFromDifferentStarPaths)
{
    WriteFile("shared.ets", R"ETS(
        export function Shared(): int { return 1; }
    )ETS");
    WriteFile("left.ets", R"ETS(
        export * from "./shared.ets";
    )ETS");
    WriteFile("right.ets", R"ETS(
        export * from "./shared.ets";
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        export * from "./left.ets";
        export * from "./right.ets";
    )ETS");

    const auto &cache = Checker()->ResolveExportClosure(Program());

    EXPECT_TRUE(cache.Has("Shared"));
    EXPECT_FALSE(cache.HasAmbiguous("Shared"));
    EXPECT_NE(cache.Find("Shared"), nullptr);
}

TEST_F(ExportResolutionTest, MarksDifferentOriginsFromStarExportsAsAmbiguous)
{
    WriteFile("a.ets", R"ETS(
        export function Duplicated(): int { return 1; }
    )ETS");
    WriteFile("b.ets", R"ETS(
        export function Duplicated(): int { return 2; }
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        export * from "./a.ets";
        export * from "./b.ets";
    )ETS");

    const auto &cache = Checker()->ResolveExportClosure(Program());

    EXPECT_TRUE(cache.HasAmbiguous("Duplicated"));
    EXPECT_EQ(cache.Find("Duplicated"), nullptr);
}

TEST_F(ExportResolutionTest, ExplicitNamedReExportCanAdjudicateOneBranch)
{
    WriteFile("a.ets", R"ETS(
        export function Duplicated(): int { return 1; }
    )ETS");
    WriteFile("b.ets", R"ETS(
        export function Duplicated(): int { return 2; }
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        export * from "./a.ets";
        export * from "./b.ets";
        export { Duplicated as LeftDuplicated } from "./a.ets";
    )ETS");

    const auto &cache = Checker()->ResolveExportClosure(Program());

    EXPECT_TRUE(cache.Has("LeftDuplicated"));
    EXPECT_FALSE(cache.HasAmbiguous("LeftDuplicated"));
    EXPECT_NE(cache.Find("LeftDuplicated"), nullptr);
    EXPECT_TRUE(cache.HasAmbiguous("Duplicated"));
}

TEST_F(ExportResolutionTest, ExplicitNamedReExportOverridesStarCollision)
{
    WriteFile("a.ets", R"ETS(
        export function Duplicated(): int { return 1; }
    )ETS");
    WriteFile("b.ets", R"ETS(
        export function Duplicated(): int { return 2; }
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        export * from "./a.ets";
        export * from "./b.ets";
        export { Duplicated } from "./a.ets";
    )ETS");

    const auto &cache = Checker()->ResolveExportClosure(Program());

    EXPECT_TRUE(cache.Has("Duplicated"));
    EXPECT_FALSE(cache.HasAmbiguous("Duplicated"));
    EXPECT_NE(cache.Find("Duplicated"), nullptr);
    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, ImportUsesExplicitReExportThatOverridesStarCollision)
{
    WriteFile("a.ets", R"ETS(
        export function Duplicated(): int { return 1; }
    )ETS");
    WriteFile("b.ets", R"ETS(
        export function Duplicated(): int { return 2; }
    )ETS");
    WriteFile("bridge.ets", R"ETS(
        export * from "./a.ets";
        export * from "./b.ets";
        export { Duplicated } from "./a.ets";
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { Duplicated } from "./bridge.ets";

        let value: int = Duplicated();
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, ExplicitReExportOverridesStarCycleForRequestedName)
{
    WriteFile("a.ets", R"ETS(
        export function Selected(): int { return 1; }
        export * from "./bridge.ets";
    )ETS");
    WriteFile("bridge.ets", R"ETS(
        export * from "./a.ets";
        export { Selected } from "./a.ets";
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { Selected } from "./bridge.ets";

        let value: int = Selected();
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, DoesNotLoopForeverOnStarCycle)
{
    WriteFile("a.ets", R"ETS(
        export * from "./main.ets";
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        export * from "./a.ets";
    )ETS");

    const auto &cache = Checker()->ResolveExportClosure(Program());

    EXPECT_FALSE(cache.Has("MissingFromCycle"));
    EXPECT_FALSE(cache.HasAmbiguous("MissingFromCycle"));
}

TEST_F(ExportResolutionTest, NamespaceExportResolvesToTargetSurface)
{
    WriteFile("source.ets", R"ETS(
        export function Member(): int { return 1; }
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        export * as SourceNamespace from "./source.ets";
    )ETS");

    const auto &cache = Checker()->ResolveExportClosure(Program());
    const auto *entry = cache.Find("SourceNamespace");

    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->variable, nullptr);
    ASSERT_NE(entry->surface.program, nullptr);
    EXPECT_TRUE(entry->surface.resolvedSource.EndsWith("/source.ets"));
}

TEST_F(ExportResolutionTest, ReExportResolutionUsesImportDeclarationIdentity)
{
    WriteFile("source.ets", R"ETS(
        export function Original(): int { return 1; }
    )ETS");
    WriteFile("bridge.ets", R"ETS(
        export { Original as Renamed } from "./source.ets";
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { Renamed } from "./bridge.ets";
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, ExplicitBadReExportReportsErrorWithoutBeingImported)
{
    WriteFile("source.ets", R"ETS(
        export function Ok(): int { return 1; }
    )ETS");
    WriteFile("bridge.ets", R"ETS(
        export { Missing as UnusedBad } from "./source.ets";
        export { Ok } from "./source.ets";
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { Ok } from "./bridge.ets";
    )ETS");

    EXPECT_TRUE(Checker()->IsAnyError());
}

TEST_F(ExportResolutionTest, RequestedBadReExportReportsError)
{
    WriteFile("source.ets", R"ETS(
        export function Ok(): int { return 1; }
    )ETS");
    WriteFile("bridge.ets", R"ETS(
        export { Missing as UnusedBad } from "./source.ets";
        export { Ok } from "./source.ets";
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { UnusedBad } from "./bridge.ets";
    )ETS");

    EXPECT_TRUE(Checker()->IsAnyError());
}

TEST_F(ExportResolutionTest, DefaultImportRequestsDefaultOnly)
{
    WriteImportMatrixFiles();

    InitializeFromFile("main.ets", R"ETS(
        import A from "./source.ets";
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, NamespaceImportOnlyLocatesSurface)
{
    WriteNamedOnlyImportMatrixFiles();

    InitializeFromFile("main.ets", R"ETS(
        import * as ns from "./source.ets";
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, NamespaceImportAllowsDefaultExportedTargetWithNamedExports)
{
    WriteImportMatrixFiles();

    InitializeFromFile("main.ets", R"ETS(
        import * as ns from "./source.ets";
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, NamespaceImportReportsDefaultOnlyTarget)
{
    WriteDefaultOnlyImportMatrixFiles();

    InitializeFromFile("main.ets", R"ETS(
        import * as ns from "./source.ets";
    )ETS");

    EXPECT_TRUE(Checker()->IsAnyError());
}

TEST_F(ExportResolutionTest, MultipleNamedImportRequestsEachName)
{
    WriteImportMatrixFiles();

    InitializeFromFile("main.ets", R"ETS(
        import { a, b } from "./source.ets";
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, AliasedNamedImportRequestsImportedName)
{
    WriteImportMatrixFiles();

    InitializeFromFile("main.ets", R"ETS(
        import { a as localA } from "./source.ets";
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, NamedDefaultImportRequestsDefault)
{
    WriteImportMatrixFiles();

    InitializeFromFile("main.ets", R"ETS(
        import { default as A } from "./source.ets";
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, DefaultAndNamedImportRequestsBothNames)
{
    WriteImportMatrixFiles();

    InitializeFromFile("main.ets", R"ETS(
        import A, { a } from "./source.ets";
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, DefaultAndNamespaceImportAllowsDefaultExportedTargetWithNamedExports)
{
    WriteImportMatrixFiles();

    InitializeFromFile("main.ets", R"ETS(
        import A, * as ns from "./source.ets";
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, TypeOnlyImportRequestsTypeName)
{
    WriteImportMatrixFiles();

    InitializeFromFile("main.ets", R"ETS(
        import type { TypeOnly } from "./source.ets";
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, TypeOnlyImportedGenericAliasContextualizesExportedMethodCallback)
{
    WriteFile("base.ets", R"ETS(
        export declare class BusinessError<T = void> extends Error {}
        export type AsyncCallback<T, E = void> = (err: BusinessError<E> | null, data: T | undefined) => void;
    )ETS");
    WriteFile("context.ets", R"ETS(
        import type { AsyncCallback } from "./base.ets";

        export declare class Context {
            getGroupDir(dataGroupID: string, callback: AsyncCallback<string>): void;
            getGroupDir(dataGroupID: string): Promise<string>;
        }
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { Context } from "./context.ets";

        let abilityContext: Context | undefined;
        abilityContext?.getGroupDir("context_hap1", (err, data) => {});
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, ImportedModuleStarExportAmbiguityReportsError)
{
    WriteFile("left.ets", R"ETS(
        export function Ok(): int { return 1; }
        export function Duplicated(): int { return 1; }
    )ETS");
    WriteFile("right.ets", R"ETS(
        export function Duplicated(): int { return 2; }
    )ETS");
    WriteFile("bridge.ets", R"ETS(
        export * from "./left.ets";
        export * from "./right.ets";
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { Ok } from "./bridge.ets";
    )ETS");

    EXPECT_TRUE(Checker()->IsAnyError());
    EXPECT_EQ(GetAnyError().Message(), "The entity named 'Duplicated' is already exported.");
}

TEST_F(ExportResolutionTest, RequestedStarExportAmbiguityUsesSurfaceDiagnostic)
{
    WriteFile("left.ets", R"ETS(
        export function Duplicated(): int { return 1; }
    )ETS");
    WriteFile("right.ets", R"ETS(
        export function Duplicated(): int { return 2; }
    )ETS");
    WriteFile("bridge.ets", R"ETS(
        export * from "./left.ets";
        export * from "./right.ets";
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { Duplicated } from "./bridge.ets";
    )ETS");

    EXPECT_TRUE(Checker()->IsAnyError());
    EXPECT_EQ(GetAnyError().Message(), "The entity named 'Duplicated' is already exported.");
}

TEST_F(ExportResolutionTest, CurrentModuleStarExportAmbiguityReportsError)
{
    WriteFile("left.ets", R"ETS(
        export function Duplicated(): int { return 1; }
    )ETS");
    WriteFile("right.ets", R"ETS(
        export function Duplicated(): int { return 2; }
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        export * from "./left.ets";
        export * from "./right.ets";
    )ETS");

    EXPECT_TRUE(Checker()->IsAnyError());
    EXPECT_EQ(GetAnyError().Message(), "The entity named 'Duplicated' is already exported.");
}

TEST_F(ExportResolutionTest, NamespaceImportDoesNotCheckUnrequestedTargetCode)
{
    WriteFile("source.ets", R"ETS(
        export function Ok(): int { return 1; }
        let bad: int = "bad";
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import * as ns from "./source.ets";
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError());
}

TEST_F(ExportResolutionTest, NamespaceImportDoesNotBindReExportedNamesLocally)
{
    WriteFile("source.ets", R"ETS(
        export function Ok(): int { return 1; }
    )ETS");
    WriteFile("bridge.ets", R"ETS(
        export { Ok } from "./source.ets";
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import * as ns from "./bridge.ets";
        let value = Ok;
    )ETS");

    EXPECT_TRUE(Checker()->IsAnyError());
}

TEST_F(ExportResolutionTest, NamespaceImportResolvesRequestedMember)
{
    WriteFile("source.ets", R"ETS(
        export function Ok(): int { return 1; }
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import * as ns from "./source.ets";
        let value: int = ns.Ok();
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, NamespaceImportReportsRequestedMissingMember)
{
    WriteFile("source.ets", R"ETS(
        export function Ok(): int { return 1; }
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import * as ns from "./source.ets";
        let value = ns.Missing;
    )ETS");

    EXPECT_TRUE(Checker()->IsAnyError());
}

TEST_F(ExportResolutionTest, NamedImportAliasResolvesQualifiedTypeBase)
{
    WriteFile("source.ets", R"ETS(
        export namespace annotation {
            export class InternalAPI {}
        }
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { annotation as annotationAlias } from "./source.ets";
        let value: annotationAlias.InternalAPI|null = null;
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, NamedImportAliasResolvesRequestedFunction)
{
    WriteFile("source.ets", R"ETS(
        export let Ok: int = 1;
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { Ok as Alias } from "./source.ets";
        let value: int = Alias;
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, NamedImportResolvesCallableFunctionBinding)
{
    WriteFile("source.ets", R"ETS(
        export function exported(): int { return 1; }
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { exported } from "./source.ets";
        let value: int = exported();
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, NamedImportNamespaceResolvesCallableMember)
{
    WriteFile("source.ets", R"ETS(
        export namespace stub {
            export function toValue(): int { return 1; }
        }
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { stub } from "./source.ets";
        let value: int = stub.toValue();
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, NamedImportResolvesDeclareNamespace)
{
    WriteFile("source.d.ets", R"ETS(
        export declare namespace ns {}
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { ns } from "./source";
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, NamedImportResolvesDeclarationClassWithBodylessConstructor)
{
    WriteFile("source.d.ets", R"ETS(
        export class ChipLike {
            constructor();
        }
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { ChipLike } from "./source";
        let value: ChipLike|null = null;
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, DefaultImportResolvesDeclarationAbstractClassWithImplicitConstructor)
{
    WriteFile("source.d.ets", R"ETS(
        export default abstract class BaseContext {
            stageMode: boolean;
        }
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import BaseContext from "./source";
        abstract class Derived extends BaseContext {}
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, DuplicateNamespaceImportResolvesDeclareAndRuntimeNamespace)
{
    WriteFile("decl/source.d.ets", R"ETS(
        export declare namespace ns {}
    )ETS");
    WriteFile("source.ets", R"ETS(
        export namespace ns {}
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { ns } from "./decl/source";
        import { ns } from "./source";
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, NamedImportClassResolvesStaticMember)
{
    WriteFile("source.ets", R"ETS(
        export class AbcFile {
            static loadAbcFile(): int { return 1; }
        }
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { AbcFile } from "./source.ets";
        let value: int = AbcFile.loadAbcFile();
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, PackageImportResolvesRequestedFractionExport)
{
    WriteFile("pkg/annotation.ets", R"ETS(
        export namespace annotation {
            export class InternalAPI {}
        }
    )ETS");
    WriteFile("pkg/stub.ets", R"ETS(
        export namespace stub {
            export class Value {}
        }
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { annotation, stub } from "./pkg";

        let value: annotation.InternalAPI|null = null;
        let stubValue: stub.Value|null = null;
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, PackageExplicitReExportOverridesStarExportFromOtherFraction)
{
    WriteFile("pkg/explicit.ets", R"ETS(
        export { A } from "../selected.ets";
    )ETS");
    WriteFile("pkg/star.ets", R"ETS(
        export * from "../generated.ets";
    )ETS");
    WriteFile("selected.ets", R"ETS(
        export class A {}
    )ETS");
    WriteFile("generated.ets", R"ETS(
        export class A {}
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { A } from "./pkg";

        let value: A|null = null;
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

TEST_F(ExportResolutionTest, ImportedSyntaxErrorStillAllowsRequestedNameError)
{
    WriteFile("ex.ets", R"ETS(
        what the fuck
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { somename } from "./ex.ets";
    )ETS");

    const auto diagnostics = Checker()->DiagnosticEngine().PrintAndFlushErrorDiagnostic();
    EXPECT_NE(diagnostics.find("Syntax error"), std::string::npos) << diagnostics;
    EXPECT_NE(diagnostics.find("Imported element not exported 'somename'"), std::string::npos) << diagnostics;
}

TEST_F(ExportResolutionTest, StarExportPropagatesLocalSelectiveExportAlias)
{
    WriteFile("source.ets", R"ETS(
        class LocalA {
            value: int = 1;
        }
        export { LocalA as A };
    )ETS");
    WriteFile("bridge.ets", R"ETS(
        export * from "./source.ets";
    )ETS");

    InitializeFromFile("main.ets", R"ETS(
        import { A } from "./bridge.ets";
        let value: A = new A();
        let field: int = value.value;
    )ETS");

    EXPECT_FALSE(Checker()->IsAnyError()) << (Checker()->IsAnyError() ? GetAnyError().Message() : std::string {});
}

}  // namespace ark::es2panda::compiler::test
