/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "ast_verifier_test.h"
#include "libarkbase/macros.h"
#include "util/ast-builders/classDefinitionBuilder.h"
#include "util/ast-builders/binaryExpressionBuilder.h"
#include "util/ast-builders/classPropertyBuilder.h"
#include "util/ast-builders/identifierBuilder.h"
#include "util/ast-builders/numberLiteralBuilder.h"
#include "util/ast-builders/awaitExpressionBuilder.h"
#include "util/ast-builders/bigintLiteralBuilder.h"
#include "util/ast-builders/blockExpressionBuilder.h"
#include "util/ast-builders/blockStatementBuilder.h"
#include "util/ast-builders/booleanLiteralBuilder.h"
#include "util/ast-builders/breakStatementBuilder.h"
#include "util/ast-builders/callExpressionBuilder.h"
#include "util/ast-builders/superExpressionBuilder.h"
#include "util/ast-builders/charLiteralBuilder.h"
#include "util/ast-builders/classDeclarationBuilder.h"
#include "util/ast-builders/etsTypeReferenceBuilder.h"
#include "util/ast-builders/etsTypeReferencePartBuilder.h"
#include "util/ast-builders/exportDefaultDeclarationBuilder.h"
#include "util/ast-builders/expressionStatementBuilder.h"
#include "util/ast-builders/ifStatementBuilder.h"
#include "util/ast-builders/importDeclarationBuilder.h"
#include "util/ast-builders/importSpecifierBuilder.h"
#include "util/ast-builders/memberExpressionBuilder.h"
#include "util/ast-builders/thisExpressionBuilder.h"
#include "util/ast-builders/methodDefinitionBuilder.h"
#include "util/ast-builders/functionExpressionBuilder.h"
#include "util/ast-builders/scriptFunctionBuilder.h"
#include "util/ast-builders/stringLiteralBuilder.h"
#include "util/ast-builders/switchStatementBuilder.h"
#include "util/ast-builders/tsClassImplementsBuilder.h"
#include "util/ast-builders/tsEnumDeclarationBuilder.h"
#include "util/ast-builders/tsEnumMemberBuilder.h"
#include "util/ast-builders/tsTypeParameterInstantiationBuilder.h"
#include "util/ast-builders/unaryExpressionBuilder.h"
#include "util/ast-builders/undefinedLiteralBuilder.h"
#include "util/ast-builders/updateExpressionBuilder.h"
#include "util/ast-builders/variableDeclarationBuilder.h"
#include "util/ast-builders/variableDeclaratorBuilder.h"
#include "util/ast-builders/whileStatementBuilder.h"
#include "util/ast-builders/switchCaseStatementBuilder.h"

#include <gtest/gtest.h>

using ark::es2panda::ir::AwaitExpressionBuilder;
using ark::es2panda::ir::BigIntLiteralBuilder;
using ark::es2panda::ir::BinaryExpressionBuilder;
using ark::es2panda::ir::BlockExpressionBuilder;
using ark::es2panda::ir::BlockStatementBuilder;
using ark::es2panda::ir::BooleanLiteralBuilder;
using ark::es2panda::ir::BreakStatementBuilder;
using ark::es2panda::ir::CallExpressionBuilder;
using ark::es2panda::ir::CharLiteralBuilder;
using ark::es2panda::ir::ClassDeclarationBuilder;
using ark::es2panda::ir::ClassDefinitionBuilder;
using ark::es2panda::ir::ClassPropertyBuilder;
using ark::es2panda::ir::ETSTypeReferenceBuilder;
using ark::es2panda::ir::ETSTypeReferencePartBuilder;
using ark::es2panda::ir::ExportDefaultDeclarationBuilder;
using ark::es2panda::ir::ExpressionStatementBuilder;
using ark::es2panda::ir::FunctionExpressionBuilder;
using ark::es2panda::ir::IdentifierBuilder;
using ark::es2panda::ir::IfStatementBuilder;
using ark::es2panda::ir::ImportDeclarationBuilder;
using ark::es2panda::ir::ImportSpecifierBuilder;
using ark::es2panda::ir::MemberExpressionBuilder;
using ark::es2panda::ir::MethodDefinitionBuilder;
using ark::es2panda::ir::NumberLiteralBuilder;
using ark::es2panda::ir::ScriptFunctionBuilder;
using ark::es2panda::ir::StringLiteralBuilder;
using ark::es2panda::ir::SuperExpressionBuilder;
using ark::es2panda::ir::SwitchCaseStatementBuilder;
using ark::es2panda::ir::SwitchStatementBuilder;
using ark::es2panda::ir::ThisExpressionBuilder;
using ark::es2panda::ir::TSClassImplementsBuilder;
using ark::es2panda::ir::TSEnumDeclarationBuilder;
using ark::es2panda::ir::TSEnumMemberBuilder;
using ark::es2panda::ir::TSTypeParameterInstantiationBuilder;
using ark::es2panda::ir::UnaryExpressionBuilder;
using ark::es2panda::ir::UndefinedLiteralBuilder;
using ark::es2panda::ir::UpdateExpressionBuilder;
using ark::es2panda::ir::VariableDeclarationBuilder;
using ark::es2panda::ir::VariableDeclaratorBuilder;
using ark::es2panda::ir::WhileStatementBuilder;

