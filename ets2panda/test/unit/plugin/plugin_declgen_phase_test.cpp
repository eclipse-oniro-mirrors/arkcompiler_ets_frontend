/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "libarkbase/os/filesystem.h"
#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

static constexpr std::string_view ARKTSCONFIG_OPTION = "--arktsconfig";
static constexpr std::string_view ARKTSCONFIG_PREFIX = "--arktsconfig=";
static constexpr std::string_view OUTPUT_PREFIX = "--output=";

static constexpr std::string_view MODULE_DECLARATION_NAME = "ModuleDeclaration";
static constexpr std::string_view DECLARATION_PROPERTY = "declaration";
static constexpr std::string_view IMPORT_MARKER = "import { ConfigService }";
static constexpr std::string_view EXPECTED_IMPORT =
    "import { ConfigService } from \"har3/module/config/configService\";";
// Mixed-separator buggy form produced by the former single rfind('.') split:
// "har3.module.config/configService"
static constexpr std::string_view PACKAGE_QUALIFIED_MARKER = "from \"har3.";
// Preserved-relative-import regression form (breaks --read-metadata consumers):
// "./module/config/configService"
static constexpr std::string_view RELATIVE_IMPORT_MARKER = "from \"./module/config/configService\"";

static es2panda_Impl *impl = nullptr;
static es2panda_Context *g_context = nullptr;
static std::vector<std::string> g_declarationTexts;
static std::string g_stringLiteralValue;

static std::string FindArktsConfig(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg.compare(0, ARKTSCONFIG_PREFIX.size(), ARKTSCONFIG_PREFIX) == 0) {
            return std::string(arg.substr(ARKTSCONFIG_PREFIX.size()));
        }
        if (arg == ARKTSCONFIG_OPTION && i + 1 < argc) {
            return std::string(argv[i + 1]);
        }
    }
    return "";
}

static std::vector<std::string> BuildConfigArgs(int argc, char **argv, const std::string &configPath)
{
    std::vector<std::string> args;
    args.emplace_back(ark::os::GetAbsolutePath(argv[1]));
    for (int i = 2; i < argc - 1; ++i) {
        std::string_view arg = argv[i];
        if (arg.compare(0, OUTPUT_PREFIX.size(), OUTPUT_PREFIX) == 0 ||
            arg.compare(0, ARKTSCONFIG_PREFIX.size(), ARKTSCONFIG_PREFIX) == 0) {
            continue;
        }
        args.emplace_back(argv[i]);
    }
    args.emplace_back(std::string(ARKTSCONFIG_PREFIX) + ark::os::GetAbsolutePath(configPath));
    args.emplace_back("--simultaneous");
    args.emplace_back("--emit-declaration");
    args.emplace_back("--opt-level=0");
    return args;
}

struct TestPaths {
    std::string mainPagePath;
    std::string configServicePath;
};

static bool ResolveTestPaths(int argc, char **argv, TestPaths &paths)
{
    std::string testDataDir = ark::os::GetAbsolutePath(ark::os::GetParentDir(argv[argc - 1])) + "/declgen_phase";
    paths.mainPagePath = testDataDir + "/MainPage.ats";
    paths.configServicePath = testDataDir + "/module/config/configService.ats";
    return true;
}

static void CaptureStringLiteral(es2panda_AstNode *node)
{
    if (impl->IsStringLiteral(node)) {
        g_stringLiteralValue = impl->StringLiteralStrConst(g_context, node);
    }
}

static std::string GetDeclarationPropertyValue(es2panda_AstNode *property)
{
    g_stringLiteralValue.clear();
    impl->AstNodeIterateConst(g_context, property, CaptureStringLiteral);
    return g_stringLiteralValue;
}

static void CollectAnnotationDeclarations(es2panda_Context *context, es2panda_Program *program)
{
    auto *globalClass = impl->ProgramGlobalClass(context, program);
    if (globalClass == nullptr) {
        return;
    }
    size_t annotationsLen = 0;
    auto **annotations = impl->ClassDefinitionAnnotations(context, globalClass, &annotationsLen);
    for (size_t i = 0; i < annotationsLen; ++i) {
        auto *annotation = annotations[i];
        if (!impl->IsAnnotationUsage(annotation)) {
            continue;
        }
        auto *expr = impl->AnnotationUsageIrExpr(context, annotation);
        if (expr == nullptr || !impl->IsIdentifier(expr)) {
            continue;
        }
        const char *annotationName = impl->IdentifierName(context, expr);
        if (annotationName == nullptr || MODULE_DECLARATION_NAME != annotationName) {
            continue;
        }
        size_t propertiesLen = 0;
        auto **properties = impl->AnnotationUsageIrPropertiesConst(context, annotation, &propertiesLen);
        for (size_t j = 0; j < propertiesLen; ++j) {
            auto *key = impl->ClassElementId(context, properties[j]);
            const char *keyName = (key != nullptr) ? impl->IdentifierName(context, key) : nullptr;
            if (keyName == nullptr || DECLARATION_PROPERTY != keyName) {
                continue;
            }
            std::string declarationText = GetDeclarationPropertyValue(properties[j]);
            if (!declarationText.empty()) {
                g_declarationTexts.push_back(std::move(declarationText));
            }
        }
    }
}

