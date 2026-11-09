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

#include "gtest/gtest.h"
#include "../lsp_api_test.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "lsp/include/cancellation_token.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/symbol_reference_index.h"
#include "util/arktsconfig.h"
#include "util/diagnosticEngine.h"

namespace {
using ark::es2panda::lsp::Initializer;

class ScopedTestContext {
public:
    ScopedTestContext(const std::string &file, const std::string &source, const std::string &configPath)
    {
        impl_ = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
        if (impl_ == nullptr) {
            return;
        }
        const auto configOption = "--arktsconfig=" + configPath;
        std::array<const char *, 2U> args = {file.c_str(), configOption.c_str()};
        config_ = impl_->CreateConfig(args.size(), args.data());
        if (config_ == nullptr) {
            return;
        }
        context_ = source.empty() ? impl_->CreateContextFromFile(config_, file.c_str())
                                  : impl_->CreateContextFromString(config_, source.c_str(), file.c_str());
        if (context_ != nullptr) {
            impl_->ProceedToState(context_, ES2PANDA_STATE_BOUND);
        }
    }

    ~ScopedTestContext()
    {
        if (impl_ == nullptr) {
            return;
        }
        if (context_ != nullptr) {
            impl_->DestroyContext(context_);
        }
        if (config_ != nullptr) {
            impl_->DestroyConfigWithoutLog(config_);
        }
    }

    ScopedTestContext(const ScopedTestContext &) = delete;
    ScopedTestContext &operator=(const ScopedTestContext &) = delete;
    ScopedTestContext(ScopedTestContext &&) = delete;
    ScopedTestContext &operator=(ScopedTestContext &&) = delete;

    es2panda_Context *Get() const
    {
        return context_;
    }

private:
    const es2panda_Impl *impl_ {};
    es2panda_Config *config_ {};
    es2panda_Context *context_ {};
};

constexpr std::string_view IMPORT_FIXES_ID = "ImportFixes";
constexpr int UNRESOLVED_ERROR_CODE = 1005;
constexpr int DEFAULT_THROTTLE = 20;

class ComputeRelativeImportPathTests : public LSPAPITests {
public:
    void SetUp() override
    {
        LSPAPITests::SetUp();
        ark::es2panda::lsp::ClearSymbolReferenceIndex();
    }

    void TearDown() override
    {
        ark::es2panda::lsp::ClearSymbolReferenceIndex();
    }

    std::filesystem::path TempBasePath()
    {
        std::filesystem::path basePath = testing::TempDir();
        basePath.append(GetExecutableName());
        if (!tempBaseRegistered_) {
            tempFiles_.push_back(basePath);
            tempBaseRegistered_ = true;
        }
        return basePath;
    }

    std::string CreateNestedTempFile(const std::string &relativePath, const std::string &text)
    {
        auto filePath = TempBasePath().append(relativePath);
        std::filesystem::create_directories(filePath.parent_path());
        std::ofstream outStream(filePath);
        if (outStream.fail()) {
            return "";
        }
        outStream << text;
        return filePath.string();
    }

    static std::string MakePathsConfig(const std::string &baseUrl,
                                       const std::vector<std::pair<std::string, std::string>> &paths)
    {
        std::string configText = R"({"compilerOptions": {"baseUrl": ")" + baseUrl + R"(", "paths": {)";
        for (size_t i = 0; i < paths.size(); ++i) {
            if (i != 0U) {
                configText += ", ";
            }
            configText += "\"" + paths[i].first + "\": [\"" + paths[i].second + "\"]";
        }
        configText += "}}}";
        return configText;
    }

    ark::es2panda::ArkTsConfig ParsePathsConfig(const std::string &configText)
    {
        auto configPath = CreateNestedTempFile("aimp_paths_config.json", configText);
        ark::es2panda::ArkTsConfig config(configPath, diagnosticEngine_);
        EXPECT_TRUE(config.Parse());
        return config;
    }

    static std::string MakeDependenciesConfig(
        const std::string &baseUrl, const std::vector<std::tuple<std::string, std::string, std::string>> &dependencies)
    {
        std::string configText = R"({"compilerOptions": {"baseUrl": ")" + baseUrl + R"(", "dependencies": {)";
        for (size_t i = 0; i < dependencies.size(); ++i) {
            if (i != 0U) {
                configText += ", ";
            }
            const auto &sourceFilePath = std::get<2>(dependencies[i]);
            configText += "\"" + std::get<0>(dependencies[i]) + "\": {\"language\": \"js\", \"path\": \"" +
                          std::get<1>(dependencies[i]) + "\"" +
                          (sourceFilePath.empty() ? "" : ", \"sourceFilePath\": \"" + sourceFilePath + "\"") +
                          ", \"ohmUrl\": \"\"}";
        }
        configText += "}}}";
        return configText;
    }

