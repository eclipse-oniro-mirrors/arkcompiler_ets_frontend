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

#include <gtest/gtest.h>
#include <cstddef>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include "lsp/include/internal_api.h"
#include "lsp/include/refactors/generate_constructor.h"
#include "lsp/include/refactors/generate_override_methods.h"
#include "lsp/include/refactors/move_to_new_file.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace {
using ark::es2panda::lsp::GenerateConstructorRefactor;
using ark::es2panda::lsp::GenerateOverrideMethods;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::MoveToNewFileRefactor;
using ark::es2panda::lsp::RefactorContext;
using ark::es2panda::lsp::RefactorEditInfo;

class LSPRefactorsMoveGenerateTests : public LSPAPITests {
public:
    static RefactorContext MakeRefactorContext(es2panda_Context *ctx, TextChangesContext *textCtx,
                                               const std::string &kind, size_t pos, size_t end)
    {
        RefactorContext rc;
        rc.context = ctx;
        rc.kind = kind;
        rc.span.pos = pos;
        rc.span.end = end;
        rc.textChangesContext = textCtx;
        return rc;
    }

    static std::string ReadFileContent(const std::string &fullPath)
    {
        std::ifstream ifs(fullPath);
        if (!ifs.is_open()) {
            return "";
        }
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        return buffer.str();
    }

    std::unique_ptr<RefactorEditInfo> RunMoveToNewFile(const std::string &source, size_t pos, size_t end,
                                                       std::vector<std::string> &filePaths)
    {
        filePaths = CreateTempFile({"MoveGenerate_case.ets"}, {source});
        if (filePaths.empty()) {
            return nullptr;
        }
        Initializer init;
        es2panda_Context *ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);
        if (ctx == nullptr) {
            return nullptr;
        }
        MoveToNewFileRefactor refactor;
        ark::es2panda::lsp::FormatCodeSettings settings;
        auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
        TextChangesContext changeText {{}, formatContext, {}};
        auto refContext = MakeRefactorContext(
            ctx, &changeText, std::string(ark::es2panda::lsp::TO_MOVE_TO_NEW_FILE_ACTION.kind), pos, end);
        auto available = refactor.GetAvailableActions(refContext);
        EXPECT_FALSE(available.empty());
        std::unique_ptr<RefactorEditInfo> editInfo =
            refactor.GetEditsForAction(refContext, std::string(ark::es2panda::lsp::TO_MOVE_TO_NEW_FILE_ACTION.name));
        init.DestroyContext(ctx);
        return editInfo;
    }

    std::optional<std::string> RunGenerateConstructor(const std::string &source, size_t cursorPosition,
                                                      const std::string &fileName)
    {
        auto tempFiles = CreateTempFile({fileName}, {source});
        if (tempFiles.empty()) {
            return std::nullopt;
        }
        Initializer init;
        es2panda_Context *ctx = init.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
        if (ctx == nullptr) {
            return std::nullopt;
        }
        ark::es2panda::lsp::FormatCodeSettings settings;
        auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
        TextChangesContext changeText {{}, formatContext, {}};
        auto refContext =
            MakeRefactorContext(ctx, &changeText, std::string(ark::es2panda::lsp::TO_GENERATE_CONSTRUCTOR_ACTION.kind),
                                cursorPosition, cursorPosition);
        GenerateConstructorRefactor refactor;
        auto editInfo = refactor.GetEditsForAction(
            refContext, std::string(ark::es2panda::lsp::TO_GENERATE_CONSTRUCTOR_ACTION.name));
        init.DestroyContext(ctx);
        if (editInfo == nullptr || editInfo->GetFileTextChanges().empty() ||
            editInfo->GetFileTextChanges().at(0).textChanges.empty()) {
            return std::nullopt;
        }
        return std::string(editInfo->GetFileTextChanges().at(0).textChanges.at(0).newText);
    }

    std::optional<std::string> RunGenerateOverrideMethods(const std::string &source, size_t cursorPosition,
                                                          const std::string &fileName)
    {
        auto tempFiles = CreateTempFile({fileName}, {source});
        if (tempFiles.empty()) {
            return std::nullopt;
        }
        Initializer init;
        es2panda_Context *ctx = init.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
        if (ctx == nullptr) {
            return std::nullopt;
        }
        ark::es2panda::lsp::FormatCodeSettings settings;
        auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
        TextChangesContext changeText {{}, formatContext, {}};
        auto refContext = MakeRefactorContext(ctx, &changeText,
                                              std::string(ark::es2panda::lsp::TO_GENERATE_OVERRIDE_METHODS_ACTION.kind),
                                              cursorPosition, cursorPosition);
        GenerateOverrideMethods refactor;
        auto editInfo = refactor.GetEditsForAction(
            refContext, std::string(ark::es2panda::lsp::TO_GENERATE_OVERRIDE_METHODS_ACTION.name));
        init.DestroyContext(ctx);
        if (editInfo == nullptr || editInfo->GetFileTextChanges().empty() ||
            editInfo->GetFileTextChanges().at(0).textChanges.empty()) {
            return std::nullopt;
        }
        return std::string(editInfo->GetFileTextChanges().at(0).textChanges.at(0).newText);
    }
};

