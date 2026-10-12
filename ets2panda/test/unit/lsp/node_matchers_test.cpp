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

#include "lsp/include/node_matchers.h"
#include "lsp/include/api.h"
#include "lsp_api_test.h"
#include <gtest/gtest.h>

namespace {
using ::NodeInfo;
using ark::es2panda::ir::AstNode;
using ark::es2panda::ir::AstNodeType;
using ark::es2panda::lsp::Initializer;

class LspNodeMatchersTests : public LSPAPITests {};

TEST_F(LspNodeMatchersTests, MatchClassDefinition_MatchesClass)
{
    Initializer initializer = Initializer();
    const std::string source = R"(class MyClass {})";
    es2panda_Context *ctx = initializer.CreateContext("class_def_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    auto classDef = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::CLASS_DEFINITION; });
    ASSERT_NE(classDef, nullptr);

    NodeInfo info("MyClass", AstNodeType::CLASS_DEFINITION);
    bool result = ark::es2panda::lsp::MatchClassDefinition(classDef, &info);
    EXPECT_TRUE(result);

    initializer.DestroyContext(ctx);
}

TEST_F(LspNodeMatchersTests, MatchIdentifier_MatchesIdentifier)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x = 42;)";
    es2panda_Context *ctx = initializer.CreateContext("ident_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    auto ident = ast->FindChild(
        [](AstNode *node) { return node->Type() == AstNodeType::IDENTIFIER && node->AsIdentifier()->Name() == "x"; });
    ASSERT_NE(ident, nullptr);

    NodeInfo info("x", AstNodeType::IDENTIFIER);
    bool result = ark::es2panda::lsp::MatchIdentifier(ident, &info);
    EXPECT_TRUE(result);

    initializer.DestroyContext(ctx);
}

TEST_F(LspNodeMatchersTests, MatchClassProperty_MatchesProperty)
{
    Initializer initializer = Initializer();
    const std::string source = R"(class MyClass { prop: number = 0; })";
    es2panda_Context *ctx = initializer.CreateContext("class_prop_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    auto classProp = ast->FindChild([](AstNode *node) {
        return node->Type() == AstNodeType::CLASS_PROPERTY && node->AsClassProperty()->Id()->Name() == "prop";
    });
    ASSERT_NE(classProp, nullptr);

    NodeInfo info("prop", AstNodeType::CLASS_PROPERTY);
    bool result = ark::es2panda::lsp::MatchClassProperty(classProp, &info);
    EXPECT_TRUE(result);

    initializer.DestroyContext(ctx);
}

TEST_F(LspNodeMatchersTests, MatchMethodDefinition_MatchesMethod)
{
    Initializer initializer = Initializer();
    const std::string source = R"(class MyClass { myMethod() {} })";
    es2panda_Context *ctx = initializer.CreateContext("method_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    auto methodDef = ast->FindChild([](AstNode *node) {
        return node->Type() == AstNodeType::METHOD_DEFINITION &&
               node->AsMethodDefinition()->Function()->Id()->Name() == "myMethod";
    });
    ASSERT_NE(methodDef, nullptr);

    NodeInfo info("myMethod", AstNodeType::METHOD_DEFINITION);
    bool result = ark::es2panda::lsp::MatchMethodDefinition(methodDef, &info);
    EXPECT_TRUE(result);

    initializer.DestroyContext(ctx);
}

TEST_F(LspNodeMatchersTests, MatchTSEnumDeclaration_MatchesEnum)
{
    Initializer initializer = Initializer();
    const std::string source = R"(enum MyEnum { A, B })";
    es2panda_Context *ctx = initializer.CreateContext("enum_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    auto enumDecl = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::TS_ENUM_DECLARATION; });
    ASSERT_NE(enumDecl, nullptr);

    NodeInfo info("MyEnum", AstNodeType::TS_ENUM_DECLARATION);
    bool result = ark::es2panda::lsp::MatchTSEnumDeclaration(enumDecl, &info);
    EXPECT_TRUE(result);

    initializer.DestroyContext(ctx);
}

TEST_F(LspNodeMatchersTests, MatchTSInterfaceDeclaration_MatchesInterface)
{
    Initializer initializer = Initializer();
    const std::string source = R"(interface MyInterface { prop: string; })";
    es2panda_Context *ctx = initializer.CreateContext("interface_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    auto interfaceDecl =
        ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::TS_INTERFACE_DECLARATION; });
    ASSERT_NE(interfaceDecl, nullptr);

    NodeInfo info("MyInterface", AstNodeType::TS_INTERFACE_DECLARATION);
    bool result = ark::es2panda::lsp::MatchTSInterfaceDeclaration(interfaceDecl, &info);
    EXPECT_TRUE(result);

    initializer.DestroyContext(ctx);
}