    static void BuildSymbolIndex(Initializer &initializer, const std::string &filePath)
    {
        auto *context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);
        ASSERT_NE(context, nullptr);
        ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));
        initializer.DestroyContext(context);
    }

    static void BuildSymbolIndexWithConfig(const std::string &filePath, const std::string &configPath)
    {
        auto *compilerImpl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
        ASSERT_NE(compilerImpl, nullptr);
        const auto configOption = "--arktsconfig=" + configPath;
        std::array<const char *, 2U> args = {filePath.c_str(), configOption.c_str()};
        auto *config = compilerImpl->CreateConfig(args.size(), args.data());
        ASSERT_NE(config, nullptr);
        auto *context = compilerImpl->CreateContextFromFile(config, filePath.c_str());
        ASSERT_NE(context, nullptr);
        compilerImpl->ProceedToState(context, ES2PANDA_STATE_BOUND);
        EXPECT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));
        compilerImpl->DestroyContext(context);
        compilerImpl->DestroyConfigWithoutLog(config);
    }

    static std::vector<CodeFixActionInfo> GetImportFixes(es2panda_Context *context, const std::string &source,
                                                         std::string_view unresolvedName)
    {
        const auto position = source.find(unresolvedName);
        EXPECT_NE(position, std::string::npos);
        std::vector<int> errorCodes {UNRESOLVED_ERROR_CODE};
        CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
        return ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, position, position + unresolvedName.size(),
                                                              errorCodes, options);
    }

    static const CodeFixActionInfo *FindImportFix(const std::vector<CodeFixActionInfo> &fixes)
    {
        for (const auto &fix : fixes) {
            if (fix.fixName_ == IMPORT_FIXES_ID) {
                return &fix;
            }
        }
        return nullptr;
    }

    static std::vector<std::string> GetImportFixDescriptions(const std::vector<CodeFixActionInfo> &fixes)
    {
        std::vector<std::string> descriptions;
        for (const auto &fix : fixes) {
            if (fix.fixName_ == IMPORT_FIXES_ID) {
                descriptions.push_back(fix.description_);
            }
        }
        return descriptions;
    }

private:
    class NullCancellationToken : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return false;
        }
    };

    static NullCancellationToken &GetNullHost()
    {
        static NullCancellationToken instance;
        return instance;
    }

    static ark::es2panda::lsp::CancellationToken CreateToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    bool tempBaseRegistered_ = false;
    ark::es2panda::util::DiagnosticEngine diagnosticEngine_;
};

// A definition located in another module mapped by 'paths' is imported through the module
// alias: the module entry file keeps the bare alias, an inner file keeps its path under
// the module root - the resolver maps the bare alias onto the entry file only and does not
// aggregate the inner files' exports, so the sub-path form keeps the import resolvable.
TEST_F(ComputeRelativeImportPathTests, CrossModuleFilesMapToModuleName)
{
    const auto basePath = TempBasePath().string();
    const auto entryPage = basePath + "/entry/src/main/ets/pages/AimpPage.ets";
    const auto libraryIndex = basePath + "/library/src/main/ets/Index.ets";
    const auto libraryFoo = basePath + "/library/src/main/ets/utils/AimpFoo.ets";
    auto config = ParsePathsConfig(MakePathsConfig(
        basePath, {{"entry", basePath + "/entry/src/main/ets"}, {"library", basePath + "/library/src/main/ets"}}));

    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, libraryIndex, &config), "library");
    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, libraryFoo, &config), "library/utils/AimpFoo");
}

// Definitions from the importing file's own module keep the relative import form.
TEST_F(ComputeRelativeImportPathTests, SameModuleFileKeepsRelativePath)
{
    const auto basePath = TempBasePath().string();
    const auto entryPage = basePath + "/entry/src/main/ets/pages/AimpPage.ets";
    const auto entryFoo = basePath + "/entry/src/main/ets/utils/AimpFoo.ets";
    auto config = ParsePathsConfig(MakePathsConfig(
        basePath, {{"entry", basePath + "/entry/src/main/ets"}, {"library", basePath + "/library/src/main/ets"}}));

    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, entryFoo, &config), "../utils/AimpFoo");
    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, entryFoo), "../utils/AimpFoo");
}

// Files not covered by any 'paths' alias, or a missing config, fall back to relative paths.
TEST_F(ComputeRelativeImportPathTests, UnmappedFileFallsBackToRelativePath)
{
    const auto basePath = TempBasePath().string();
    const auto entryPage = basePath + "/entry/src/main/ets/pages/AimpPage.ets";
    const auto outsideFile = basePath + "/shared/AimpOutside.ets";
    auto config = ParsePathsConfig(MakePathsConfig(basePath, {{"library", basePath + "/library/src/main/ets"}}));

    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, outsideFile, &config),
              "../../../../../shared/AimpOutside");
    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, outsideFile),
              "../../../../../shared/AimpOutside");
}

// Per-file targets map through their key without the file extension, both when the target
// carries the extension and when it is already extension-stripped (SDK declaration style).
TEST_F(ComputeRelativeImportPathTests, ExactFileTargetMapsToAliasWithoutExtension)
{
    const auto basePath = TempBasePath().string();
    const auto entryPage = basePath + "/entry/src/main/ets/pages/AimpPage.ets";
    const auto featureFile = basePath + "/AimpFeature.ets";
    const auto sdkDeclFile = basePath + "/sdk/AimpSdkApi.d.ets";
    auto config = ParsePathsConfig(
        MakePathsConfig(basePath, {{"feature", featureFile}, {"AimpSdkApi", basePath + "/sdk/AimpSdkApi"}}));

    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, featureFile, &config), "feature");
    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, sdkDeclFile, &config), "AimpSdkApi");
}