// NOLINTBEGIN(readability-magic-numbers)
namespace {
TEST_F(ASTVerifierTest, arrowFunctionExpressionBuild)
{
    auto left = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(10)).Build();
    auto right = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(5)).Build();
    auto binaryExpr = BinaryExpressionBuilder(Allocator())
                          .SetLeft(left)
                          .SetRight(right)
                          .SetOperator(ark::es2panda::lexer::TokenType::PUNCTUATOR_PLUS)
                          .Build();
    auto awaitExpr = AwaitExpressionBuilder(Allocator()).SetArgument(binaryExpr).Build();
    ASSERT_TRUE(awaitExpr->IsAwaitExpression());
}

TEST_F(ASTVerifierTest, awaitExpressionBuild)
{
    auto left = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(10)).Build();
    auto right = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(5)).Build();
    auto binaryExpr = BinaryExpressionBuilder(Allocator())
                          .SetLeft(left)
                          .SetRight(right)
                          .SetOperator(ark::es2panda::lexer::TokenType::PUNCTUATOR_PLUS)
                          .Build();
    auto awaitExpr = AwaitExpressionBuilder(Allocator()).SetArgument(binaryExpr).Build();
    ASSERT_TRUE(awaitExpr->IsAwaitExpression());
}

TEST_F(ASTVerifierTest, bigintLiteralBuild)
{
    auto bigint = BigIntLiteralBuilder(Allocator()).SetValue("123").Build();
    ASSERT_TRUE(bigint->IsBigIntLiteral());
}

TEST_F(ASTVerifierTest, binaryExpressionBuild)
{
    auto left = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(10)).Build();
    auto right = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(5)).Build();
    auto binaryExpr = BinaryExpressionBuilder(Allocator())
                          .SetLeft(left)
                          .SetRight(right)
                          .SetOperator(ark::es2panda::lexer::TokenType::PUNCTUATOR_PLUS)
                          .Build();
    ASSERT_TRUE(binaryExpr->IsBinaryExpression());
}

TEST_F(ASTVerifierTest, blockExpressionBuild)
{
    auto left = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(10)).Build();
    auto right = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(5)).Build();
    auto binaryExpr = BinaryExpressionBuilder(Allocator())
                          .SetLeft(left)
                          .SetRight(right)
                          .SetOperator(ark::es2panda::lexer::TokenType::PUNCTUATOR_PLUS)
                          .Build();
    auto exprStmnt = ExpressionStatementBuilder(Allocator()).SetExpression(binaryExpr).Build();
    auto blockExpr = BlockExpressionBuilder(Allocator()).AddStatement(exprStmnt).Build();
    ASSERT_TRUE(blockExpr->IsBlockExpression());
}

TEST_F(ASTVerifierTest, blockStatementBuild)
{
    auto left = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(10)).Build();
    auto right = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(5)).Build();
    auto binaryExpr = BinaryExpressionBuilder(Allocator())
                          .SetLeft(left)
                          .SetRight(right)
                          .SetOperator(ark::es2panda::lexer::TokenType::PUNCTUATOR_PLUS)
                          .Build();
    auto exprStmnt = ExpressionStatementBuilder(Allocator()).SetExpression(binaryExpr).Build();
    auto blockStmnt = BlockStatementBuilder(Allocator()).AddStatement(exprStmnt).Build();
    ASSERT_TRUE(blockStmnt->IsBlockStatement());
}

TEST_F(ASTVerifierTest, booleanLiteralBuild)
{
    auto boolLiteral = BooleanLiteralBuilder(Allocator()).SetValue(true).Build();
    ASSERT_TRUE(boolLiteral->IsBooleanLiteral());
}

