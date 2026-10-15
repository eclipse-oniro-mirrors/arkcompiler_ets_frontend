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
#include <vector>
#include <string>

namespace {
using ::NodeInfo;
using ark::es2panda::ir::AstNode;
using ark::es2panda::ir::AstNodeType;
using ark::es2panda::lsp::Initializer;

class LspNodeMatchersCompleteTests : public LSPAPITests {};

// P2B-NMC-01: MatchProperty_MatchesProperty
TEST_F(LspNodeMatchersCompleteTests, MatchProperty_MatchesProperty)
{
    Initializer initializer = Initializer();
    // Object literal property (not class property) to produce AstNodeType::PROPERTY
    const std::string source = R"(let obj = { prop: 42 };)";
    es2panda_Context *ctx = initializer.CreateContext("nmc01.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto prop = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::PROPERTY; });
    ASSERT_NE(prop, nullptr);
    NodeInfo info("prop", AstNodeType::PROPERTY);
    EXPECT_TRUE(ark::es2panda::lsp::MatchProperty(prop, &info));
    NodeInfo wrongInfo("other", AstNodeType::PROPERTY);
    EXPECT_FALSE(ark::es2panda::lsp::MatchProperty(prop, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-02: MatchTSEnumMember_MatchesEnumMember
TEST_F(LspNodeMatchersCompleteTests, MatchTSEnumMember_MatchesEnumMember)
{
    Initializer initializer = Initializer();
    const std::string source = R"(enum E { A, B })";
    es2panda_Context *ctx = initializer.CreateContext("nmc02.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto member = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::TS_ENUM_MEMBER; });
    ASSERT_NE(member, nullptr);
    NodeInfo info("A", AstNodeType::TS_ENUM_MEMBER);
    EXPECT_TRUE(ark::es2panda::lsp::MatchTSEnumMember(member, &info));
    NodeInfo wrongInfo("B", AstNodeType::TS_ENUM_MEMBER);
    EXPECT_FALSE(ark::es2panda::lsp::MatchTSEnumMember(member, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-03: MatchTSTypeAliasDeclaration_MatchesTypeAlias
TEST_F(LspNodeMatchersCompleteTests, MatchTSTypeAliasDeclaration_MatchesTypeAlias)
{
    Initializer initializer = Initializer();
    const std::string source = R"(type MyType = number;)";
    es2panda_Context *ctx = initializer.CreateContext("nmc03.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto alias = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::TS_TYPE_ALIAS_DECLARATION; });
    ASSERT_NE(alias, nullptr);
    NodeInfo info("MyType", AstNodeType::TS_TYPE_ALIAS_DECLARATION);
    EXPECT_TRUE(ark::es2panda::lsp::MatchTSTypeAliasDeclaration(alias, &info));
    NodeInfo wrongInfo("Other", AstNodeType::TS_TYPE_ALIAS_DECLARATION);
    EXPECT_FALSE(ark::es2panda::lsp::MatchTSTypeAliasDeclaration(alias, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-04: MatchEtsStringLiteralType_MatchesStringLiteralType
TEST_F(LspNodeMatchersCompleteTests, MatchEtsStringLiteralType_MatchesStringLiteralType)
{
    Initializer initializer = Initializer();
    const std::string source = R"(class C { s: "hello" = "hello"; })";
    es2panda_Context *ctx = initializer.CreateContext("nmc04.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto slt = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::ETS_STRING_LITERAL_TYPE; });
    ASSERT_NE(slt, nullptr);
    NodeInfo info("s", AstNodeType::ETS_STRING_LITERAL_TYPE);
    EXPECT_TRUE(ark::es2panda::lsp::MatchEtsStringLiteralType(slt, &info));
    NodeInfo wrongInfo("other", AstNodeType::ETS_STRING_LITERAL_TYPE);
    EXPECT_FALSE(ark::es2panda::lsp::MatchEtsStringLiteralType(slt, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-05: MatchEtsTypeReference_MatchesTypeReference
TEST_F(LspNodeMatchersCompleteTests, MatchEtsTypeReference_MatchesTypeReference)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x: number = 1;)";
    es2panda_Context *ctx = initializer.CreateContext("nmc05.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto typeRef = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::ETS_TYPE_REFERENCE; });
    ASSERT_NE(typeRef, nullptr);
    NodeInfo info("number", AstNodeType::ETS_TYPE_REFERENCE);
    EXPECT_TRUE(ark::es2panda::lsp::MatchEtsTypeReference(typeRef, &info));
    NodeInfo wrongInfo("string", AstNodeType::ETS_TYPE_REFERENCE);
    EXPECT_FALSE(ark::es2panda::lsp::MatchEtsTypeReference(typeRef, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-06: MatchEtsKeyofType_MatchesKeyofType
TEST_F(LspNodeMatchersCompleteTests, MatchEtsKeyofType_MatchesKeyofType)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x: keyof number = 1;)";
    es2panda_Context *ctx = initializer.CreateContext("nmc06.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto keyof = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::ETS_KEYOF_TYPE; });
    ASSERT_NE(keyof, nullptr);
    NodeInfo info("number", AstNodeType::ETS_KEYOF_TYPE);
    EXPECT_TRUE(ark::es2panda::lsp::MatchEtsKeyofType(keyof, &info));
    NodeInfo wrongInfo("string", AstNodeType::ETS_KEYOF_TYPE);
    EXPECT_FALSE(ark::es2panda::lsp::MatchEtsKeyofType(keyof, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-07: MatchEtsNewClassInstanceExpression_MatchesNewExpr
TEST_F(LspNodeMatchersCompleteTests, MatchEtsNewClassInstanceExpression_MatchesNewExpr)
{
    Initializer initializer = Initializer();
    const std::string source = R"(class C {} let c = new C();)";
    es2panda_Context *ctx = initializer.CreateContext("nmc07.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto newExpr =
        ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::ETS_NEW_CLASS_INSTANCE_EXPRESSION; });
    ASSERT_NE(newExpr, nullptr);
    NodeInfo info("C", AstNodeType::ETS_NEW_CLASS_INSTANCE_EXPRESSION);
    EXPECT_TRUE(ark::es2panda::lsp::MatchEtsNewClassInstanceExpression(newExpr, &info));
    NodeInfo wrongInfo("Other", AstNodeType::ETS_NEW_CLASS_INSTANCE_EXPRESSION);
    EXPECT_FALSE(ark::es2panda::lsp::MatchEtsNewClassInstanceExpression(newExpr, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-08: MatchSpreadElement_MatchesSpread
TEST_F(LspNodeMatchersCompleteTests, MatchSpreadElement_MatchesSpread)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let a = [1, 2]; let b = [...a];)";
    es2panda_Context *ctx = initializer.CreateContext("nmc08.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto spread = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::SPREAD_ELEMENT; });
    ASSERT_NE(spread, nullptr);
    NodeInfo info("a", AstNodeType::SPREAD_ELEMENT);
    EXPECT_TRUE(ark::es2panda::lsp::MatchSpreadElement(spread, &info));
    NodeInfo wrongInfo("b", AstNodeType::SPREAD_ELEMENT);
    EXPECT_FALSE(ark::es2panda::lsp::MatchSpreadElement(spread, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-09: MatchTsTypeReference_MatchesTsTypeReference
TEST_F(LspNodeMatchersCompleteTests, MatchTsTypeReference_MatchesTsTypeReference)
{
    Initializer initializer = Initializer();
    // MatchTsTypeReference checks IsETSTypeReference internally, so we search for ETS_TYPE_REFERENCE
    const std::string source = R"(let x: number = 1;)";
    es2panda_Context *ctx = initializer.CreateContext("nmc09.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto typeRef = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::ETS_TYPE_REFERENCE; });
    ASSERT_NE(typeRef, nullptr);
    NodeInfo info("number", AstNodeType::TS_TYPE_REFERENCE);
    EXPECT_TRUE(ark::es2panda::lsp::MatchTsTypeReference(typeRef, &info));
    NodeInfo wrongInfo("string", AstNodeType::TS_TYPE_REFERENCE);
    EXPECT_FALSE(ark::es2panda::lsp::MatchTsTypeReference(typeRef, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-10: MatchScriptFunction_MatchesScriptFunction
TEST_F(LspNodeMatchersCompleteTests, MatchScriptFunction_MatchesScriptFunction)
{
    Initializer initializer = Initializer();
    const std::string source = R"(function foo(): void {})";
    es2panda_Context *ctx = initializer.CreateContext("nmc10.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto scriptFunc = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::SCRIPT_FUNCTION; });
    ASSERT_NE(scriptFunc, nullptr);
    NodeInfo info("foo", AstNodeType::SCRIPT_FUNCTION);
    EXPECT_TRUE(ark::es2panda::lsp::MatchScriptFunction(scriptFunc, &info));
    NodeInfo wrongInfo("bar", AstNodeType::SCRIPT_FUNCTION);
    EXPECT_FALSE(ark::es2panda::lsp::MatchScriptFunction(scriptFunc, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-11: MatchVariableDeclarator_MatchesDeclarator
TEST_F(LspNodeMatchersCompleteTests, MatchVariableDeclarator_MatchesDeclarator)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x = 42;)";
    es2panda_Context *ctx = initializer.CreateContext("nmc11.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto decl = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::VARIABLE_DECLARATOR; });
    ASSERT_NE(decl, nullptr);
    NodeInfo info("x", AstNodeType::VARIABLE_DECLARATOR);
    EXPECT_TRUE(ark::es2panda::lsp::MatchVariableDeclarator(decl, &info));
    NodeInfo wrongInfo("y", AstNodeType::VARIABLE_DECLARATOR);
    EXPECT_FALSE(ark::es2panda::lsp::MatchVariableDeclarator(decl, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-12: MatchVariableDeclaration_MatchesDeclaration
TEST_F(LspNodeMatchersCompleteTests, MatchVariableDeclaration_MatchesDeclaration)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x = 42;)";
    es2panda_Context *ctx = initializer.CreateContext("nmc12.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto varDecl = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::VARIABLE_DECLARATION; });
    ASSERT_NE(varDecl, nullptr);
    NodeInfo info("x", AstNodeType::VARIABLE_DECLARATION);
    EXPECT_TRUE(ark::es2panda::lsp::MatchVariableDeclaration(varDecl, &info));
    NodeInfo wrongInfo("y", AstNodeType::VARIABLE_DECLARATION);
    EXPECT_FALSE(ark::es2panda::lsp::MatchVariableDeclaration(varDecl, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-13: MatchClassDeclaration_MatchesClassDeclaration
TEST_F(LspNodeMatchersCompleteTests, MatchClassDeclaration_MatchesClassDeclaration)
{
    Initializer initializer = Initializer();
    const std::string source = R"(class MyClass {})";
    es2panda_Context *ctx = initializer.CreateContext("nmc13.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto classDecl = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::CLASS_DECLARATION; });
    ASSERT_NE(classDecl, nullptr);
    NodeInfo info("MyClass", AstNodeType::CLASS_DECLARATION);
    EXPECT_TRUE(ark::es2panda::lsp::MatchClassDeclaration(classDecl, &info));
    NodeInfo wrongInfo("Other", AstNodeType::CLASS_DECLARATION);
    EXPECT_FALSE(ark::es2panda::lsp::MatchClassDeclaration(classDecl, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-14: MatchAnnotationDeclaration_MatchesAnnotationDecl
TEST_F(LspNodeMatchersCompleteTests, MatchAnnotationDeclaration_MatchesAnnotationDecl)
{
    Initializer initializer = Initializer();
    const std::string source = R"(@interface Annot {})";
    es2panda_Context *ctx = initializer.CreateContext("nmc14.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto annotDecl = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::ANNOTATION_DECLARATION; });
    ASSERT_NE(annotDecl, nullptr);
    NodeInfo info("Annot", AstNodeType::ANNOTATION_DECLARATION);
    EXPECT_TRUE(ark::es2panda::lsp::MatchAnnotationDeclaration(annotDecl, &info));
    NodeInfo wrongInfo("Other", AstNodeType::ANNOTATION_DECLARATION);
    EXPECT_FALSE(ark::es2panda::lsp::MatchAnnotationDeclaration(annotDecl, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-15: MatchAnnotationUsage_MatchesAnnotationUsage
TEST_F(LspNodeMatchersCompleteTests, MatchAnnotationUsage_MatchesAnnotationUsage)
{
    Initializer initializer = Initializer();
    const std::string source = R"(@interface Annot {} @Annot class C {})";
    es2panda_Context *ctx = initializer.CreateContext("nmc15.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto annotUse = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::ANNOTATION_USAGE; });
    ASSERT_NE(annotUse, nullptr);
    NodeInfo info("Annot", AstNodeType::ANNOTATION_USAGE);
    EXPECT_TRUE(ark::es2panda::lsp::MatchAnnotationUsage(annotUse, &info));
    NodeInfo wrongInfo("Other", AstNodeType::ANNOTATION_USAGE);
    EXPECT_FALSE(ark::es2panda::lsp::MatchAnnotationUsage(annotUse, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-16: MatchAwaitExpression_MatchesAwait
TEST_F(LspNodeMatchersCompleteTests, MatchAwaitExpression_MatchesAwait)
{
    Initializer initializer = Initializer();
    const std::string source = R"(async function foo() { await bar; })";
    es2panda_Context *ctx = initializer.CreateContext("nmc16.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto awaitExpr = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::AWAIT_EXPRESSION; });
    ASSERT_NE(awaitExpr, nullptr);
    NodeInfo info("bar", AstNodeType::AWAIT_EXPRESSION);
    EXPECT_TRUE(ark::es2panda::lsp::MatchAwaitExpression(awaitExpr, &info));
    NodeInfo wrongInfo("other", AstNodeType::AWAIT_EXPRESSION);
    EXPECT_FALSE(ark::es2panda::lsp::MatchAwaitExpression(awaitExpr, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-17: MatchBigIntLiteral_MatchesBigInt
TEST_F(LspNodeMatchersCompleteTests, MatchBigIntLiteral_MatchesBigInt)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x = 100n;)";
    es2panda_Context *ctx = initializer.CreateContext("nmc17.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto bigint = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::BIGINT_LITERAL; });
    ASSERT_NE(bigint, nullptr);
    NodeInfo info("100", AstNodeType::BIGINT_LITERAL);
    EXPECT_TRUE(ark::es2panda::lsp::MatchBigIntLiteral(bigint, &info));
    NodeInfo wrongInfo("200", AstNodeType::BIGINT_LITERAL);
    EXPECT_FALSE(ark::es2panda::lsp::MatchBigIntLiteral(bigint, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-18: MatchImportDefaultSpecifier_MatchesDefaultImport
TEST_F(LspNodeMatchersCompleteTests, MatchImportDefaultSpecifier_MatchesDefaultImport)
{
    Initializer initializer = Initializer();
    const std::string source = R"(import foo from './mod';)";
    es2panda_Context *ctx = initializer.CreateContext("nmc18.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto defSpec = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::IMPORT_DEFAULT_SPECIFIER; });
    ASSERT_NE(defSpec, nullptr);
    NodeInfo info("foo", AstNodeType::IMPORT_DEFAULT_SPECIFIER);
    EXPECT_TRUE(ark::es2panda::lsp::MatchImportDefaultSpecifier(defSpec, &info));
    NodeInfo wrongInfo("bar", AstNodeType::IMPORT_DEFAULT_SPECIFIER);
    EXPECT_FALSE(ark::es2panda::lsp::MatchImportDefaultSpecifier(defSpec, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-19: MatchImportNamespaceSpecifier_MatchesNamespaceImport
TEST_F(LspNodeMatchersCompleteTests, MatchImportNamespaceSpecifier_MatchesNamespaceImport)
{
    Initializer initializer = Initializer();
    const std::string source = R"(import * as ns from './mod';)";
    es2panda_Context *ctx = initializer.CreateContext("nmc19.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto nsSpec = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::IMPORT_NAMESPACE_SPECIFIER; });
    ASSERT_NE(nsSpec, nullptr);
    NodeInfo info("ns", AstNodeType::IMPORT_NAMESPACE_SPECIFIER);
    EXPECT_TRUE(ark::es2panda::lsp::MatchImportNamespaceSpecifier(nsSpec, &info));
    NodeInfo wrongInfo("other", AstNodeType::IMPORT_NAMESPACE_SPECIFIER);
    EXPECT_FALSE(ark::es2panda::lsp::MatchImportNamespaceSpecifier(nsSpec, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-20: MatchTSTypeParameter_MatchesTypeParameter
TEST_F(LspNodeMatchersCompleteTests, MatchTSTypeParameter_MatchesTypeParameter)
{
    Initializer initializer = Initializer();
    const std::string source = R"(function foo<T>(x: T): void {})";
    es2panda_Context *ctx = initializer.CreateContext("nmc20.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto tp = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::TS_TYPE_PARAMETER; });
    ASSERT_NE(tp, nullptr);
    NodeInfo info("T", AstNodeType::TS_TYPE_PARAMETER);
    EXPECT_TRUE(ark::es2panda::lsp::MatchTSTypeParameter(tp, &info));
    NodeInfo wrongInfo("U", AstNodeType::TS_TYPE_PARAMETER);
    EXPECT_FALSE(ark::es2panda::lsp::MatchTSTypeParameter(tp, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-21: MatchSwitchStatement_MatchesSwitch
TEST_F(LspNodeMatchersCompleteTests, MatchSwitchStatement_MatchesSwitch)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x = 1; switch (x) { case 1: break; })";
    es2panda_Context *ctx = initializer.CreateContext("nmc21.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto sw = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::SWITCH_STATEMENT; });
    ASSERT_NE(sw, nullptr);
    NodeInfo info("x", AstNodeType::SWITCH_STATEMENT);
    EXPECT_TRUE(ark::es2panda::lsp::MatchSwitchStatement(sw, &info));
    NodeInfo wrongInfo("y", AstNodeType::SWITCH_STATEMENT);
    EXPECT_FALSE(ark::es2panda::lsp::MatchSwitchStatement(sw, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-22: MatchEtsParameterExpression_MatchesParamExpr
TEST_F(LspNodeMatchersCompleteTests, MatchEtsParameterExpression_MatchesParamExpr)
{
    Initializer initializer = Initializer();
    const std::string source = R"(function foo(p: number): void {})";
    es2panda_Context *ctx = initializer.CreateContext("nmc22.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto param = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::ETS_PARAMETER_EXPRESSION; });
    ASSERT_NE(param, nullptr);
    NodeInfo info("p", AstNodeType::ETS_PARAMETER_EXPRESSION);
    EXPECT_TRUE(ark::es2panda::lsp::MatchEtsParameterExpression(param, &info));
    NodeInfo wrongInfo("q", AstNodeType::ETS_PARAMETER_EXPRESSION);
    EXPECT_FALSE(ark::es2panda::lsp::MatchEtsParameterExpression(param, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-23: MatchTsNonNullExpression_MatchesNonNull
TEST_F(LspNodeMatchersCompleteTests, MatchTsNonNullExpression_MatchesNonNull)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x: number | undefined = 1; let y = x!;)";
    es2panda_Context *ctx = initializer.CreateContext("nmc23.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto nonnull = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::TS_NON_NULL_EXPRESSION; });
    ASSERT_NE(nonnull, nullptr);
    NodeInfo info("x", AstNodeType::TS_NON_NULL_EXPRESSION);
    EXPECT_TRUE(ark::es2panda::lsp::MatchTsNonNullExpression(nonnull, &info));
    NodeInfo wrongInfo("y", AstNodeType::TS_NON_NULL_EXPRESSION);
    EXPECT_FALSE(ark::es2panda::lsp::MatchTsNonNullExpression(nonnull, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-24: MatchFunctionDeclaration_MatchesFunctionDecl
TEST_F(LspNodeMatchersCompleteTests, MatchFunctionDeclaration_MatchesFunctionDecl)
{
    Initializer initializer = Initializer();
    const std::string source = R"(function foo(): void {})";
    es2panda_Context *ctx = initializer.CreateContext("nmc24.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto funcDecl = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::FUNCTION_DECLARATION; });
    ASSERT_NE(funcDecl, nullptr);
    NodeInfo info("foo", AstNodeType::FUNCTION_DECLARATION);
    EXPECT_TRUE(ark::es2panda::lsp::MatchFunctionDeclaration(funcDecl, &info));
    NodeInfo wrongInfo("bar", AstNodeType::FUNCTION_DECLARATION);
    EXPECT_FALSE(ark::es2panda::lsp::MatchFunctionDeclaration(funcDecl, &wrongInfo));
    initializer.DestroyContext(ctx);
}

// P2B-NMC-25: HandleIdentifier_ReturnsInfo
// GATED_HANDLER_KINDS excludes IDENTIFIER from the public node-info path;
// this direct call is the only execution path.
TEST_F(LspNodeMatchersCompleteTests, HandleIdentifier_ReturnsInfo)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x = 42;)";
    es2panda_Context *ctx = initializer.CreateContext("nmc25.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto ident = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::IDENTIFIER; });
    ASSERT_NE(ident, nullptr);
    std::vector<NodeInfo> result;
    ark::es2panda::lsp::HandleIdentifier(ident, result);
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0].name, "x");
    EXPECT_EQ(result[0].kind, AstNodeType::IDENTIFIER);
    initializer.DestroyContext(ctx);
}

// P2B-NMC-26: HandleMemberExpression_ReturnsInfo
TEST_F(LspNodeMatchersCompleteTests, HandleMemberExpression_ReturnsInfo)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x = {a: 1}; let y = x.a;)";
    es2panda_Context *ctx = initializer.CreateContext("nmc26.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto member = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::MEMBER_EXPRESSION; });
    ASSERT_NE(member, nullptr);
    std::vector<NodeInfo> result;
    ark::es2panda::lsp::HandleMemberExpression(member, result);
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0].name, "a");
    EXPECT_EQ(result[0].kind, AstNodeType::MEMBER_EXPRESSION);
    initializer.DestroyContext(ctx);
}

// P2B-NMC-27: HandleSpeadeElement_ReturnsInfo
TEST_F(LspNodeMatchersCompleteTests, HandleSpeadeElement_ReturnsInfo)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let a = [1]; let b = [...a];)";
    es2panda_Context *ctx = initializer.CreateContext("nmc27.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto spread = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::SPREAD_ELEMENT; });
    ASSERT_NE(spread, nullptr);
    std::vector<NodeInfo> result;
    ark::es2panda::lsp::HandleSpeadeElement(spread, result);
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0].name, "a");
    EXPECT_EQ(result[0].kind, AstNodeType::SPREAD_ELEMENT);
    initializer.DestroyContext(ctx);
}

// P2B-NMC-28: HandleTSEnumMember_ReturnsInfo
TEST_F(LspNodeMatchersCompleteTests, HandleTSEnumMember_ReturnsInfo)
{
    Initializer initializer = Initializer();
    const std::string source = R"(enum E { A, B })";
    es2panda_Context *ctx = initializer.CreateContext("nmc28.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto member = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::TS_ENUM_MEMBER; });
    ASSERT_NE(member, nullptr);
    std::vector<NodeInfo> result;
    ark::es2panda::lsp::HandleTSEnumMember(member, result);
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0].name, "A");
    EXPECT_EQ(result[0].kind, AstNodeType::TS_ENUM_MEMBER);
    initializer.DestroyContext(ctx);
}

// P2B-NMC-29: HandleCallExpression_ReturnsInfo
// Member-callee branch (obj.method()) needs its own fixture; this pins the
// plain callee arm.
TEST_F(LspNodeMatchersCompleteTests, HandleCallExpression_ReturnsInfo)
{
    Initializer initializer = Initializer();
    const std::string source = R"(function foo() {} foo();)";
    es2panda_Context *ctx = initializer.CreateContext("nmc29.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto call = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::CALL_EXPRESSION; });
    ASSERT_NE(call, nullptr);
    std::vector<NodeInfo> result;
    ark::es2panda::lsp::HandleCallExpression(call, result);
    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0].name, "foo");
    EXPECT_EQ(result[0].kind, AstNodeType::CALL_EXPRESSION);
    initializer.DestroyContext(ctx);
}

// P2B-NMC-30: ExtractIdentifierFromNode_ExtractsIdentifier
TEST_F(LspNodeMatchersCompleteTests, ExtractIdentifierFromNode_ExtractsIdentifier)
{
    Initializer initializer = Initializer();
    const std::string source = R"(let x = 42;)";
    es2panda_Context *ctx = initializer.CreateContext("nmc30.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto ast = GetAstFromContext<AstNode>(ctx);
    ASSERT_NE(ast, nullptr);
    auto ident = ast->FindChild([](AstNode *node) { return node->Type() == AstNodeType::IDENTIFIER; });
    ASSERT_NE(ident, nullptr);
    NodeInfo info("x", AstNodeType::IDENTIFIER);
    auto *extracted = ark::es2panda::lsp::ExtractIdentifierFromNode(ident, &info);
    ASSERT_NE(extracted, nullptr);
    EXPECT_TRUE(extracted->IsIdentifier());
    EXPECT_EQ(extracted->AsIdentifier()->Name(), "x");
    // null node returns nullptr
    EXPECT_EQ(ark::es2panda::lsp::ExtractIdentifierFromNode(nullptr, &info), nullptr);
    initializer.DestroyContext(ctx);
}

}  // namespace