// A longer per-file alias wins over the module-directory alias for the same file; per-file
// keys (hybrid static files) resolve through their full key.
TEST_F(ComputeRelativeImportPathTests, PerFileAliasPreferredOverDirectoryAlias)
{
    const auto basePath = TempBasePath().string();
    const auto entryPage = basePath + "/entry/src/main/ets/pages/AimpPage.ets";
    const auto libraryFoo = basePath + "/library/src/main/ets/utils/AimpFoo.ets";
    auto config = ParsePathsConfig(MakePathsConfig(basePath, {{"library", basePath + "/library/src/main/ets"},
                                                              {"library/src/main/ets/utils/AimpFoo", libraryFoo}}));

    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, libraryFoo, &config),
              "library/src/main/ets/utils/AimpFoo");
}

// A per-file 'paths' alias ending in 'Index' keeps its key: the parent alias may map to a
// different entry file (a custom entry), so the file name alone must not normalize it.
TEST_F(ComputeRelativeImportPathTests, PerFileIndexAliasKeepsFileIdentity)
{
    const auto basePath = TempBasePath().string();
    const auto entryPage = basePath + "/entry/Page.ets";
    const auto mainFile = basePath + "/library/Main.ets";
    const auto indexFile = basePath + "/library/Index.ets";
    auto config = ParsePathsConfig(MakePathsConfig(basePath, {{"library", mainFile}, {"library/Index", indexFile}}));

    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, mainFile, &config), "library");
    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, indexFile, &config), "library/Index");
}

// Definitions of a dynamic module resolve through their canonical file-level dependency
// key, both when they are indexed from the module source file and from its declaration
// file: only the module entry (registered under the bare package name) normalizes to the
// package name, inner files keep their file identity ('dynlib/Utils'), so same-name
// exports of sibling files stay separate symbols. The referenced files must exist: the
// config parser resolves their paths through realpath.
TEST_F(ComputeRelativeImportPathTests, DynamicDependencyFilesMapToModuleName)
{
    const auto basePath = TempBasePath();
    ASSERT_FALSE(CreateNestedTempFile("dynlib/Index.ets", "export { AimpUtil } from './Utils';\n").empty());
    ASSERT_FALSE(CreateNestedTempFile("dynlib/Utils.ets", "export const AimpUtil: number = 1;\n").empty());
    ASSERT_FALSE(CreateNestedTempFile("decls/dynlib/Index.d.ets", "export const AimpUtil: number;\n").empty());
    ASSERT_FALSE(CreateNestedTempFile("decls/dynlib/Utils.d.ets", "export const AimpUtil: number;\n").empty());

    const auto canonical = std::filesystem::weakly_canonical(basePath).string();
    const auto entryPage = canonical + "/entry/src/main/ets/pages/AimpPage.ets";
    const auto dynIndex = canonical + "/dynlib/Index.ets";
    const auto dynUtils = canonical + "/dynlib/Utils.ets";
    const auto dynIndexDecl = canonical + "/decls/dynlib/Index.d.ets";
    const auto dynUtilsDecl = canonical + "/decls/dynlib/Utils.d.ets";
    auto config = ParsePathsConfig(MakeDependenciesConfig(canonical, {{"dynlib/Index", dynIndexDecl, dynIndex},
                                                                      {"dynlib", dynIndexDecl, dynIndex},
                                                                      {"dynlib/Utils", dynUtilsDecl, dynUtils}}));

    // The module entry keeps the bare package name, from the source file and the
    // declaration file alike.
    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, dynIndex, &config), "dynlib");
    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, dynIndexDecl, &config), "dynlib");
    // An inner file keeps its file-level identity.
    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, dynUtils, &config), "dynlib/Utils");
    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, dynUtilsDecl, &config), "dynlib/Utils");
}

