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
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <variant>

#include "ir/base/classDefinition.h"
#include "ir/base/methodDefinition.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/rename.h"
#include "lsp/include/types.h"
#include "lsp_api_test.h"
#include "util/path.h"

namespace {

using ark::es2panda::ir::AstNode;
using ark::es2panda::lsp::Initializer;

constexpr const char *NONEXISTENT_LIB_PATH = "/nonexistent/very/deep/path/";

class RenameModuleShapesTests : public LSPAPITests {
public:
    // Same derivation as lsp_rename_test.cpp GetPandalibPath: 3 levels up from the node's file.
    std::string GetPandalibPath(const AstNode *stdlibDecl)
    {
        auto filePath = std::string(stdlibDecl->Range().start.Program()->SourceFile().GetAbsolutePath().Utf8());
        size_t lastDelimPos = filePath.find_last_of(ark::es2panda::util::PATH_DELIMITER);
        std::string dir = (lastDelimPos != std::string::npos) ? filePath.substr(0, lastDelimPos) : filePath;
        const int threeLevelsUp = 3;
        for (int i = 0; i < threeLevelsUp; ++i) {
            size_t pos = dir.find_last_of(ark::es2panda::util::PATH_DELIMITER);
            if (pos != std::string::npos) {
                dir = dir.substr(0, pos);
            }
        }
        return dir;
    }

    static AstNode *FindNode(AstNode *root, const std::function<bool(AstNode *)> &pred)
    {
        AstNode *result = nullptr;
        root->FindChild([&result, &pred](AstNode *node) {
            if (result == nullptr && pred(node)) {
                result = node;
            }
            return false;
        });
        return result;
    }

    static AstNode *FindStringLiteral(AstNode *root, const std::string &content)
    {
        return FindNode(root, [&content](AstNode *node) {
            return node->IsStringLiteral() && node->AsStringLiteral()->ToString() == content;
        });
    }

    static AstNode *FindClassDefinition(AstNode *root, const std::string &name)
    {
        return FindNode(root, [&name](AstNode *node) {
            return node->IsClassDefinition() && node->AsClassDefinition()->Ident() != nullptr &&
                   node->AsClassDefinition()->Ident()->Name().Utf8() == name;
        });
    }