TEST_F(ASTVerifierTest, breakStatementBuild)
{
    auto breakStmnt = BreakStatementBuilder(Allocator()).Build();
    ASSERT_TRUE(breakStmnt->IsBreakStatement());
}

TEST_F(ASTVerifierTest, callExpressionBuild)
{
    auto superExpr = SuperExpressionBuilder(Allocator()).Build();
    auto callExpr = CallExpressionBuilder(Allocator()).SetCallee(superExpr).Build();
    ASSERT_TRUE(callExpr->IsCallExpression());
}

TEST_F(ASTVerifierTest, charLiteralBuild)
{
    // Create char literal with default value u0000
    auto charLiteral = CharLiteralBuilder(Allocator()).Build();
    ASSERT_TRUE(charLiteral->IsCharLiteral());
}

TEST_F(ASTVerifierTest, classDeclarationBuild)
{
    auto id = IdentifierBuilder(Allocator()).SetName("a").Build();
    auto number = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(10)).Build();
    auto classProperty = ClassPropertyBuilder(Allocator())
                             .SetKey(id)
                             .SetValue(number)
                             .AddModifier(ark::es2panda::ir::ModifierFlags::PRIVATE)
                             .Build();

    auto classId = IdentifierBuilder(Allocator()).SetName("A").Build();
    auto classDef = ClassDefinitionBuilder(Allocator()).SetIdentifier(classId).AddProperty(classProperty).Build();
    auto classDecl = ClassDeclarationBuilder(Allocator()).SetDefinition(classDef).Build();
    ASSERT_TRUE(classDecl->IsClassDeclaration());
}

TEST_F(ASTVerifierTest, etsTypeReferenceBuild)
{
    auto id = IdentifierBuilder(Allocator()).SetName("a").Build();
    auto etsTypeReferencePart =
        ETSTypeReferencePartBuilder(Allocator()).SetName(id).SetTypeParams(nullptr).SetPrev(nullptr).Build();
    auto etsTypeReference = ETSTypeReferenceBuilder(Allocator()).SetETSTypeReferencePart(etsTypeReferencePart).Build();
    ASSERT_TRUE(etsTypeReference->IsETSTypeReference());
}

TEST_F(ASTVerifierTest, exportDefaultDeclarationBuild)
{
    auto idParent = IdentifierBuilder(Allocator()).SetName("parent").Build();
    auto id = IdentifierBuilder(Allocator()).SetName("a").SetParent<IdentifierBuilder>(idParent).Build();
    auto number = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(10)).Build();
    auto classProperty = ClassPropertyBuilder(Allocator())
                             .SetKey(id)
                             .SetValue(number)
                             .AddModifier(ark::es2panda::ir::ModifierFlags::PRIVATE)
                             .Build();

    auto classId = IdentifierBuilder(Allocator()).SetName("A").Build();
    auto classDef = ClassDefinitionBuilder(Allocator()).SetIdentifier(classId).AddProperty(classProperty).Build();
    auto classDecl = ClassDeclarationBuilder(Allocator()).SetDefinition(classDef).Build();
    auto exportDefaultDecl =
        ExportDefaultDeclarationBuilder(Allocator()).SetDeclaration(classDecl).SetExportEquals(true).Build();
    ASSERT_TRUE(exportDefaultDecl->IsExportDefaultDeclaration());
}

TEST_F(ASTVerifierTest, expressionStatementBuild)
{
    auto left = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(10)).Build();
    auto right = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(5)).Build();
    auto binaryExpr = BinaryExpressionBuilder(Allocator())
                          .SetLeft(left)
                          .SetRight(right)
                          .SetOperator(ark::es2panda::lexer::TokenType::PUNCTUATOR_PLUS)
                          .Build();
    auto exprStmnt = ExpressionStatementBuilder(Allocator()).SetExpression(binaryExpr).Build();
    ASSERT_TRUE(exprStmnt->IsExpressionStatement());
}

TEST_F(ASTVerifierTest, ifStatementBuild)
{
    auto left = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(10)).Build();
    auto right = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(5)).Build();
    auto binaryExpr = BinaryExpressionBuilder(Allocator())
                          .SetLeft(left)
                          .SetRight(right)
                          .SetOperator(ark::es2panda::lexer::TokenType::PUNCTUATOR_PLUS)
                          .Build();
    auto consStmnt = ExpressionStatementBuilder(Allocator()).SetExpression(binaryExpr).Build();
    auto alterStmnt = ExpressionStatementBuilder(Allocator()).SetExpression(binaryExpr).Build();
    auto ifStmnt =
        IfStatementBuilder(Allocator()).SetTest(binaryExpr).SetConsequent(consStmnt).SetAlternate(alterStmnt).Build();
    ASSERT_TRUE(ifStmnt->IsIfStatement());
}