// Two sibling files of one dynamic package each export the same class name ('Item'); the
// module symbol keys keep the file-level identity ('dynlib/A', 'dynlib/B'), so the two
// independent symbols stay separate: the references of one file's Item never include the
// other file's declaration or references, and an importer of one sub-path does not
// aggregate with the other file's symbol.
TEST_F(ComputeRelativeImportPathTests, DynamicSiblingSameNameSymbolsKeepSeparateKeys)
{
    const auto basePath = TempBasePath();
    const std::string fileAText = "export class Item {\n    a: number = 1;\n}\n";
    const std::string fileBText = "export class Item {\n    b: number = 2;\n}\n";
    const std::string declAText = "declare class Item {\n    a: number;\n}\nexport { Item };\n";
    const std::string declBText = "declare class Item {\n    b: number;\n}\nexport { Item };\n";
    const std::string importerText = "import { Item } from 'dynlib/A';\n\nlet value = new Item();\n";
    ASSERT_FALSE(CreateNestedTempFile("dynlib/A.ets", fileAText).empty());
    ASSERT_FALSE(CreateNestedTempFile("dynlib/B.ets", fileBText).empty());
    ASSERT_FALSE(CreateNestedTempFile("decl/A.d.ets", declAText).empty());
    ASSERT_FALSE(CreateNestedTempFile("decl/B.d.ets", declBText).empty());
    ASSERT_FALSE(CreateNestedTempFile("entry/Page.ets", importerText).empty());

    const auto canonical = std::filesystem::weakly_canonical(basePath).string();
    const auto fileA = canonical + "/dynlib/A.ets";
    const auto fileB = canonical + "/dynlib/B.ets";
    const auto declA = canonical + "/decl/A.d.ets";
    const auto declB = canonical + "/decl/B.d.ets";
    const auto importer = canonical + "/entry/Page.ets";
    const auto configPath = (TempBasePath() / "aimp_paths_config.json").string();
    auto config =
        ParsePathsConfig(MakeDependenciesConfig(canonical, {{"dynlib/A", declA, fileA}, {"dynlib/B", declB, fileB}}));

    // The file-level identity is preserved on both keys of each file (declaration and
    // source); only an entry key would normalize to the bare package name.
    EXPECT_EQ(ark::es2panda::lsp::FindDynamicDeclModuleKey(declA, config), "dynlib/A");
    EXPECT_EQ(ark::es2panda::lsp::FindDynamicDeclModuleKey(declB, config), "dynlib/B");
    EXPECT_EQ(ark::es2panda::lsp::FindDynamicDeclModuleKey(fileA, config), "dynlib/A");
    EXPECT_EQ(ark::es2panda::lsp::FindDynamicDeclModuleKey(fileB, config), "dynlib/B");
    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(importer, fileA, &config), "dynlib/A");
    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(importer, fileB, &config), "dynlib/B");

    // The symbol index keeps the two Item symbols separate: the reference list of A's
    // Item covers only A and the sub-path importer, never B's independent declaration.
    // The contexts carry the config so both sides of a module key canonicalize alike.
    BuildSymbolIndexWithConfig(fileA, configPath);
    BuildSymbolIndexWithConfig(fileB, configPath);
    BuildSymbolIndexWithConfig(importer, configPath);

    Initializer initializer;

    auto *contextOfA = initializer.CreateContext(fileA.c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(contextOfA, nullptr);
    auto refs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(contextOfA, fileAText.find("Item"));
    initializer.DestroyContext(contextOfA);

    EXPECT_EQ(
        std::count_if(refs.referenceInfos.begin(), refs.referenceInfos.end(),
                      [&fileB, &declB](const auto &ref) { return ref.fileName == fileB || ref.fileName == declB; }),
        0U);
    const auto countInA = std::count_if(refs.referenceInfos.begin(), refs.referenceInfos.end(),
                                        [&fileA](const auto &ref) { return ref.fileName == fileA; });
    const auto countInImporter = std::count_if(refs.referenceInfos.begin(), refs.referenceInfos.end(),
                                               [&importer](const auto &ref) { return ref.fileName == importer; });
    // The declaration occurrence in A itself and both occurrences of the importer.
    EXPECT_EQ(countInA, 1U);
    EXPECT_EQ(countInImporter, 2U);
}

// A custom module entry ('Main.ets') and an independent 'Index.ets' are different files: the
// Index file keeps its file-level key even though its name matches the default entry segment.
// Only the dependency key recorded for the package alias may normalize to the bare package
// name, so the two same-named exports stay separate symbols.
TEST_F(ComputeRelativeImportPathTests, DynamicCustomEntryKeepsIndependentIndexFileIdentity)
{
    const auto basePath = TempBasePath();
    const std::string mainText = "export class Item {\n    main: number = 1;\n}\n";
    const std::string indexText = "export class Item {\n    index: number = 2;\n}\n";
    const std::string importerText = "import { Item } from 'dynlib/Index';\n\nlet value = new Item();\n";
    ASSERT_FALSE(CreateNestedTempFile("dynlib/Main.ets", mainText).empty());
    ASSERT_FALSE(CreateNestedTempFile("dynlib/Index.ets", indexText).empty());
    ASSERT_FALSE(
        CreateNestedTempFile("decl/Main.d.ets", "declare class Item {\n    main: number;\n}\nexport { Item };\n")
            .empty());
    ASSERT_FALSE(
        CreateNestedTempFile("decl/Index.d.ets", "declare class Item {\n    index: number;\n}\nexport { Item };\n")
            .empty());
    ASSERT_FALSE(CreateNestedTempFile("entry/Page.ets", importerText).empty());

    const auto canonical = std::filesystem::weakly_canonical(basePath).string();
    const auto mainFile = canonical + "/dynlib/Main.ets";
    const auto indexFile = canonical + "/dynlib/Index.ets";
    const auto mainDecl = canonical + "/decl/Main.d.ets";
    const auto indexDecl = canonical + "/decl/Index.d.ets";
    const auto entryPage = canonical + "/entry/Page.ets";
    const auto configPath = (TempBasePath() / "aimp_paths_config.json").string();
    auto config = ParsePathsConfig(
        MakeDependenciesConfig(canonical, {{"dynlib", mainDecl, mainFile}, {"dynlib/Index", indexDecl, indexFile}}));

    // The recorded entry alias normalizes to the package name; the independent Index file
    // keeps its file-level identity.
    EXPECT_EQ(ark::es2panda::lsp::FindDynamicDeclModuleKey(mainDecl, config), "dynlib");
    EXPECT_EQ(ark::es2panda::lsp::FindDynamicDeclModuleKey(mainFile, config), "dynlib");
    EXPECT_EQ(ark::es2panda::lsp::FindDynamicDeclModuleKey(indexDecl, config), "dynlib/Index");
    EXPECT_EQ(ark::es2panda::lsp::FindDynamicDeclModuleKey(indexFile, config), "dynlib/Index");
    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, indexFile, &config), "dynlib/Index");
    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, indexDecl, &config), "dynlib/Index");

    // The references of Main's Item stay inside Main; the Index file and its importer never
    // aggregate with the custom entry.
    BuildSymbolIndexWithConfig(mainFile, configPath);
    BuildSymbolIndexWithConfig(indexFile, configPath);
    BuildSymbolIndexWithConfig(entryPage, configPath);

    Initializer initializer;
    auto *contextOfMain = initializer.CreateContext(mainFile.c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(contextOfMain, nullptr);
    const auto itemPos = mainText.find("Item");
    ASSERT_NE(itemPos, std::string::npos);
    auto refs = ark::es2panda::lsp::GetReferencesAtPositionFromIndex(contextOfMain, itemPos);
    initializer.DestroyContext(contextOfMain);

    EXPECT_EQ(refs.definitionInfo.fileName, mainFile);
    for (const auto &refInfo : refs.referenceInfos) {
        EXPECT_NE(refInfo.fileName, indexFile);
        EXPECT_NE(refInfo.fileName, indexDecl);
        EXPECT_NE(refInfo.fileName, entryPage);
    }
}

