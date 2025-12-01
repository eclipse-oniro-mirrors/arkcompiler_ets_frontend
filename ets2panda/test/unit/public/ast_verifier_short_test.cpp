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
#include "checker/ETSchecker.h"
#include "es2panda.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/literals/numberLiteral.h"
#include "ir/expressions/literals/booleanLiteral.h"
#include "libarkbase/macros.h"
#include "varbinder/ETSBinder.h"
#include "util/diagnosticEngine.h"

namespace {

using ark::es2panda::ScriptExtension;
using ark::es2panda::checker::ETSChecker;
using ark::es2panda::compiler::ast_verifier::ArithmeticOperationValid;
using ark::es2panda::compiler::ast_verifier::NodeHasParent;
using ark::es2panda::compiler::ast_verifier::NodeHasSourceRange;
using ark::es2panda::compiler::ast_verifier::NodeHasType;
using ark::es2panda::compiler::ast_verifier::NoPrimitiveTypes;
using ark::es2panda::compiler::ast_verifier::SequenceExpressionHasLastType;
using ark::es2panda::compiler::ast_verifier::VariableHasEnclosingScope;
using ark::es2panda::compiler::ast_verifier::VariableHasScope;
using ark::es2panda::compiler::ast_verifier::VariableNameIdentifierNameSame;
using ark::es2panda::ir::BinaryExpression;
using ark::es2panda::ir::BooleanLiteral;
using ark::es2panda::ir::Expression;
using ark::es2panda::ir::Identifier;
using ark::es2panda::ir::NumberLiteral;
using ark::es2panda::ir::SequenceExpression;
using ark::es2panda::ir::StringLiteral;
using ark::es2panda::lexer::Number;
using ark::es2panda::lexer::TokenType;
using ark::es2panda::util::DiagnosticEngine;
using ark::es2panda::util::StringView;
using ark::es2panda::varbinder::FunctionScope;
using ark::es2panda::varbinder::LetDecl;
using ark::es2panda::varbinder::LocalScope;
using ark::es2panda::varbinder::LocalVariable;
using ark::es2panda::varbinder::VariableFlags;

TEST_F(ASTVerifierTest, DISABLED_NullParent)  // #31976
{
    StringLiteral emptyNode;
    EXPECT_TRUE(VerifyNode<NodeHasParent>(&emptyNode, ExpectVerifierMessage {"NULL_PARENT"}));
}

TEST_F(ASTVerifierTest, DISABLED_NullRange)  // #31976
{
    StringLiteral emptyNode;
    EXPECT_TRUE(VerifyNode<NodeHasSourceRange>(&emptyNode, ExpectVerifierMessage {"NULL_RANGE"}));
}

// NOTE(dkofanov): #22355 'NodeHasType' is broken.
TEST_F(ASTVerifierTest, DISABLED_NullType)  // #31976
{
    StringLiteral emptyNode;
    EXPECT_TRUE(VerifyNode<NodeHasType>(&emptyNode, ExpectVerifierMessage {"NULL_TS_TYPE"}));
}

TEST_F(ASTVerifierTest, DISABLED_WithoutScope)  // #31976
{
    StringLiteral emptyNode;
    EXPECT_TRUE(VerifyNode<VariableHasScope>(&emptyNode));
}

TEST_F(ASTVerifierTest, DISABLED_ScopeTest)  // #31976
{
    Identifier ident(StringView("var_decl"), Allocator());
    LetDecl decl("test", &ident);
    LocalVariable local(&decl, VariableFlags::LOCAL);
    ident.SetVariable(&local);

    LocalScope scope(Allocator(), nullptr);
    FunctionScope parentScope(Allocator(), nullptr);
    scope.SetParent(&parentScope);
    scope.AddDecl(Allocator(), &decl, ScriptExtension::ETS);
    scope.BindNode(&ident);

    local.SetScope(&scope);

    EXPECT_TRUE(VerifyNode<VariableHasScope>(&ident));
}

TEST_F(ASTVerifierTest, DISABLED_ScopeNodeTest)  // #31976
{
    Identifier ident(StringView("var_decl"), Allocator());
    LetDecl decl("test", &ident);
    LocalVariable local(&decl, VariableFlags::LOCAL);
    ident.SetVariable(&local);

    LocalScope scope(Allocator(), nullptr);
    FunctionScope parentScope(Allocator(), nullptr);
    scope.SetParent(&parentScope);
    scope.AddDecl(Allocator(), &decl, ScriptExtension::ETS);
    scope.BindNode(&ident);
    parentScope.BindNode(&ident);

    local.SetScope(&scope);

    EXPECT_TRUE(VerifyNode<VariableHasEnclosingScope>(&ident));
}

TEST_F(ASTVerifierTest, DISABLED_ArithmeticExpressionCorrect1)  // #31976
{
    DiagnosticEngine de {};
    ETSChecker etschecker {Allocator(), de};

    auto left = NumberLiteral(Number {1});
    auto right = NumberLiteral(Number {6});
    auto arithmeticExpression = BinaryExpression(&left, &right, TokenType::PUNCTUATOR_PLUS);

    left.SetTsType(etschecker.GlobalIntType());
    right.SetTsType(etschecker.GlobalIntType());

    EXPECT_TRUE(VerifyNode<ArithmeticOperationValid>(arithmeticExpression.AsBinaryExpression()));
}

TEST_F(ASTVerifierTest, DISABLED_ArithmeticExpressionCorrect2)  // #31976
{
    DiagnosticEngine de {};
    ETSChecker etschecker {Allocator(), de};

    constexpr uint32_t LEFT1_PARAM = 1;
    constexpr uint32_t LEFT2_PARAM = 12;
    constexpr uint32_t RIGHT2_PARAM = 6;
    auto left1 = NumberLiteral(Number {LEFT1_PARAM});
    auto left2 = NumberLiteral(Number {LEFT2_PARAM});
    auto right2 = NumberLiteral(Number {RIGHT2_PARAM});
    auto right1 = BinaryExpression(&left2, &right2, TokenType::PUNCTUATOR_MULTIPLY);
    auto arithmeticExpression = BinaryExpression(&left1, &right1, TokenType::PUNCTUATOR_PLUS);

    left1.SetTsType(etschecker.GlobalIntType());
    right1.SetTsType(etschecker.GlobalIntType());
    left2.SetTsType(etschecker.GlobalIntType());
    right2.SetTsType(etschecker.GlobalIntType());

    EXPECT_TRUE(VerifyNode<ArithmeticOperationValid>(arithmeticExpression.AsBinaryExpression()));
}

TEST_F(ASTVerifierTest, DISABLED_ArithmeticExpressionNegative1)  // #31976
{
    DiagnosticEngine de {};
    ETSChecker etschecker {Allocator(), de};

    const StringView leftParam("1");
    constexpr uint32_t RIGHT_PARAM = 1;
    auto left = StringLiteral(leftParam);
    auto right = NumberLiteral(Number {RIGHT_PARAM});
    auto arithmeticExpression = BinaryExpression(&left, &right, TokenType::PUNCTUATOR_DIVIDE);

    left.SetTsType(etschecker.GlobalETSStringLiteralType());
    right.SetTsType(etschecker.GlobalIntType());

    EXPECT_TRUE(VerifyNode<ArithmeticOperationValid>(arithmeticExpression.AsBinaryExpression(),
                                                     ExpectVerifierMessage {"Not a numeric type"}));
}

TEST_F(ASTVerifierTest, DISABLED_ArithmeticExpressionNegative2)  // #31976
{
    DiagnosticEngine de {};
    ETSChecker etschecker {Allocator(), de};

    auto left = BooleanLiteral(true);
    auto right = NumberLiteral(Number {1});
    auto arithmeticExpression = BinaryExpression(&left, &right, TokenType::PUNCTUATOR_DIVIDE);

    left.SetTsType(etschecker.GlobalETSStringLiteralType());
    right.SetTsType(etschecker.GlobalIntType());

    EXPECT_TRUE(VerifyNode<ArithmeticOperationValid>(arithmeticExpression.AsBinaryExpression(),
                                                     ExpectVerifierMessage {"Not a numeric type"}));
}

TEST_F(ASTVerifierTest, DISABLED_PrimitiveType)  // #31976
{
    DiagnosticEngine de {};
    ETSChecker etschecker {Allocator(), de};

    auto ast = BooleanLiteral(true);
    ast.SetTsType(etschecker.CreateETSBooleanType(true));

    ASSERT_TRUE(VerifyNode<NoPrimitiveTypes>(&ast, ExpectVerifierMessage {"PRIMITIVE_BEFORE_LOWERING"}));
    Get<NoPrimitiveTypes>()->SetNumberLoweringOccured();
    ASSERT_TRUE(VerifyNode<NoPrimitiveTypes>(&ast));
    Get<NoPrimitiveTypes>()->SetNumberLoweringOccured(false);
}

TEST_F(ASTVerifierTest, DISABLED_SequenceExpressionType)  // #31976
{
    auto de = DiagnosticEngine();
    auto checker = ETSChecker(Allocator(), de);
    const auto literalsCount = 3;
    std::array<NumberLiteral, literalsCount> literals {NumberLiteral {Number {1}}, NumberLiteral {Number {2}},
                                                       NumberLiteral {Number {3}}};
    ark::es2panda::ArenaVector<Expression *> expressions {Allocator()->Adapter()};
    expressions.insert(expressions.end(), {&literals[0], &literals[1], &literals[2]});
    SequenceExpression sequenceExpression {std::move(expressions)};

    literals[literalsCount - 1].SetTsType(checker.GlobalIntType());
    sequenceExpression.SetTsType(checker.GlobalIntType());

    EXPECT_TRUE(VerifyNode<SequenceExpressionHasLastType>(&sequenceExpression));
}

TEST_F(ASTVerifierTest, VariableNameIdentifierNameSameNegative)
{
    char const *text = R"(
        function main(): void {
            let tmp = 1;
            let lambda2: (value: int) => Int = (value: int): Int => {
                let a = 42;
                let num: Int = new Int(a + value);
                return num;
            }
            let n_tmp = tmp + 2;
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        // Note(@kirillbychkov): Change Identifier name in variable lambda2
        GetAst()
            ->AsETSModule()
            ->Statements()[0]
            ->AsClassDeclaration()
            ->Definition()
            ->AsClassDefinition()
            ->Body()[0]
            ->AsClassElement()
            ->Value()
            ->AsFunctionExpression()
            ->Function()
            ->AsScriptFunction()
            ->Body()
            ->AsBlockStatement()
            ->Statements()[1]
            ->AsVariableDeclaration()
            ->Declarators()[0]
            ->AsVariableDeclarator()
            ->Id()
            ->AsIdentifier()
            ->SetName("not_name");

        EXPECT_TRUE(Verify<VariableNameIdentifierNameSame>(ExpectVerifierMessage {"IDENTIFIER_NAME_DIFFERENCE"}));
    }
}

TEST_F(ASTVerifierTest, VariableNameIdentifierNameSame)
{
    char const *text = R"(
        function main(): void {
            let tmp = 1;
            let lambda2: (value: int) => Int = (value: int): Int => {
                let a = 42;
                let num: Int = new Int(a + value);
                return num;
            }
            let n_tmp = tmp + 2;
        }
    )";

    CONTEXT(ES2PANDA_STATE_CHECKED, text)
    {
        EXPECT_TRUE(Verify<VariableNameIdentifierNameSame>());
    }
}

}  // namespace