TEST_F(ASTVerifierTest, memberExpressionBuild)
{
    auto member = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(10)).Build();
    auto thisExpr = ThisExpressionBuilder(Allocator()).Build();
    auto memberExpression = MemberExpressionBuilder(Allocator())
                                .SetObject(thisExpr)
                                .SetProperty(member)
                                .SetKind(ark::es2panda::ir::MemberExpressionKind::PROPERTY_ACCESS)
                                .Build();
    ASSERT_TRUE(memberExpression->IsMemberExpression());
}

TEST_F(ASTVerifierTest, methodDefinitionBuild)
{
    auto id = IdentifierBuilder(Allocator()).SetName("a").Build();
    ark::ArenaVector<ark::es2panda::ir::Expression *> params(Allocator()->Adapter());
    auto signature = ark::es2panda::ir::FunctionSignature(nullptr, std::move(params), nullptr);
    auto func = ScriptFunctionBuilder(Allocator()).SetSignature(&signature).Build();
    auto funcExpr = FunctionExpressionBuilder(Allocator()).SetFunction(func).Build();
    auto methodDef = MethodDefinitionBuilder(Allocator())
                         .SetKey(id)
                         .SetMethodDefinitionKind(ark::es2panda::ir::MethodDefinitionKind::METHOD)
                         .SetModifierFlags(ark::es2panda::ir::ModifierFlags::PUBLIC)
                         .SetValue(funcExpr)
                         .Build();
    ASSERT_TRUE(methodDef->IsMethodDefinition());
}

TEST_F(ASTVerifierTest, stringLiteralBuild)
{
    auto stringLiteral = StringLiteralBuilder(Allocator()).SetValue("qwe").Build();
    ASSERT_TRUE(stringLiteral->IsStringLiteral());
}

TEST_F(ASTVerifierTest, tsClassImplementsBuild)
{
    auto left = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(10)).Build();
    auto right = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(5)).Build();
    auto binaryExpr = BinaryExpressionBuilder(Allocator())
                          .SetLeft(left)
                          .SetRight(right)
                          .SetOperator(ark::es2panda::lexer::TokenType::PUNCTUATOR_PLUS)
                          .Build();
    ark::ArenaVector<ark::es2panda::ir::TypeNode *> types(Allocator()->Adapter());
    auto typeParams = TSTypeParameterInstantiationBuilder(Allocator()).SetParams(std::move(types)).Build();
    auto tsClassImpl =
        TSClassImplementsBuilder(Allocator()).SetExpression(binaryExpr).SetTypeParameters(typeParams).Build();
    ASSERT_TRUE(tsClassImpl->IsTSClassImplements());
}

TEST_F(ASTVerifierTest, switchStatementBuild)
{
    auto left = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(10)).Build();
    auto right = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(5)).Build();
    auto binaryExpr = BinaryExpressionBuilder(Allocator())
                          .SetLeft(left)
                          .SetRight(right)
                          .SetOperator(ark::es2panda::lexer::TokenType::PUNCTUATOR_PLUS)
                          .Build();
    auto consStmnt = ExpressionStatementBuilder(Allocator()).SetExpression(binaryExpr).Build();

    auto thisExpr = ThisExpressionBuilder(Allocator()).Build();
    ark::ArenaVector<ark::es2panda::ir::SwitchCaseStatement *> cases(Allocator()->Adapter());
    ark::ArenaVector<ark::es2panda::ir::Statement *> conseq(Allocator()->Adapter());
    conseq.push_back(consStmnt);
    auto switchCase = SwitchCaseStatementBuilder(Allocator()).SetTest(binaryExpr).SetConsequent(conseq).Build();
    cases.push_back(switchCase);
    auto switchStmnt = SwitchStatementBuilder(Allocator()).SetCases(cases).SetDiscriminant(thisExpr).Build();
    ASSERT_TRUE(switchStmnt->IsSwitchStatement());
}