// A moved statement is re-emitted with an "export " prefix when its name is referenced
// anywhere else in the old file, so the new file keeps the declaration visible.
TEST_F(LSPRefactorsMoveGenerateTests, MoveToNewFile_UsedInOldFile_IsExported)
{
    const std::string src = R"(function MoveGenerate_Moved(): void {
}
console.log(1);
)";
    const size_t pos = 10;
    const size_t end = 38;
    std::vector<std::string> filePaths;
    auto editInfo = RunMoveToNewFile(src, pos, end, filePaths);
    ASSERT_NE(editInfo, nullptr);
    ASSERT_FALSE(filePaths.empty());
    const std::string newFilePath = fs::path(filePaths[0]).parent_path().string() + "/MoveGenerate_Moved.ets";
    ASSERT_TRUE(fs::exists(newFilePath));
    EXPECT_EQ(ReadFileContent(newFilePath), "export function MoveGenerate_Moved(): void {\n}\n");
}

// A statement with a default modifier keeps the "default" keyword in the moved text.
TEST_F(LSPRefactorsMoveGenerateTests, DISABLED_MoveToNewFile_DefaultExportModifier_IsMovedVerbatim)
{
    const std::string src = R"(export default function MoveGenerate_Defaulted(): void {
}
console.log(1);
)";
    const size_t pos = 20;
    const size_t end = 57;
    std::vector<std::string> filePaths;
    auto editInfo = RunMoveToNewFile(src, pos, end, filePaths);
    ASSERT_NE(editInfo, nullptr);
    ASSERT_FALSE(filePaths.empty());
    const std::string newFilePath = fs::path(filePaths[0]).parent_path().string() + "/MoveGenerate_Defaulted.ets";
    ASSERT_TRUE(fs::exists(newFilePath));
    EXPECT_EQ(ReadFileContent(newFilePath), "export default function MoveGenerate_Defaulted(): void {\n}\n");
}

// An export-from (re-export) declaration is not an import declaration, so it is not
// copied into the new file when an unrelated statement is moved.
TEST_F(LSPRefactorsMoveGenerateTests, MoveToNewFile_ReExport_StaysInOldFile)
{
    const std::string src = R"(export { X } from './other';
function MoveGenerate_ReexportTarget(): void {
}
)";
    const size_t pos = 40;
    const size_t end = 100;
    std::vector<std::string> filePaths;
    auto editInfo = RunMoveToNewFile(src, pos, end, filePaths);
    ASSERT_NE(editInfo, nullptr);
    ASSERT_FALSE(filePaths.empty());
    const std::string newFilePath = fs::path(filePaths[0]).parent_path().string() + "/MoveGenerate_ReexportTarget.ets";
    ASSERT_TRUE(fs::exists(newFilePath));
    EXPECT_EQ(ReadFileContent(newFilePath), "export function MoveGenerate_ReexportTarget(): void {\n}\n");
}