// A static file of a hybrid module auto-imports a dynamic sibling: same module, so the
// relative import form is kept even though the files are mapped by different config sections
// ('paths' per-file alias vs 'dependencies' source file).
TEST_F(ComputeRelativeImportPathTests, HybridModuleStaticToDynamicSiblingKeepsRelativePath)
{
    const auto basePath = TempBasePath();
    ASSERT_FALSE(
        CreateNestedTempFile("entry/src/main/ets/AimpTest.ets", "'use static'\nexport const AimpVal = 1;\n").empty());
    ASSERT_FALSE(CreateNestedTempFile("entry/src/main/ets/AimpDynTest.ets", "export const AimpDynVal = 2;\n").empty());
    ASSERT_FALSE(CreateNestedTempFile("decls/entry/AimpDynTest.d.ets", "export const AimpDynVal: number;\n").empty());

    const auto canonical = std::filesystem::weakly_canonical(basePath).string();
    const auto staticFile = canonical + "/entry/src/main/ets/AimpTest.ets";
    const auto dynamicFile = canonical + "/entry/src/main/ets/AimpDynTest.ets";
    // Hybrid module config: the static file has a 'paths' per-file alias, the dynamic file a
    // 'dependencies' entry with its source file.
    std::string configText = R"({"compilerOptions": {"baseUrl": ")" + canonical +
                             R"(", "paths": {"entry/src/main/ets/AimpTest": [")" + staticFile +
                             R"("]}, "dependencies": {"entry/src/main/ets/AimpDynTest": {"language": "js", "path": ")" +
                             canonical + "/decls/entry/AimpDynTest.d.ets" + R"(", "sourceFilePath": ")" + dynamicFile +
                             R"(", "ohmUrl": ""}}}})";
    auto config = ParsePathsConfig(configText);

    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(staticFile, dynamicFile, &config), "./AimpDynTest");
    // The reverse direction (dynamic importing a static sibling) behaves the same.
    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(dynamicFile, staticFile, &config), "./AimpTest");
}

// Definitions from the importing file's own dynamic module keep the relative import form and
// the definition file path in the 'importDeclaration' payload.
TEST_F(ComputeRelativeImportPathTests, DynamicDependencySameModuleKeepsRelativePath)
{
    const auto basePath = TempBasePath();
    ASSERT_FALSE(CreateNestedTempFile("dynlib/Index.ets", "export { AimpUtil } from './Utils';\n").empty());
    ASSERT_FALSE(CreateNestedTempFile("dynlib/Utils.ets", "export const AimpUtil: number = 1;\n").empty());
    ASSERT_FALSE(CreateNestedTempFile("decls/dynlib/Index.d.ets", "export const AimpUtil: number;\n").empty());
    ASSERT_FALSE(CreateNestedTempFile("decls/dynlib/Utils.d.ets", "export const AimpUtil: number;\n").empty());

    const auto canonical = std::filesystem::weakly_canonical(basePath).string();
    const auto dynIndex = canonical + "/dynlib/Index.ets";
    const auto dynUtils = canonical + "/dynlib/Utils.ets";
    auto config = ParsePathsConfig(
        MakeDependenciesConfig(canonical, {{"dynlib/Index", canonical + "/decls/dynlib/Index.d.ets", dynIndex},
                                           {"dynlib/Utils", canonical + "/decls/dynlib/Utils.d.ets", dynUtils}}));

    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(dynUtils, dynIndex, &config), "./Index");
}

// Interop SDK declarations (dependencies without a source file) resolve to their dynamic
// module name: the 'dependencies' entry is the authoritative dynamic classification, so
// imports reference the importable 'dynamic@xxx' specifier instead of a relative path to
// the declaration file.
TEST_F(ComputeRelativeImportPathTests, DependencyWithoutSourceFileMapsToDynamicModule)
{
    const auto basePath = TempBasePath();
    ASSERT_FALSE(CreateNestedTempFile("api/@ohos.demo.d.ets", "declare namespace demo {}\n").empty());

    const auto canonical = std::filesystem::weakly_canonical(basePath).string();
    const auto entryPage = canonical + "/entry/src/main/ets/pages/AimpPage.ets";
    const auto sdkDeclFile = canonical + "/api/@ohos.demo.d.ets";
    auto config = ParsePathsConfig(MakeDependenciesConfig(canonical, {{"dynamic@ohos.demo", sdkDeclFile, ""}}));

    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, sdkDeclFile, &config), "dynamic@ohos.demo");
}