static void CollectAllAnnotationDeclarations(es2panda_Context *context)
{
    auto *program = impl->ContextProgram(context);
    CollectAnnotationDeclarations(context, program);

    size_t sourcesLen = 0;
    auto **sources = impl->ProgramExternalSources(context, program, &sourcesLen);
    if (sources == nullptr) {
        return;
    }
    for (size_t i = 0; i < sourcesLen; ++i) {
        size_t programsLen = 0;
        auto **programs = impl->ExternalSourcePrograms(sources[i], &programsLen);
        for (size_t j = 0; j < programsLen; ++j) {
            CollectAnnotationDeclarations(context, programs[j]);
        }
    }
}

static int ReportDeclarationFailure(const std::string &reason, const std::string &declarationText)
{
    std::cerr << "FAIL: " << reason << std::endl;
    std::cerr << std::endl;
    std::cerr << "--- ModuleDeclaration declaration text ---" << std::endl;
    std::cerr << declarationText << std::endl;
    std::cerr << "-------------------------------------------" << std::endl;
    return TEST_ERROR_CODE;
}

static int CheckAnnotationDeclarations()
{
    std::string mainPageDecl;
    for (const auto &decl : g_declarationTexts) {
        if (decl.find(IMPORT_MARKER) != std::string::npos) {
            mainPageDecl = decl;
            break;
        }
    }
    if (mainPageDecl.empty()) {
        std::cerr << "FAIL: no ModuleDeclaration annotation with a ConfigService import was found." << std::endl;
        std::cerr << "Either DeclGenPhase did not run or it collected no imports." << std::endl;
        std::cerr << "Collected declarations: " << g_declarationTexts.size() << std::endl;
        return TEST_ERROR_CODE;
    }
    if (mainPageDecl.find(PACKAGE_QUALIFIED_MARKER) != std::string::npos) {
        return ReportDeclarationFailure(
            "intra-package relative import was rewritten with mixed separators (or a dotted package prefix) in "
            "the ModuleDeclaration annotation. Every separator dot of the flattened ModuleName must become '/': "
            "see PackageQualifiedImportSource() (ir/srcDump.cpp).",
            mainPageDecl);
    }
    if (mainPageDecl.find(RELATIVE_IMPORT_MARKER) != std::string::npos) {
        return ReportDeclarationFailure(
            "intra-package relative import was preserved in the ModuleDeclaration annotation. Relative imports "
            "must be rewritten into their full ohm-url form: --read-metadata consumers resolve relative sources "
            "against the etscache directory and end up in a separate program instance, breaking subtype "
            "relations across modules. See CallDeclgen() (compiler/lowering/ets/declGenPhase.cpp) and "
            "PackageQualifiedImportSource() (ir/srcDump.cpp).",
            mainPageDecl);
    }
    if (mainPageDecl.find(EXPECTED_IMPORT) == std::string::npos) {
        return ReportDeclarationFailure("ohm-url rewritten import not found in the ModuleDeclaration annotation.",
                                        mainPageDecl);
    }
    std::cout << "PASS: intra-package relative import rewritten into full ohm-url form in ModuleDeclaration "
                 "annotation"
              << std::endl;
    return 0;
}

static int RunTest(es2panda_Config *config, const TestPaths &paths)
{
    const char *fileNames[] = {paths.configServicePath.c_str(), paths.mainPagePath.c_str()};
    auto *context = impl->CreateContextSimultaneousMode(config, 2, fileNames);
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }
    g_context = context;

    impl->ProceedToState(context, ES2PANDA_STATE_LOWERED);
    CheckForErrors("LOWERED", context);
    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR) {
        std::cerr << "COMPILATION FAILED: " << impl->ContextErrorMessage(context) << std::endl;
        impl->DestroyContext(context);
        g_context = nullptr;
        return PROCEED_ERROR_CODE;
    }

    g_declarationTexts.clear();
    CollectAllAnnotationDeclarations(context);
    int result = CheckAnnotationDeclarations();
    impl->DestroyContext(context);
    g_context = nullptr;
    return result;
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }
    impl = GetImpl();
    if (impl == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }
    std::cout << "LOAD SUCCESS" << std::endl;

    TestPaths paths;
    ResolveTestPaths(argc, argv, paths);

    std::string configPath = FindArktsConfig(argc, argv);
    if (configPath.empty()) {
        std::cerr << "No --arktsconfig found in argv" << std::endl;
        return TEST_ERROR_CODE;
    }

    auto stringArgs = BuildConfigArgs(argc, argv, configPath);
    std::vector<const char *> args;
    args.reserve(stringArgs.size());
    for (const auto &arg : stringArgs) {
        args.push_back(arg.c_str());
    }
    auto *config = impl->CreateConfig(static_cast<int>(args.size()), args.data());
    if (config == nullptr) {
        std::cerr << "FAILED TO CREATE CONFIG" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    int result = RunTest(config, paths);
    impl->DestroyConfig(config);
    return result;
}

// NOLINTEND