// If the target file already exists, the refactor picks a unique name with a numeric suffix.
TEST_F(LSPRefactorsMoveGenerateTests, MoveToNewFile_ExistingTargetFile_UsesUniqueName)
{
    const std::string src = R"(function MoveGenerate_Existing(): void {
}
console.log(1);
)";
    const size_t pos = 10;
    const size_t end = 41;
    std::vector<std::string> filePaths = CreateTempFile({"MoveGenerate_existing_case.ets"}, {src});
    ASSERT_FALSE(filePaths.empty());
    const std::string directory = fs::path(filePaths[0]).parent_path().string();
    const std::string firstPath = directory + "/MoveGenerate_Existing.ets";
    const std::string secondPath = directory + "/MoveGenerate_Existing_1.ets";
    {
        std::ofstream preCreated(firstPath);
        preCreated << "// pre-existing file\n";
    }
    ASSERT_TRUE(fs::exists(firstPath));

    Initializer init;
    es2panda_Context *ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    MoveToNewFileRefactor refactor;
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    auto refContext = MakeRefactorContext(ctx, &changeText,
                                          std::string(ark::es2panda::lsp::TO_MOVE_TO_NEW_FILE_ACTION.kind), pos, end);
    auto available = refactor.GetAvailableActions(refContext);
    ASSERT_FALSE(available.empty());
    auto editInfo =
        refactor.GetEditsForAction(refContext, std::string(ark::es2panda::lsp::TO_MOVE_TO_NEW_FILE_ACTION.name));
    ASSERT_NE(editInfo, nullptr);
    init.DestroyContext(ctx);

    EXPECT_EQ(ReadFileContent(firstPath), "// pre-existing file\n");
    ASSERT_TRUE(fs::exists(secondPath));
    EXPECT_EQ(ReadFileContent(secondPath), "export function MoveGenerate_Existing(): void {\n}\n");
}

// Generating a constructor for a class that already has one merges the missing field
// parameter and assignment into that constructor instead of creating an illegal overload.
TEST_F(LSPRefactorsMoveGenerateTests, DISABLED_GenerateConstructor_MergesIntoExistingConstructor)
{
    const std::string src = R"(class MoveGenerate_Point {
    constructor() {
    }
    x: number
}
)";
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr std::string_view expected = "constructor(x: number) {\n        this.x = x\n    }";
    const size_t pos = src.find("x: number");
    auto result = RunGenerateConstructor(src, pos, "GenerateConstructor_WhenConstructorExists.ets");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, expected);
}

// Readonly instance fields are collected; static fields are skipped.
TEST_F(LSPRefactorsMoveGenerateTests, GenerateConstructor_ReadonlyFieldIncluded_StaticFieldSkipped)
{
    const std::string src = R"(class MoveGenerate_Config {
    readonly name: string
    static version: number
}
)";
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr std::string_view expected =
        "\n    constructor(name: string) {\n"
        "        this.name = name\n"
        "    }\n";
    const size_t pos = src.find("readonly name");
    auto result = RunGenerateConstructor(src, pos, "GenerateConstructor_ReadonlyField.ets");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, expected);
}

// Optional instance fields are skipped; only required fields are collected.
TEST_F(LSPRefactorsMoveGenerateTests, GenerateConstructor_OptionalFieldSkipped)
{
    const std::string src = R"(class MoveGenerate_Profile {
    nickname?: string
    age: number
}
)";
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr std::string_view expected =
        "\n    constructor(age: number) {\n"
        "        this.age = age\n"
        "    }\n";
    const size_t pos = src.find("nickname");
    auto result = RunGenerateConstructor(src, pos, "GenerateConstructor_OptionalField.ets");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, expected);
}