TEST_F(LspNodeMatchersTests, MatchExportSpecifier_MatchesExport)
{
    Initializer initializer = Initializer();
    const std::string source = R"(export { Foo } from './mod';)";
    es2panda_Context *ctx = initializer.CreateContext("export_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    auto exportDecl = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::REEXPORT_STATEMENT; });
    ASSERT_NE(exportDecl, nullptr);

    NodeInfo info("Foo", AstNodeType::REEXPORT_STATEMENT);
    bool result = ark::es2panda::lsp::MatchExportSpecifier(exportDecl, &info);
    EXPECT_TRUE(result);

    initializer.DestroyContext(ctx);
}

TEST_F(LspNodeMatchersTests, MatchMemberExpression_MatchesMemberAccess)
{
    Initializer initializer = Initializer();
    const std::string source = R"(obj.prop;)";
    es2panda_Context *ctx = initializer.CreateContext("member_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    auto memberExpr = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::MEMBER_EXPRESSION; });
    ASSERT_NE(memberExpr, nullptr);

    NodeInfo info("prop", AstNodeType::MEMBER_EXPRESSION);
    bool result = ark::es2panda::lsp::MatchMemberExpression(memberExpr, &info);
    EXPECT_TRUE(result);

    initializer.DestroyContext(ctx);
}

TEST_F(LspNodeMatchersTests, MatchEtsStructDeclaration_MatchesStruct)
{
    Initializer initializer = Initializer();
    const std::string source = R"(struct MyStruct { field: number; })";
    es2panda_Context *ctx = initializer.CreateContext("struct_decl_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    auto structDecl = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::STRUCT_DECLARATION; });
    ASSERT_NE(structDecl, nullptr);

    NodeInfo info("MyStruct", AstNodeType::STRUCT_DECLARATION);
    bool result = ark::es2panda::lsp::MatchEtsStructDeclaration(structDecl, &info);
    EXPECT_TRUE(result);

    initializer.DestroyContext(ctx);
}

// Super/member-callee branches remain untested; this pins the plain-identifier callee arm.
TEST_F(LspNodeMatchersTests, MatchCallExpression_MatchesCall)
{
    Initializer initializer = Initializer();
    const std::string source = R"(myFunc();)";
    es2panda_Context *ctx = initializer.CreateContext("call_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    auto callExpr = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::CALL_EXPRESSION; });
    ASSERT_NE(callExpr, nullptr);

    NodeInfo info("myFunc", AstNodeType::CALL_EXPRESSION);
    bool result = ark::es2panda::lsp::MatchCallExpression(callExpr, &info);
    EXPECT_TRUE(result);

    initializer.DestroyContext(ctx);
}

TEST_F(LspNodeMatchersTests, MatchImportSpecifier_MatchesImport)
{
    Initializer initializer = Initializer();
    const std::string source = R"(import { Foo } from './mod';)";
    es2panda_Context *ctx = initializer.CreateContext("import_spec_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    auto importSpec = ast->FindChild([](AstNode *node) {
        return node->Type() == AstNodeType::IMPORT_SPECIFIER && node->AsImportSpecifier()->Imported()->Name() == "Foo";
    });
    ASSERT_NE(importSpec, nullptr);

    NodeInfo info("Foo", AstNodeType::IMPORT_SPECIFIER);
    bool result = ark::es2panda::lsp::MatchImportSpecifier(importSpec, &info);
    EXPECT_TRUE(result);

    initializer.DestroyContext(ctx);
}

// Direct node-level seam vs upstream get_node_export_test.cpp API-level test;
// upstream alias/re-export branches not repeated.
TEST_F(LspNodeMatchersTests, ExtractExportSpecifierIdentifier_ExtractsName)
{
    Initializer initializer = Initializer();
    const std::string source = R"(export { Foo } from './mod';)";
    es2panda_Context *ctx = initializer.CreateContext("extract_export_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    auto exportDecl = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::REEXPORT_STATEMENT; });
    ASSERT_NE(exportDecl, nullptr);

    NodeInfo info("Foo", AstNodeType::REEXPORT_STATEMENT);
    auto *result = ark::es2panda::lsp::ExtractExportSpecifierIdentifier(exportDecl, &info);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->IsIdentifier());
    EXPECT_EQ(result->AsIdentifier()->Name(), "Foo");

    initializer.DestroyContext(ctx);
}

// Direct node-level seam vs upstream GetTsClassImplements; upstream namespace/
// generic implements lists not repeated.
TEST_F(LspNodeMatchersTests, ExtractTSClassImplementsIdentifier_ExtractsName)
{
    Initializer initializer = Initializer();
    const std::string source = R"(class MyClass implements MyInterface {})";
    es2panda_Context *ctx = initializer.CreateContext("implements_test.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);

    auto implements = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::TS_CLASS_IMPLEMENTS; });
    ASSERT_NE(implements, nullptr);

    NodeInfo info("MyInterface", AstNodeType::TS_CLASS_IMPLEMENTS);
    auto *result = ark::es2panda::lsp::ExtractTSClassImplementsIdentifier(implements, &info);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->IsIdentifier());
    EXPECT_EQ(result->AsIdentifier()->Name(), "MyInterface");

    initializer.DestroyContext(ctx);
}

// P1D2-NM-15: ExtractETSKeyofTypeIdentifier_ExtractsName - BLOCKED_DESIGN_MAPPING
// The extractor returns the parent node (TS_TYPE_ALIAS_DECLARATION), not the identifier.
// Current product cannot satisfy the design requirement of asserting the exact identifier.
// See node_matchers.cpp:300-315 - ExtractETSKeyofTypeIdentifier returns node->Parent() on success.
// The design requires: assert exact designed identifier without unsafe AsIdentifier.

TEST_F(LspNodeMatchersTests, GetNodeMatchers_ReturnsNonEmptyMap)
{
    const auto &matchers = ark::es2panda::lsp::GetNodeMatchers();
    EXPECT_FALSE(matchers.empty());

    EXPECT_NE(matchers.find(AstNodeType::CLASS_DEFINITION), matchers.end());
    EXPECT_NE(matchers.find(AstNodeType::IDENTIFIER), matchers.end());
    EXPECT_NE(matchers.find(AstNodeType::CLASS_PROPERTY), matchers.end());
    EXPECT_NE(matchers.find(AstNodeType::METHOD_DEFINITION), matchers.end());
    EXPECT_NE(matchers.find(AstNodeType::TS_ENUM_DECLARATION), matchers.end());
    EXPECT_NE(matchers.find(AstNodeType::TS_INTERFACE_DECLARATION), matchers.end());
    EXPECT_NE(matchers.find(AstNodeType::EXPORT_SPECIFIER), matchers.end());
    EXPECT_NE(matchers.find(AstNodeType::MEMBER_EXPRESSION), matchers.end());
    EXPECT_NE(matchers.find(AstNodeType::STRUCT_DECLARATION), matchers.end());
    EXPECT_NE(matchers.find(AstNodeType::CALL_EXPRESSION), matchers.end());
    EXPECT_NE(matchers.find(AstNodeType::IMPORT_SPECIFIER), matchers.end());
    EXPECT_EQ(matchers.size(), 38U);
}

TEST_F(LspNodeMatchersTests, GetNodeExtractors_ReturnsNonEmptyMap)
{
    const auto &extractors = ark::es2panda::lsp::GetNodeExtractors();
    EXPECT_FALSE(extractors.empty());

    EXPECT_NE(extractors.find(AstNodeType::CLASS_DEFINITION), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::IDENTIFIER), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::CLASS_PROPERTY), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::PROPERTY), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::METHOD_DEFINITION), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::CLASS_DECLARATION), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::TS_ENUM_DECLARATION), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::TS_ENUM_MEMBER), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::TS_INTERFACE_DECLARATION), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::TS_TYPE_ALIAS_DECLARATION), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::EXPORT_SPECIFIER), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::REEXPORT_STATEMENT), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::MEMBER_EXPRESSION), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::CALL_EXPRESSION), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::SUPER_EXPRESSION), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::AWAIT_EXPRESSION), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::TS_CLASS_IMPLEMENTS), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::ETS_STRING_LITERAL_TYPE), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::ETS_KEYOF_TYPE), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::TS_TYPE_REFERENCE), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::SCRIPT_FUNCTION), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::VARIABLE_DECLARATOR), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::VARIABLE_DECLARATION), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::ANNOTATION_DECLARATION), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::ANNOTATION_USAGE), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::BIGINT_LITERAL), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::IMPORT_SPECIFIER), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::IMPORT_DEFAULT_SPECIFIER), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::IMPORT_NAMESPACE_SPECIFIER), extractors.end());
    EXPECT_NE(extractors.find(AstNodeType::TS_TYPE_PARAMETER), extractors.end());
    EXPECT_EQ(extractors.size(), 30U);
}