// The same interop declaration file can also be aliased by a 'paths' entry (the build system
// maps static aliases of dynamic APIs onto the interop declarations). The dynamic dependency
// classification wins: the declaration keeps its dynamic module name, so same-named symbols
// of the static and the dynamic SDK module stay on distinct module keys.
TEST_F(ComputeRelativeImportPathTests, DynamicDeclWithStaticAliasKeepsDynamicModule)
{
    const auto basePath = TempBasePath();
    ASSERT_FALSE(CreateNestedTempFile("api/@ohos.demo.d.ets", "declare namespace demo {}\n").empty());

    const auto canonical = std::filesystem::weakly_canonical(basePath).string();
    const auto entryPage = canonical + "/entry/src/main/ets/pages/AimpPage.ets";
    const auto sdkDeclFile = canonical + "/api/@ohos.demo.d.ets";
    std::string configText = R"({"compilerOptions": {"baseUrl": ")" + canonical + R"(", "paths": {"@ohos.demo": [")" +
                             sdkDeclFile + R"("]}, "dependencies":)" +
                             R"({"dynamic@ohos.demo": {"language": "js", "path": ")" + sdkDeclFile +
                             R"(", "ohmUrl": ""}}}})";
    auto config = ParsePathsConfig(configText);

    EXPECT_EQ(ark::es2panda::lsp::ComputeRelativeImportPath(entryPage, sdkDeclFile, &config), "dynamic@ohos.demo");
}

// End to end: the ImportFixes code fix inserts an import statement that uses the module
// name mapped by the arktsconfig 'paths' section instead of a relative file path, also for
// definitions that live in a non-entry file of the library module.
TEST_F(ComputeRelativeImportPathTests, ImportFixesUseModuleAliasForCrossModuleDefinition)
{
    const auto basePath = TempBasePath();
    const auto libraryUtilPath =
        std::filesystem::weakly_canonical(basePath / "library/src/main/ets/utils/AimpLibUtil.ets");
    const auto consumerPath = std::filesystem::weakly_canonical(basePath / "entry/src/main/ets/pages/AimpUse.ets");
    const auto configPath = basePath / "aimp_fixes_config.json";

    const std::string libraryText = "export function AimpLibFn(): void {}\n";
    const std::string consumerText = "function use(): void {\n    AimpLibFn();\n}\n";
    const auto configText =
        MakePathsConfig(basePath.string(), {{"library", libraryUtilPath.parent_path().parent_path().string()}});

    ASSERT_FALSE(CreateNestedTempFile("library/src/main/ets/utils/AimpLibUtil.ets", libraryText).empty());
    ASSERT_FALSE(CreateNestedTempFile("entry/src/main/ets/pages/AimpUse.ets", consumerText).empty());
    ASSERT_FALSE(CreateNestedTempFile("aimp_fixes_config.json", configText).empty());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, libraryUtilPath.string());

    const auto consumerFile = consumerPath.string();
    const auto configOption = "--arktsconfig=" + configPath.string();
    std::array<const char *, 2U> args = {consumerFile.c_str(), configOption.c_str()};
    auto *compilerImpl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    ASSERT_NE(compilerImpl, nullptr);

    auto configDeleter = [compilerImpl](es2panda_Config *config) { compilerImpl->DestroyConfigWithoutLog(config); };
    std::unique_ptr<es2panda_Config, decltype(configDeleter)> config(
        compilerImpl->CreateConfig(args.size(), args.data()), configDeleter);
    ASSERT_NE(config, nullptr);

    auto contextDeleter = [compilerImpl](es2panda_Context *context) { compilerImpl->DestroyContext(context); };
    std::unique_ptr<es2panda_Context, decltype(contextDeleter)> context(
        compilerImpl->CreateContextFromFile(config.get(), consumerFile.c_str()), contextDeleter);
    ASSERT_NE(context, nullptr);
    compilerImpl->ProceedToState(context.get(), ES2PANDA_STATE_BOUND);

    auto fixes = GetImportFixes(context.get(), consumerText, "AimpLibFn");
    auto descriptions = GetImportFixDescriptions(fixes);
    // The definition sits in a non-entry file and the entry does not export it: only the
    // file path under the module root is a valid import form.
    const auto aliasDescription = std::string("Add import {AimpLibFn} from 'library'");
    const auto fileDescription = std::string("Add import {AimpLibFn} from 'library/utils/AimpLibUtil'");
    EXPECT_EQ(std::find(descriptions.begin(), descriptions.end(), aliasDescription), descriptions.end());
    EXPECT_NE(std::find(descriptions.begin(), descriptions.end(), fileDescription), descriptions.end());

    const auto fileFixIt = std::find_if(fixes.begin(), fixes.end(), [&fileDescription](const auto &fix) {
        return fix.fixName_ == IMPORT_FIXES_ID && fix.description_ == fileDescription;
    });
    ASSERT_NE(fileFixIt, fixes.end());
    ASSERT_EQ(fileFixIt->changes_.size(), 1U);
    ASSERT_EQ(fileFixIt->changes_[0].textChanges.size(), 1U);
    const auto &change = fileFixIt->changes_[0].textChanges[0];
    EXPECT_EQ(change.span.start, 0U);
    EXPECT_EQ(change.span.length, 0U);
    EXPECT_EQ(change.newText, "import { AimpLibFn } from 'library/utils/AimpLibUtil';\n");
}