TEST_F(ASTVerifierTest, enumBuild)
{
    auto id = IdentifierBuilder(Allocator()).SetName("a").Build();
    auto idMember = IdentifierBuilder(Allocator()).SetName("s").Build();
    auto init = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(10)).Build();
    auto enumMember = TSEnumMemberBuilder(Allocator()).SetKey(idMember).SetInit(init).Build();
    auto enumDecl = TSEnumDeclarationBuilder(Allocator())
                        .SetKey(id)
                        .SetIsConst(false)
                        .SetIsDeclare(true)
                        .SetIsConst(false)
                        .AddMember(enumMember)
                        .Build();
    ASSERT_TRUE(enumDecl->IsTSEnumDeclaration());
}

TEST_F(ASTVerifierTest, unaryExpressionBuild)
{
    auto id = IdentifierBuilder(Allocator()).SetName("a").Build();
    auto unaryExpr = UnaryExpressionBuilder(Allocator())
                         .SetArgument(id)
                         .SetOperator(ark::es2panda::lexer::TokenType::PUNCTUATOR_MINUS)
                         .Build();
    ASSERT_TRUE(unaryExpr->IsUnaryExpression());
}

TEST_F(ASTVerifierTest, undefinedLiteralBuild)
{
    auto undefLiteral = UndefinedLiteralBuilder(Allocator()).Build();
    ASSERT_TRUE(undefLiteral->IsUndefinedLiteral());
}

TEST_F(ASTVerifierTest, updateExpressionBuild)
{
    auto id = IdentifierBuilder(Allocator()).SetName("a").Build();
    auto updateExpr = UpdateExpressionBuilder(Allocator())
                          .SetArgument(id)
                          .SetOperator(ark::es2panda::lexer::TokenType::PUNCTUATOR_MINUS_MINUS)
                          .SetIsPrefix(true)
                          .Build();
    ASSERT_TRUE(updateExpr->IsUpdateExpression());
}

TEST_F(ASTVerifierTest, VariableDeclarationBuild)
{
    auto idDeclarator = IdentifierBuilder(Allocator()).SetName("a").Build();
    auto init = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(10)).Build();
    auto varDeclarator = VariableDeclaratorBuilder(Allocator()).SetId(idDeclarator).SetInit(init).Build();
    ark::ArenaVector<ark::es2panda::ir::VariableDeclarator *> declarators(Allocator()->Adapter());
    declarators.push_back(varDeclarator);
    auto varDecl = VariableDeclarationBuilder(Allocator())
                       .SetDeclarators(std::move(declarators))
                       .SetKind(ark::es2panda::ir::VariableDeclaration::VariableDeclarationKind::LET)
                       .Build();
    ASSERT_TRUE(varDecl->IsVariableDeclaration());
}

TEST_F(ASTVerifierTest, whileStatementBuild)
{
    auto left = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(10)).Build();
    auto right = NumberLiteralBuilder(Allocator()).SetValue(ark::es2panda::lexer::Number(5)).Build();
    auto binaryExpr = BinaryExpressionBuilder(Allocator())
                          .SetLeft(left)
                          .SetRight(right)
                          .SetOperator(ark::es2panda::lexer::TokenType::PUNCTUATOR_PLUS)
                          .Build();
    auto exprStmnt = ExpressionStatementBuilder(Allocator()).SetExpression(binaryExpr).Build();
    auto whileStmnt = WhileStatementBuilder(Allocator()).SetTest(binaryExpr).SetBody(exprStmnt).Build();
    ASSERT_TRUE(whileStmnt->IsWhileStatement());
}

TEST_F(ASTVerifierTest, multipleImportTest)
{
    auto importedIdent = IdentifierBuilder(Allocator()).SetName("A").Build();
    auto localIdent = IdentifierBuilder(Allocator()).SetName("A").Build();
    auto stringLiteral = StringLiteralBuilder(Allocator()).SetValue("test").Build();
    auto importSpecifier = ImportSpecifierBuilder(Allocator()).SetImported(importedIdent).SetLocal(localIdent).Build();
    auto importDecl =
        ImportDeclarationBuilder(Allocator()).SetSource(stringLiteral).AddSpecifier(importSpecifier).Build();
    ASSERT_TRUE(importDecl->IsImportDeclaration());

    auto classId = IdentifierBuilder(Allocator()).SetName("B").Build();
    auto classDef = ClassDefinitionBuilder(Allocator()).SetIdentifier(classId).SetSuperClass(localIdent).Build();
    auto classDecl = ClassDeclarationBuilder(Allocator()).SetDefinition(classDef).Build();
    ASSERT_TRUE(classDecl->IsClassDeclaration());
}
}  // anonymous namespace
// NOLINTEND(readability-magic-numbers)