TEST_F(LspNodeMatchersTests, GetNodeInfoHandlers_ReturnsNonEmptyMap)
{
    const auto &handlers = ark::es2panda::lsp::GetNodeInfoHandlers();
    EXPECT_FALSE(handlers.empty());

    EXPECT_NE(handlers.find(AstNodeType::IDENTIFIER), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::CLASS_DEFINITION), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::CLASS_PROPERTY), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::PROPERTY), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::VARIABLE_DECLARATION), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::VARIABLE_DECLARATOR), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::IMPORT_SPECIFIER), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::IMPORT_DEFAULT_SPECIFIER), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::IMPORT_NAMESPACE_SPECIFIER), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::TS_CLASS_IMPLEMENTS), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::MEMBER_EXPRESSION), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::TS_INTERFACE_DECLARATION), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::TS_TYPE_ALIAS_DECLARATION), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::SPREAD_ELEMENT), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::STRUCT_DECLARATION), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::CLASS_DECLARATION), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::SCRIPT_FUNCTION), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::FUNCTION_DECLARATION), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::METHOD_DEFINITION), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::TS_ENUM_DECLARATION), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::TS_ENUM_MEMBER), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::CALL_EXPRESSION), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::ANNOTATION_DECLARATION), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::AWAIT_EXPRESSION), handlers.end());
    EXPECT_NE(handlers.find(AstNodeType::ANNOTATION_USAGE), handlers.end());
    // Per requirement 3: do not pin a stale total count unless design explicitly requires it.
    // Design only requires non-empty and contains designed keys.
}

}  // namespace