// When the module entry re-exports the definition, the bare module alias is a valid import
// form and is offered alongside the file-level path.
TEST_F(ComputeRelativeImportPathTests, ImportFixesUseModuleAliasWhenEntryReExports)
{
    const auto basePath = TempBasePath();
    const auto libraryIndexPath = std::filesystem::weakly_canonical(basePath / "library/src/main/ets/Index.ets");
    const auto libraryUtilPath =
        std::filesystem::weakly_canonical(basePath / "library/src/main/ets/utils/AimpLibUtil.ets");
    const auto consumerPath =
        std::filesystem::weakly_canonical(basePath / "entry/src/main/ets/pages/AimpUseReExport.ets");
    const auto configPath = basePath / "aimp_reexport_fix_config.json";

    const std::string libraryIndexText = "export { AimpLibFn } from './utils/AimpLibUtil';\n";
    const std::string libraryText = "export function AimpLibFn(): void {}\n";
    const std::string consumerText = "function use(): void {\n    AimpLibFn();\n}\n";
    const auto configText =
        MakePathsConfig(basePath.string(), {{"library", libraryUtilPath.parent_path().parent_path().string()}});

    ASSERT_FALSE(CreateNestedTempFile("library/src/main/ets/Index.ets", libraryIndexText).empty());
    ASSERT_FALSE(CreateNestedTempFile("library/src/main/ets/utils/AimpLibUtil.ets", libraryText).empty());
    ASSERT_FALSE(CreateNestedTempFile("entry/src/main/ets/pages/AimpUseReExport.ets", consumerText).empty());
    ASSERT_FALSE(CreateNestedTempFile("aimp_reexport_fix_config.json", configText).empty());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, libraryIndexPath.string());
    BuildSymbolIndex(indexInitializer, libraryUtilPath.string());

    const auto consumerFile = consumerPath.string();
    ScopedTestContext context(consumerFile, "", configPath.string());
    ASSERT_NE(context.Get(), nullptr);

    auto fixes = GetImportFixes(context.Get(), consumerText, "AimpLibFn");
    auto descriptions = GetImportFixDescriptions(fixes);
    const auto aliasDescription = std::string("Add import {AimpLibFn} from 'library'");
    const auto fileDescription = std::string("Add import {AimpLibFn} from 'library/utils/AimpLibUtil'");
    EXPECT_NE(std::find(descriptions.begin(), descriptions.end(), aliasDescription), descriptions.end());
    EXPECT_NE(std::find(descriptions.begin(), descriptions.end(), fileDescription), descriptions.end());
}

// A static module entry file can be mapped by both the module-root alias and a longer
// per-file alias. Auto import keeps the module alias ('library') as a candidate in addition
// to the per-file form, mirroring the import fix behavior.
TEST_F(ComputeRelativeImportPathTests, CompletionEntryCarriesModuleAliasForStaticEntryFile)
{
    const auto basePath = TempBasePath();
    const auto libraryRoot = std::filesystem::weakly_canonical(basePath / "library/src/main/ets");
    const auto libraryIndexPath = std::filesystem::weakly_canonical(libraryRoot / "Index.ets");
    const auto consumerPath = std::filesystem::weakly_canonical(basePath / "entry/PageEntryItem.ets");
    const auto configPath = basePath / "aimp_static_entry_config.json";

    const std::string libraryText = "export class EntryItem {}\n";
    const std::string consumerText = "function use(): void {\n    EntryItem\n}\n";
    const auto configText = MakePathsConfig(
        basePath.string(), {{"library", libraryRoot.string()}, {"library/Index", libraryIndexPath.string()}});

    ASSERT_FALSE(CreateNestedTempFile("library/src/main/ets/Index.ets", libraryText).empty());
    ASSERT_FALSE(CreateNestedTempFile("entry/PageEntryItem.ets", consumerText).empty());
    ASSERT_FALSE(CreateNestedTempFile("aimp_static_entry_config.json", configText).empty());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, libraryIndexPath.string());

    const auto consumerFile = consumerPath.string();
    ScopedTestContext context(consumerFile, consumerText, configPath.string());
    ASSERT_NE(context.Get(), nullptr);

    LSPAPI const *lspApi = GetImpl();
    const auto identifier = consumerText.find("EntryItem");
    ASSERT_NE(identifier, std::string::npos);
    auto entries = lspApi->getCompletionsAtPosition(context.Get(), identifier + std::strlen("EntryItem")).GetEntries();

    std::unordered_set<std::string> importDeclarations;
    for (auto &entry : entries) {
        auto data = entry.GetCompletionEntryData();
        if (!data.has_value() || data->GetNamedExport() != "EntryItem") {
            continue;
        }
        importDeclarations.insert(data->GetImportDeclaration());
    }
    EXPECT_EQ(importDeclarations.count("library"), 1U);
    EXPECT_EQ(importDeclarations.count("library/Index"), 1U);
}