    static void ExpectMethodKindStrings(AstNode *ast)
    {
        auto *getter = FindNode(ast, [](AstNode *node) {
            return node->IsMethodDefinition() &&
                   node->AsMethodDefinition()->Kind() == ark::es2panda::ir::MethodDefinitionKind::GET;
        });
        ASSERT_NE(getter, nullptr);
        EXPECT_EQ(ark::es2panda::lsp::GetNodeKindForRenameInfo(getter), "get");

        auto *setter = FindNode(ast, [](AstNode *node) {
            return node->IsMethodDefinition() &&
                   node->AsMethodDefinition()->Kind() == ark::es2panda::ir::MethodDefinitionKind::SET;
        });
        ASSERT_NE(setter, nullptr);
        EXPECT_EQ(ark::es2panda::lsp::GetNodeKindForRenameInfo(setter), "set");

        auto *ctor = FindNode(ast, [](AstNode *node) {
            return node->IsMethodDefinition() &&
                   node->AsMethodDefinition()->Kind() == ark::es2panda::ir::MethodDefinitionKind::CONSTRUCTOR;
        });
        ASSERT_NE(ctor, nullptr);
        EXPECT_EQ(ark::es2panda::lsp::GetNodeKindForRenameInfo(ctor), "constructor");
    }
};

// PI-003 — module-specifier rename, plain module kind (no "/index" suffix): the whole literal
// content becomes name/displayName/fullDisplayName (rename.cpp:347-352), the kind is "module"
// (rename.cpp:354), and the trigger span starts after the quote and the last slash with the
// last-segment length (rename.cpp:355-357).
TEST_F(RenameModuleShapesTests, ModuleKindPlainPath)
{
    Initializer initializer;
    const std::string source = "const s = \"lib/network.ets\";\n";
    auto filePaths = CreateTempFile({"module_shapes_plain.ets"}, {source});
    ASSERT_EQ(filePaths.size(), 1U);

    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto *ast = GetAstFromContext<AstNode>(ctx);
    auto *strLit = FindStringLiteral(ast, "lib/network.ets");
    ASSERT_NE(strLit, nullptr);
    auto *program = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram;

    auto renameInfo = ark::es2panda::lsp::GetRenameInfoForModule(strLit, program);
    ASSERT_TRUE(renameInfo.has_value());
    EXPECT_TRUE(renameInfo->GetCanRenameSuccess());
    EXPECT_EQ(renameInfo->GetDisplayName(), "lib/network.ets");
    EXPECT_EQ(renameInfo->GetFullDisplayName(), "lib/network.ets");
    EXPECT_EQ(renameInfo->GetKind(), "module");
    // span: skip quote + "lib/" (5 bytes), cover "network.ets" (11 bytes)
    EXPECT_EQ(renameInfo->GetTriggerSpan().start, strLit->Start().index + 5);
    EXPECT_EQ(renameInfo->GetTriggerSpan().length, strlen("network.ets"));

    initializer.DestroyContext(ctx);
}

// PI-003 — directory kind: "app/index.ets" misses the literal "/index" suffix, drops the
// ".ets" extension (rename.cpp:339-340), then ends with "/index" so the "/index" suffix is
// stripped into a "directory" rename (rename.cpp:341-342, kind rename.cpp:354).
TEST_F(RenameModuleShapesTests, DirectoryKindIndexedExtension)
{
    Initializer initializer;
    const std::string source = "const s = \"app/index.ets\";\n";
    auto filePaths = CreateTempFile({"module_shapes_indexed.ets"}, {source});
    ASSERT_EQ(filePaths.size(), 1U);

    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto *ast = GetAstFromContext<AstNode>(ctx);
    auto *strLit = FindStringLiteral(ast, "app/index.ets");
    ASSERT_NE(strLit, nullptr);
    auto *program = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram;

    auto renameInfo = ark::es2panda::lsp::GetRenameInfoForModule(strLit, program);
    ASSERT_TRUE(renameInfo.has_value());
    EXPECT_EQ(renameInfo->GetDisplayName(), "app");
    EXPECT_EQ(renameInfo->GetFullDisplayName(), "app");
    EXPECT_EQ(renameInfo->GetKind(), "directory");
    // span: skip quote + "app/" (5 bytes), cover "index.ets" (9 bytes)
    EXPECT_EQ(renameInfo->GetTriggerSpan().start, strLit->Start().index + 5);
    EXPECT_EQ(renameInfo->GetTriggerSpan().length, strlen("index.ets"));

    initializer.DestroyContext(ctx);
}

// PI-003 — a dot before the last slash is stripped as if it were an extension
// (rename.cpp:339-340 find_last_of('.') yields "v1" from "v1.2/pkg"), but "v1" does not end
// with "/index", so the module kind keeps the FULL original literal (rename.cpp:348-352). The
// trigger span is slash-based regardless (rename.cpp:355-357) and covers only the last segment.
TEST_F(RenameModuleShapesTests, ModuleKindDotPrefixStaysModule)
{
    Initializer initializer;
    const std::string source = "const s = \"v1.2/pkg\";\n";
    auto filePaths = CreateTempFile({"module_shapes_dot.ets"}, {source});
    ASSERT_EQ(filePaths.size(), 1U);

    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto *ast = GetAstFromContext<AstNode>(ctx);
    auto *strLit = FindStringLiteral(ast, "v1.2/pkg");
    ASSERT_NE(strLit, nullptr);
    auto *program = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram;

    auto renameInfo = ark::es2panda::lsp::GetRenameInfoForModule(strLit, program);
    ASSERT_TRUE(renameInfo.has_value());
    EXPECT_EQ(renameInfo->GetDisplayName(), "v1.2/pkg");
    EXPECT_EQ(renameInfo->GetFullDisplayName(), "v1.2/pkg");
    EXPECT_EQ(renameInfo->GetKind(), "module");
    // span: skip quote + "v1.2/" (6 bytes), cover "pkg" (3 bytes)
    EXPECT_EQ(renameInfo->GetTriggerSpan().start, strLit->Start().index + 6);
    EXPECT_EQ(renameInfo->GetTriggerSpan().length, strlen("pkg"));

    initializer.DestroyContext(ctx);
}

// PI-003 reachability record for the empty-source-file guard (rename.cpp:330-331): measured
// UNREACHABLE from real contexts — CreateContext("") yields an ERROR-state context
// (ES2PANDA_STATE_ERROR == 7, measured), and every usable program carries a non-empty
// SourceFile path, so no public route delivers moduleSourceFile.GetPath().Empty() and the
// nullopt early return cannot execute. No testcase; chain recorded here and in the batch
// report.

// PI-004 — node-kind string matrix, part 1: namespace-transformed class definition
// (GetKindOfClassDefinition rename.cpp:389-390), raw TS enum declaration via OrigEnumDecl
// (GetNodeKindForRenameInfo rename.cpp:428-429), and the getter/setter/constructor method
// kinds (GetKindOfMethod rename.cpp:370-375).
TEST_F(RenameModuleShapesTests, NodeKindNamespaceEnumAccessorMatrix)
{
    Initializer initializer;
    const std::string source =
        "namespace NS {\n"
        "  export let v: number = 1;\n"
        "}\n"
        "enum Color { Red, Green }\n"
        "class C {\n"
        "  private vv: number = 0;\n"
        "  get val(): number {\n"
        "    return this.vv;\n"
        "  }\n"
        "  set val(n: number) {\n"
        "    this.vv = n;\n"
        "  }\n"
        "  constructor() {\n"
        "  }\n"
        "}\n";
    es2panda_Context *ctx =
        initializer.CreateContext("module_shapes_kinds1.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto *ast = GetAstFromContext<AstNode>(ctx);

    auto *nsClass = FindClassDefinition(ast, "NS");
    ASSERT_NE(nsClass, nullptr);
    ASSERT_TRUE(nsClass->AsClassDefinition()->IsNamespaceTransformed());
    EXPECT_EQ(ark::es2panda::lsp::GetNodeKindForRenameInfo(nsClass), "namespace");

    auto *enumClass = FindClassDefinition(ast, "Color");
    ASSERT_NE(enumClass, nullptr);
    auto *origEnumDecl = enumClass->AsClassDefinition()->OrigEnumDecl();
    ASSERT_NE(origEnumDecl, nullptr);
    ASSERT_TRUE(origEnumDecl->IsTSEnumDeclaration());
    EXPECT_EQ(ark::es2panda::lsp::GetNodeKindForRenameInfo(origEnumDecl), "enum");

    ExpectMethodKindStrings(ast);

    initializer.DestroyContext(ctx);
}

// PI-004 — node-kind string matrix, part 2: an enum member survives CHECKED as a ClassProperty
// inside the enum-transformed class, classified "enum member" via
// ClassDefinitionIsEnumTransformed (rename.cpp:401-403); a plain class definition is "class"
// (rename.cpp:395).
TEST_F(RenameModuleShapesTests, NodeKindEnumMemberAndClassArms)
{
    Initializer initializer;
    const std::string source =
        "enum Color { Red, Green }\n"
        "class C {\n"
        "  v: number = 0;\n"
        "}\n";
    es2panda_Context *ctx =
        initializer.CreateContext("module_shapes_kinds2.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto *ast = GetAstFromContext<AstNode>(ctx);

    auto *enumMemberProperty = FindNode(ast, [](AstNode *node) {
        return node->IsClassProperty() && node->Parent() != nullptr && node->Parent()->IsClassDefinition() &&
               node->Parent()->AsClassDefinition()->OrigEnumDecl() != nullptr;
    });
    ASSERT_NE(enumMemberProperty, nullptr);
    EXPECT_EQ(ark::es2panda::lsp::GetNodeKindForRenameInfo(enumMemberProperty), "enum member");

    auto *classDef = FindClassDefinition(ast, "C");
    ASSERT_NE(classDef, nullptr);
    EXPECT_EQ(ark::es2panda::lsp::GetNodeKindForRenameInfo(classDef), "class");

    initializer.DestroyContext(ctx);
}

// PI-004 — struct kind probe: structs are rejected at the plugin-after-parser phase
// (ESE0364), so the struct shape only exists at ES2PANDA_STATE_PARSED; there the parsed
// ClassDefinition is wrapped by ETSStructDeclaration and classified "struct" via the parent
// disjunct of GetKindOfClassDefinition (rename.cpp:392-393).
TEST_F(RenameModuleShapesTests, StructParsedStateKindProbe)
{
    Initializer initializer;
    const std::string source = "struct S {\n  a: number = 1;\n}\n";
    es2panda_Context *ctx =
        initializer.CreateContext("module_shapes_struct.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto *ast = GetAstFromContext<AstNode>(ctx);
    auto *structClass = FindNode(ast, [](AstNode *node) {
        return node->IsClassDefinition() && node->Parent() != nullptr && node->Parent()->IsETSStructDeclaration();
    });
    ASSERT_NE(structClass, nullptr);
    EXPECT_EQ(ark::es2panda::lsp::GetNodeKindForRenameInfo(structClass), "struct");

    initializer.DestroyContext(ctx);
}

// PI-004 — GetRenameInfoSuccess (rename.cpp:58-64) trigger-span construction: a string-literal
// node gets the quote-trimmed span (CreateTriggerSpanForNode rename.cpp:70-73, never executed
// before this batch), an identifier node keeps the plain node range (rename.cpp:68).
TEST_F(RenameModuleShapesTests, SuccessTriggerSpanStringTrim)
{
    Initializer initializer;
    const std::string source = "let s: string = \"hello\";\nlet q: number = 1;\n";
    es2panda_Context *ctx = initializer.CreateContext("module_shapes_span.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto *ast = GetAstFromContext<AstNode>(ctx);
    auto *strLit = FindStringLiteral(ast, "hello");
    ASSERT_NE(strLit, nullptr);
    const size_t quotePos = source.find("\"hello\"");
    ASSERT_EQ(strLit->Start().index, quotePos);

    auto strInfo = ark::es2panda::lsp::GetRenameInfoSuccess("d", "d", "string", "", strLit);
    EXPECT_TRUE(strInfo.GetCanRenameSuccess());
    EXPECT_EQ(strInfo.GetKind(), "string");
    EXPECT_EQ(strInfo.GetTriggerSpan().start, quotePos + 1);
    EXPECT_EQ(strInfo.GetTriggerSpan().length, strlen("hello"));

    auto *qIdent =
        FindNode(ast, [](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "q"; });
    ASSERT_NE(qIdent, nullptr);
    auto identInfo = ark::es2panda::lsp::GetRenameInfoSuccess("d", "d", "variable", "", qIdent);
    EXPECT_EQ(identInfo.GetTriggerSpan().start, qIdent->Start().index);
    EXPECT_EQ(identInfo.GetTriggerSpan().length, strlen("q"));

    initializer.DestroyContext(ctx);
}

// Library declarations are rejected through the public rename flow, while local declarations in
// the same context remain renameable.
TEST_F(RenameModuleShapesTests, LibraryFileArms)
{
    Initializer initializer;
    const std::string source = "console.log(\"x\");\nlet q: number = 1;\n";
    es2panda_Context *ctx =
        initializer.CreateContext("module_shapes_libfile.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto *ast = GetAstFromContext<AstNode>(ctx);
    auto *consoleIdent =
        FindNode(ast, [](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "console"; });
    ASSERT_NE(consoleIdent, nullptr);
    ASSERT_NE(consoleIdent->Variable(), nullptr);
    const std::string pandaLibPath = GetPandalibPath(consoleIdent->Variable()->Declaration()->Node());

    const size_t consolePos = source.find("console");
    ASSERT_NE(consolePos, std::string::npos);
    auto consoleInfo = ark::es2panda::lsp::GetRenameInfo(ctx, consolePos, pandaLibPath);
    ASSERT_TRUE(std::holds_alternative<ark::es2panda::lsp::RenameInfoFailure>(consoleInfo));
    const auto &consoleFailure = std::get<ark::es2panda::lsp::RenameInfoFailure>(consoleInfo);
    EXPECT_FALSE(consoleFailure.GetCanRenameFailure());
    EXPECT_EQ(consoleFailure.GetLocalizedErrorMessage(), "You cannot rename this element");

    const size_t qPos = source.find("q");
    ASSERT_NE(qPos, std::string::npos);
    auto qInfo = ark::es2panda::lsp::GetRenameInfo(ctx, qPos, pandaLibPath);
    ASSERT_TRUE(std::holds_alternative<ark::es2panda::lsp::RenameInfoSuccess>(qInfo));
    const auto &qSuccess = std::get<ark::es2panda::lsp::RenameInfoSuccess>(qInfo);
    EXPECT_TRUE(qSuccess.GetCanRenameSuccess());
    EXPECT_EQ(qSuccess.GetKind(), "property");
    EXPECT_EQ(qSuccess.GetDisplayName(), "q");
    EXPECT_EQ(qSuccess.GetFullDisplayName(), "q");
    EXPECT_EQ(qSuccess.GetTriggerSpan().start, qPos);
    EXPECT_EQ(qSuccess.GetTriggerSpan().length, strlen("q"));

    initializer.DestroyContext(ctx);
}

// Declarations loaded from an oh_modules path are rejected, while the same source in a local
// temporary file remains renameable.
TEST_F(RenameModuleShapesTests, OhModulesArms)
{
    namespace fs = std::filesystem;
    Initializer initializer;
    const std::string source = "let m: number = 1;\n";

    fs::path ohModulesDir = fs::path(testing::TempDir()) / GetExecutableName() / "oh_modules";
    fs::create_directories(ohModulesDir);
    fs::path pkgPath = ohModulesDir / "pkg.ets";
    {
        std::ofstream outStream(pkgPath);
        ASSERT_FALSE(outStream.fail());
        outStream << source;
    }

    es2panda_Context *ohModulesCtx = initializer.CreateContext(pkgPath.c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ohModulesCtx), ES2PANDA_STATE_CHECKED);
    const size_t ohModulesPos = source.find("m");
    ASSERT_NE(ohModulesPos, std::string::npos);
    auto ohModulesInfo = ark::es2panda::lsp::GetRenameInfo(ohModulesCtx, ohModulesPos, NONEXISTENT_LIB_PATH);
    ASSERT_TRUE(std::holds_alternative<ark::es2panda::lsp::RenameInfoFailure>(ohModulesInfo));
    const auto &ohModulesFailure = std::get<ark::es2panda::lsp::RenameInfoFailure>(ohModulesInfo);
    EXPECT_FALSE(ohModulesFailure.GetCanRenameFailure());
    EXPECT_EQ(ohModulesFailure.GetLocalizedErrorMessage(), "You cannot rename this element");

    initializer.DestroyContext(ohModulesCtx);
    fs::remove_all(ohModulesDir);

    auto filePaths = CreateTempFile({"module_shapes_plain_ctx.ets"}, {source});
    ASSERT_EQ(filePaths.size(), 1U);
    es2panda_Context *plainCtx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(plainCtx), ES2PANDA_STATE_CHECKED);
    const size_t plainPos = source.find("m");
    ASSERT_NE(plainPos, std::string::npos);
    auto plainInfo = ark::es2panda::lsp::GetRenameInfo(plainCtx, plainPos, NONEXISTENT_LIB_PATH);
    ASSERT_TRUE(std::holds_alternative<ark::es2panda::lsp::RenameInfoSuccess>(plainInfo));
    const auto &plainSuccess = std::get<ark::es2panda::lsp::RenameInfoSuccess>(plainInfo);
    EXPECT_TRUE(plainSuccess.GetCanRenameSuccess());
    EXPECT_EQ(plainSuccess.GetKind(), "property");
    EXPECT_EQ(plainSuccess.GetDisplayName(), "m");
    EXPECT_EQ(plainSuccess.GetTriggerSpan().start, plainPos);
    EXPECT_EQ(plainSuccess.GetTriggerSpan().length, strlen("m"));

    initializer.DestroyContext(plainCtx);
}

}  // namespace