// Inherited fields are not collected when the base class has no constructor.
TEST_F(LSPRefactorsMoveGenerateTests, GenerateConstructor_InheritedFieldsNotIncluded)
{
    const std::string src = R"(class MoveGenerate_Base {
    baseField: number
}
class MoveGenerate_Child extends MoveGenerate_Base {
    ownField: string
}
)";
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr std::string_view expected =
        "\n    constructor(ownField: string) {\n"
        "        super()\n"
        "        this.ownField = ownField\n"
        "    }\n";
    const size_t pos = src.find("ownField");
    auto result = RunGenerateConstructor(src, pos, "GenerateConstructor_InheritedFields.ets");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, expected);
}

// The refactor is not applicable when the class has no collectible fields and no
// superclass constructor parameters.
TEST_F(LSPRefactorsMoveGenerateTests, GenerateConstructor_NotApplicable_WhenNothingToGenerate)
{
    const std::string src = R"(class MoveGenerate_Empty {
}
)";
    auto tempFiles = CreateTempFile({"GenerateConstructor_NotApplicable.ets"}, {src});
    ASSERT_FALSE(tempFiles.empty());
    Initializer init;
    es2panda_Context *ctx = init.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext changeText {{}, formatContext, {}};
    const size_t pos = src.find("MoveGenerate_Empty");
    auto refContext = MakeRefactorContext(
        ctx, &changeText, std::string(ark::es2panda::lsp::TO_GENERATE_CONSTRUCTOR_ACTION.kind), pos, pos);
    GenerateConstructorRefactor refactor;
    auto available = refactor.GetAvailableActions(refContext);
    init.DestroyContext(ctx);
    EXPECT_TRUE(available.empty());
}

// Interface methods implemented by a class get empty bodies without override modifier.
TEST_F(LSPRefactorsMoveGenerateTests, GenerateOverride_ImplementsInterface_GeneratesEmptyBodies)
{
    const std::string src = R"(interface MoveGenerate_Shape {
    area(): number
    describe(): string
}
class MoveGenerate_Square implements MoveGenerate_Shape {
}
)";
    const std::string expect = "\n    area() : number {\n\n}\n\n    describe() : string {\n\n}\n\n";
    const size_t pos = src.find("MoveGenerate_Square implements");
    auto result = RunGenerateOverrideMethods(src, pos, "GenerateOverride_ImplementsInterface.ets");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, expect);
}

// Abstract class methods are overridden with super calls, while methods already
// defined in the derived class are skipped.
TEST_F(LSPRefactorsMoveGenerateTests, GenerateOverride_AbstractClass_SkipsAlreadyImplemented)
{
    const std::string src = R"(abstract class MoveGenerate_Animal {
    abstract sound(): void
    name(): string {
        return 'animal';
    }
}
class MoveGenerate_Dog extends MoveGenerate_Animal {
    sound(): void {
    }
}
)";
    const std::string expect = "\n    public override name() : string {\n        return super.name();\n    }\n\n";
    const size_t pos = src.find("MoveGenerate_Dog extends");
    auto result = RunGenerateOverrideMethods(src, pos, "GenerateOverride_AbstractClass.ets");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, expect);
}

// Interfaces extending multiple interfaces collect methods from all super interfaces.
TEST_F(LSPRefactorsMoveGenerateTests, GenerateOverride_InterfaceExtendsMultiple_GeneratesAllSuperMethods)
{
    const std::string src = R"(interface MoveGenerate_A1 {
    foo(): void
}
interface MoveGenerate_B1 {
    bar(p: number): string
}
interface MoveGenerate_C1 extends MoveGenerate_A1, MoveGenerate_B1 {
}
)";
    const std::string expect = "\n    foo() : void {\n\n}\n\n    bar(p: number) : string {\n\n}\n\n";
    const size_t pos = src.find("MoveGenerate_C1 extends");
    auto result = RunGenerateOverrideMethods(src, pos, "GenerateOverride_InterfaceExtendsMultiple.ets");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, expect);
}

}  // namespace