// End to end: an auto import completion entry for a cross-module definition carries the
// file-level module import form in its 'importDeclaration' payload. The definition lives in
// a non-entry file, so the entry keeps its path under the module root instead of the bare
// module alias.
TEST_F(ComputeRelativeImportPathTests, CompletionEntryCarriesSubpathSpecifierForCrossModuleDefinition)
{
    const auto basePath = TempBasePath();
    const auto libraryUtilPath =
        std::filesystem::weakly_canonical(basePath / "library/src/main/ets/utils/AimpLibUtil.ets");
    const auto consumerPath = std::filesystem::weakly_canonical(basePath / "entry/src/main/ets/pages/AimpUseDecl.ets");
    const auto configPath = basePath / "aimp_decl_config.json";

    const std::string libraryText = "export function AimpLibFn(): void {}\n";
    const std::string consumerText = "function use(): void {\n    AimpLibFn\n}\n";
    const auto configText =
        MakePathsConfig(basePath.string(), {{"library", libraryUtilPath.parent_path().parent_path().string()}});

    ASSERT_FALSE(CreateNestedTempFile("library/src/main/ets/utils/AimpLibUtil.ets", libraryText).empty());
    ASSERT_FALSE(CreateNestedTempFile("entry/src/main/ets/pages/AimpUseDecl.ets", consumerText).empty());
    ASSERT_FALSE(CreateNestedTempFile("aimp_decl_config.json", configText).empty());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, libraryUtilPath.string());

    const auto consumerFile = consumerPath.string();
    const auto configOption = "--arktsconfig=" + configPath.string();
    std::array<const char *, 2U> args = {consumerFile.c_str(), configOption.c_str()};
    auto *compilerImpl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    ASSERT_NE(compilerImpl, nullptr);

    auto configDeleter = [compilerImpl](es2panda_Config *config) { compilerImpl->DestroyConfigWithoutLog(config); };
    std::unique_ptr<es2panda_Config, decltype(configDeleter)> config(
        compilerImpl->CreateConfig(args.size(), args.data()), configDeleter);
    ASSERT_NE(config, nullptr);

    auto contextDeleter = [compilerImpl](es2panda_Context *context) { compilerImpl->DestroyContext(context); };
    std::unique_ptr<es2panda_Context, decltype(contextDeleter)> context(
        compilerImpl->CreateContextFromString(config.get(), consumerText.c_str(), consumerFile.c_str()),
        contextDeleter);
    ASSERT_NE(context, nullptr);
    compilerImpl->ProceedToState(context.get(), ES2PANDA_STATE_BOUND);

    LSPAPI const *lspApi = GetImpl();
    const auto identifier = consumerText.find("AimpLibFn");
    ASSERT_NE(identifier, std::string::npos);
    auto result = lspApi->getCompletionsAtPosition(context.get(), identifier + std::strlen("AimpLibFn"));
    auto entries = result.GetEntries();

    std::unordered_set<std::string> importDeclarations;
    for (auto &entry : entries) {
        auto data = entry.GetCompletionEntryData();
        if (!data.has_value() || data->GetNamedExport() != "AimpLibFn") {
            continue;
        }
        importDeclarations.insert(data->GetImportDeclaration());
    }
    // The definition file is imported through its path under the module root.
    EXPECT_EQ(importDeclarations.count("library/utils/AimpLibUtil"), 1U);
}

// End to end: an auto import completion entry for a definition indexed from a dynamic module
// source file carries the file-level dependency key in its 'importDeclaration' payload.
TEST_F(ComputeRelativeImportPathTests, CompletionEntryCarriesModuleSpecifierForDynamicDependency)
{
    const auto basePath = TempBasePath();
    const auto dynUtilPath = std::filesystem::weakly_canonical(basePath / "dynlib/AimpDynUtil.ets");
    const auto consumerPath = std::filesystem::weakly_canonical(basePath / "entry/src/main/ets/pages/AimpUseDyn.ets");
    const auto configPath = basePath / "aimp_dyn_config.json";

    const std::string dynLibText = "export function AimpDynFn(): void {}\n";
    const std::string consumerText = "function use(): void {\n    AimpDynFn\n}\n";
    const auto configText = MakeDependenciesConfig(
        basePath.string(),
        {{"dynlib/AimpDynUtil", (basePath / "decls" / "AimpDynUtil.d.ets").string(), dynUtilPath.string()}});

    ASSERT_FALSE(CreateNestedTempFile("dynlib/AimpDynUtil.ets", dynLibText).empty());
    ASSERT_FALSE(
        CreateNestedTempFile("decls/AimpDynUtil.d.ets", "export declare function AimpDynFn(): void;\n").empty());
    ASSERT_FALSE(CreateNestedTempFile("entry/src/main/ets/pages/AimpUseDyn.ets", consumerText).empty());
    ASSERT_FALSE(CreateNestedTempFile("aimp_dyn_config.json", configText).empty());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, dynUtilPath.string());

    const auto consumerFile = consumerPath.string();
    ScopedTestContext context(consumerFile, consumerText, configPath.string());
    ASSERT_NE(context.Get(), nullptr);

    LSPAPI const *lspApi = GetImpl();
    const auto identifier = consumerText.find("AimpDynFn");
    ASSERT_NE(identifier, std::string::npos);
    auto result = lspApi->getCompletionsAtPosition(context.Get(), identifier + std::strlen("AimpDynFn"));
    auto entries = result.GetEntries();

    bool foundAutoImportEntry = false;
    for (auto &entry : entries) {
        auto data = entry.GetCompletionEntryData();
        if (!data.has_value() || data->GetNamedExport() != "AimpDynFn") {
            continue;
        }
        foundAutoImportEntry = true;
        // The entry carries the absolute path of the definition source file.
        EXPECT_EQ(data->GetFileName(), dynUtilPath.string());
        EXPECT_EQ(data->GetImportDeclaration(), "dynlib/AimpDynUtil");
    }
    ASSERT_TRUE(foundAutoImportEntry) << "auto import completion entry for AimpDynFn missing";
}

}  // namespace
