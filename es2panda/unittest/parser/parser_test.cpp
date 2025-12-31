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

#include <parser/parserImpl.h>
#include <parser/program/program.h>
#include <es2panda.h>
#include <gtest/gtest.h>
#include <mem/pool_manager.h>
#include <ir/astNode.h>
#include <ir/statements/blockStatement.h>
#include <ir/statements/variableDeclaration.h>
#include <ir/statements/functionDeclaration.h>
#include <ir/statements/ifStatement.h>
#include <ir/statements/whileStatement.h>
#include <ir/statements/forUpdateStatement.h>
#include <ir/statements/forInStatement.h>
#include <ir/statements/forOfStatement.h>
#include <ir/statements/returnStatement.h>
#include <ir/statements/breakStatement.h>
#include <ir/statements/continueStatement.h>
#include <ir/statements/throwStatement.h>
#include <ir/statements/tryStatement.h>
#include <ir/statements/switchStatement.h>
#include <ir/statements/classDeclaration.h>
#include <ir/statements/expressionStatement.h>
#include <ir/statements/labelledStatement.h>
#include <ir/statements/doWhileStatement.h>
#include <ir/statements/emptyStatement.h>
#include <ir/statements/variableDeclarator.h>
#include <ir/expressions/literal.h>
#include <ir/expressions/literals/numberLiteral.h>
#include <ir/expressions/identifier.h>
#include <ir/expressions/binaryExpression.h>
#include <ir/expressions/unaryExpression.h>
#include <ir/expressions/callExpression.h>
#include <ir/expressions/memberExpression.h>
#include <ir/expressions/newExpression.h>
#include <ir/expressions/conditionalExpression.h>
#include <ir/expressions/assignmentExpression.h>
#include <ir/expressions/arrowFunctionExpression.h>
#include <ir/expressions/arrayExpression.h>
#include <ir/expressions/objectExpression.h>
#include <util/ustring.h>

namespace panda::es2panda::parser {

using mem::MemConfig;

class MemManager {
public:
    explicit MemManager()
    {
        constexpr auto COMPILER_SIZE = 8192_MB;

        MemConfig::Initialize(0, 0, COMPILER_SIZE, 0);
        PoolManager::Initialize(PoolType::MMAP);
    }

    NO_COPY_SEMANTIC(MemManager);
    NO_MOVE_SEMANTIC(MemManager);

    ~MemManager()
    {
        PoolManager::Finalize();
        MemConfig::Finalize();
    }
};

class ParserTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mm_ = std::make_unique<MemManager>();
    }

    void TearDown() override
    {
        mm_.reset();
    }

    Program ParseSource(const std::string &source, ScriptExtension ext = ScriptExtension::JS,
                        ScriptKind kind = ScriptKind::SCRIPT)
    {
        ParserImpl parser(ext);
        SourceFile sourceFile("test.js", "test", kind, ext);
        sourceFile.source = source;
        CompilerOptions options;
        return parser.Parse(sourceFile, options);
    }

    std::unique_ptr<MemManager> mm_;
};

// Test basic variable declaration parsing
TEST_F(ParserTest, TestVarDeclaration)
{
    auto program = ParseSource("var x = 42;");
    const int expectedStaSize = 1;
    const int expectedDeclSize = 1;
    const double expectedValue = 42.0;
    EXPECT_NE(program.Ast(), nullptr);
    EXPECT_EQ(program.Kind(), ScriptKind::SCRIPT);
    EXPECT_EQ(program.Extension(), ScriptExtension::JS);

    // Validate AST structure
    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_TRUE(ast->IsBlockStatement());
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    auto stmt = ast->Statements()[0];
    ASSERT_NE(stmt, nullptr);
    EXPECT_TRUE(stmt->IsVariableDeclaration());
    auto varDecl = stmt->AsVariableDeclaration();
    EXPECT_NE(varDecl, nullptr);
    EXPECT_EQ(varDecl->Kind(), ir::VariableDeclaration::VariableDeclarationKind::VAR);
    EXPECT_EQ(varDecl->Declarators().size(), expectedDeclSize);
    auto decl = varDecl->Declarators()[0];
    EXPECT_NE(decl, nullptr);
    auto id = decl->Id();
    EXPECT_NE(id, nullptr);
    EXPECT_TRUE(id->IsIdentifier());
    auto ident = decl->Id()->AsIdentifier();
    EXPECT_NE(ident, nullptr);
    EXPECT_EQ(ident->Name(), "x");
    auto init = decl->Init();
    EXPECT_NE(init, nullptr);
    EXPECT_TRUE(init->IsNumberLiteral());
    auto numLit = init->AsNumberLiteral();
    EXPECT_EQ(numLit->Number<double>(), expectedValue);
}

// Helper function to validate AST structure
void ValidateVariableDeclaration(const ir::Statement *stmt,
    ir::VariableDeclaration::VariableDeclarationKind expectedKind)
{
    ASSERT_NE(stmt, nullptr);
    EXPECT_TRUE(stmt->IsVariableDeclaration());
    auto *varDecl = stmt->AsVariableDeclaration();
    EXPECT_NE(varDecl, nullptr);
    EXPECT_EQ(varDecl->Kind(), expectedKind);
    EXPECT_FALSE(varDecl->Declarators().empty());
}

// Test let declaration
TEST_F(ParserTest, TestLetDeclaration)
{
    auto program = ParseSource("let x = 10;");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ValidateVariableDeclaration(ast->Statements()[0],
                                ir::VariableDeclaration::VariableDeclarationKind::LET);
}

// Test const declaration
TEST_F(ParserTest, TestConstDeclaration)
{
    auto program = ParseSource("const x = 20;");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ValidateVariableDeclaration(ast->Statements()[0],
                                ir::VariableDeclaration::VariableDeclarationKind::CONST);
}

// Test multiple variable declarations
TEST_F(ParserTest, TestMultipleVarDeclarations)
{
    auto program = ParseSource("var a = 1; var b = 2; var c = 3;");
    const int expectedStaSize = 3;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    for (const auto *stmt : ast->Statements()) {
        ValidateVariableDeclaration(stmt, ir::VariableDeclaration::VariableDeclarationKind::VAR);
    }
}

// Test function declaration
TEST_F(ParserTest, TestFunctionDeclaration)
{
    auto program = ParseSource("function foo() { return 42; }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsFunctionDeclaration());
    auto *funcDecl = ast->Statements()[0]->AsFunctionDeclaration();
    EXPECT_NE(funcDecl, nullptr);
    EXPECT_NE(funcDecl->Function(), nullptr);
}

// Test function with parameters
TEST_F(ParserTest, TestFunctionWithParameters)
{
    auto program = ParseSource("function add(a, b) { return a + b; }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsFunctionDeclaration());
    auto *funcDecl = ast->Statements()[0]->AsFunctionDeclaration();
    EXPECT_NE(funcDecl, nullptr);
    EXPECT_NE(funcDecl->Function(), nullptr);
}

// Test arrow function
TEST_F(ParserTest, TestArrowFunction)
{
    auto program = ParseSource("const add = (a, b) => a + b;");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsVariableDeclaration());
    auto *varDecl = ast->Statements()[0]->AsVariableDeclaration();
    EXPECT_NE(varDecl, nullptr);
    EXPECT_FALSE(varDecl->Declarators().empty());
}

// Test if statement
TEST_F(ParserTest, TestIfStatement)
{
    auto program = ParseSource("if (true) { var x = 1; }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);
    
    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsIfStatement());
    auto *ifStmt = ast->Statements()[0]->AsIfStatement();
    EXPECT_NE(ifStmt, nullptr);
    EXPECT_NE(ifStmt->Test(), nullptr);
    EXPECT_NE(ifStmt->Consequent(), nullptr);
}

// Test if-else statement
TEST_F(ParserTest, TestIfElseStatement)
{
    auto program = ParseSource("if (true) { var x = 1; } else { var x = 2; }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsIfStatement());
    auto *ifStmt = ast->Statements()[0]->AsIfStatement();
    EXPECT_NE(ifStmt, nullptr);
    EXPECT_NE(ifStmt->Alternate(), nullptr); // else branch exists
}

// Test while loop
TEST_F(ParserTest, TestWhileLoop)
{
    auto program = ParseSource("while (true) { break; }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsWhileStatement());
    auto *whileStmt = ast->Statements()[0]->AsWhileStatement();
    EXPECT_NE(whileStmt, nullptr);
    EXPECT_NE(whileStmt->Test(), nullptr);
    EXPECT_NE(whileStmt->Body(), nullptr);
}

// Test for loop
TEST_F(ParserTest, TestForLoop)
{
    auto program = ParseSource("for (var i = 0; i < 10; i++) { }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsForUpdateStatement());
    auto *forStmt = ast->Statements()[0]->AsForUpdateStatement();
    EXPECT_NE(forStmt, nullptr);
}

// Test for-in loop
TEST_F(ParserTest, TestForInLoop)
{
    auto program = ParseSource("for (var key in obj) { }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsForInStatement());
    auto *forInStmt = ast->Statements()[0]->AsForInStatement();
    EXPECT_NE(forInStmt, nullptr);
}

// Test for-of loop
TEST_F(ParserTest, TestForOfLoop)
{
    auto program = ParseSource("for (var item of arr) { }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsForOfStatement());
    auto *forOfStmt = ast->Statements()[0]->AsForOfStatement();
    EXPECT_NE(forOfStmt, nullptr);
}

// Test return statement
TEST_F(ParserTest, TestReturnStatement)
{
    auto program = ParseSource("function foo() { return 42; }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsFunctionDeclaration());
    auto *funcDecl = ast->Statements()[0]->AsFunctionDeclaration();
    EXPECT_NE(funcDecl, nullptr);
    EXPECT_NE(funcDecl->Function(), nullptr);
}

// Test break statement
TEST_F(ParserTest, TestBreakStatement)
{
    auto program = ParseSource("while (true) { break; }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    auto *whileStmt = ast->Statements()[0]->AsWhileStatement();
    ASSERT_NE(whileStmt, nullptr);
    EXPECT_NE(whileStmt->Body(), nullptr);
}

// Test continue statement
TEST_F(ParserTest, TestContinueStatement)
{
    auto program = ParseSource("while (true) { continue; }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    auto *whileStmt = ast->Statements()[0]->AsWhileStatement();
    ASSERT_NE(whileStmt, nullptr);
    EXPECT_NE(whileStmt->Body(), nullptr);
}

// Test throw statement
TEST_F(ParserTest, TestThrowStatement)
{
    auto program = ParseSource("throw new Error('test');");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsThrowStatement());
    auto *throwStmt = ast->Statements()[0]->AsThrowStatement();
    EXPECT_NE(throwStmt, nullptr);
    EXPECT_NE(throwStmt->Argument(), nullptr);
}

// Test try-catch statement
TEST_F(ParserTest, TestTryCatchStatement)
{
    auto program = ParseSource("try { } catch (e) { }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsTryStatement());
    auto *tryStmt = ast->Statements()[0]->AsTryStatement();
    EXPECT_NE(tryStmt, nullptr);
    EXPECT_NE(tryStmt->Block(), nullptr);
    EXPECT_NE(tryStmt->GetCatchClause(), nullptr); // catch clause
}

// Test try-catch-finally statement
TEST_F(ParserTest, TestTryCatchFinallyStatement)
{
    auto program = ParseSource("try { } catch (e) { } finally { }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsTryStatement());
    auto *tryStmt = ast->Statements()[0]->AsTryStatement();
    EXPECT_NE(tryStmt, nullptr);
    EXPECT_NE(tryStmt->FinallyBlock(), nullptr); // finally block
}

// Test switch statement
TEST_F(ParserTest, TestSwitchStatement)
{
    auto program = ParseSource("switch (x) { case 1: break; default: break; }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsSwitchStatement());
    auto *switchStmt = ast->Statements()[0]->AsSwitchStatement();
    EXPECT_NE(switchStmt, nullptr);
    EXPECT_NE(switchStmt->Discriminant(), nullptr);
}

// Test class declaration
TEST_F(ParserTest, TestClassDeclaration)
{
    auto program = ParseSource("class MyClass { }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsClassDeclaration());
    auto *classDecl = ast->Statements()[0]->AsClassDeclaration();
    EXPECT_NE(classDecl, nullptr);
    EXPECT_NE(classDecl->Definition(), nullptr);
}

// Test class with method
TEST_F(ParserTest, TestClassWithMethod)
{
    auto program = ParseSource("class MyClass { method() { } }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsClassDeclaration());
    auto *classDecl = ast->Statements()[0]->AsClassDeclaration();
    EXPECT_NE(classDecl, nullptr);
    EXPECT_NE(classDecl->Definition(), nullptr);
}

// Test class with constructor
TEST_F(ParserTest, TestClassWithConstructor)
{
    auto program = ParseSource("class MyClass { constructor() { } }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);
    
    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsClassDeclaration());
    auto *classDecl = ast->Statements()[0]->AsClassDeclaration();
    EXPECT_NE(classDecl, nullptr);
    EXPECT_NE(classDecl->Definition(), nullptr);
}

// Test class extends
TEST_F(ParserTest, TestClassExtends)
{
    auto program = ParseSource("class Child extends Parent { }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);
    
    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsClassDeclaration());
    auto *classDecl = ast->Statements()[0]->AsClassDeclaration();
    EXPECT_NE(classDecl, nullptr);
    EXPECT_NE(classDecl->Definition(), nullptr);
}

// Test object expression
TEST_F(ParserTest, TestObjectExpression)
{
    auto program = ParseSource("var obj = { a: 1, b: 2 };");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    EXPECT_TRUE(ast->Statements()[0]->IsVariableDeclaration());
}

// Test array expression
TEST_F(ParserTest, TestArrayExpression)
{
    auto program = ParseSource("var arr = [1, 2, 3];");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    EXPECT_TRUE(ast->Statements()[0]->IsVariableDeclaration());
}

// Test binary expressions
TEST_F(ParserTest, TestBinaryExpressions)
{
    auto program = ParseSource("var x = 1 + 2 * 3;");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    EXPECT_TRUE(ast->Statements()[0]->IsVariableDeclaration());
}

// Test unary expressions
TEST_F(ParserTest, TestUnaryExpressions)
{
    auto program = ParseSource("var x = -1; var y = !true;");
    const int expectedStaSize = 2;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    EXPECT_TRUE(ast->Statements()[0]->IsVariableDeclaration());
    EXPECT_TRUE(ast->Statements()[1]->IsVariableDeclaration());
}

// Test call expression
TEST_F(ParserTest, TestCallExpression)
{
    auto program = ParseSource("foo();");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsExpressionStatement());
    auto *exprStmt = ast->Statements()[0]->AsExpressionStatement();
    EXPECT_NE(exprStmt, nullptr);
    EXPECT_NE(exprStmt->GetExpression(), nullptr);
    EXPECT_TRUE(exprStmt->GetExpression()->IsCallExpression());
}

// Test call expression with arguments
TEST_F(ParserTest, TestCallExpressionWithArgs)
{
    auto program = ParseSource("foo(1, 2, 3);");
    const int expectedStaSize = 1;
    const int expectedArgSize = 3;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsExpressionStatement());
    auto *exprStmt = ast->Statements()[0]->AsExpressionStatement();
    EXPECT_NE(exprStmt, nullptr);
    EXPECT_NE(exprStmt->GetExpression(), nullptr);
    EXPECT_TRUE(exprStmt->GetExpression()->IsCallExpression());
    auto *callExpr = exprStmt->GetExpression()->AsCallExpression();
    EXPECT_NE(callExpr, nullptr);
    EXPECT_EQ(callExpr->Arguments().size(), expectedArgSize);
}

// Test member expression
TEST_F(ParserTest, TestMemberExpression)
{
    auto program = ParseSource("obj.prop;");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsExpressionStatement());
    auto *exprStmt = ast->Statements()[0]->AsExpressionStatement();
    EXPECT_NE(exprStmt, nullptr);
    EXPECT_NE(exprStmt->GetExpression(), nullptr);
    EXPECT_TRUE(exprStmt->GetExpression()->IsMemberExpression());
}

// Test computed member expression
TEST_F(ParserTest, TestComputedMemberExpression)
{
    auto program = ParseSource("obj['prop'];");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsExpressionStatement());
    auto *exprStmt = ast->Statements()[0]->AsExpressionStatement();
    EXPECT_NE(exprStmt, nullptr);
    EXPECT_NE(exprStmt->GetExpression(), nullptr);
    EXPECT_TRUE(exprStmt->GetExpression()->IsMemberExpression());
}

// Test new expression
TEST_F(ParserTest, TestNewExpression)
{
    auto program = ParseSource("new MyClass();");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsExpressionStatement());
    auto *exprStmt = ast->Statements()[0]->AsExpressionStatement();
    EXPECT_NE(exprStmt, nullptr);
    EXPECT_NE(exprStmt->GetExpression(), nullptr);
    EXPECT_TRUE(exprStmt->GetExpression()->IsNewExpression());
}

// Test conditional expression
TEST_F(ParserTest, TestConditionalExpression)
{
    auto program = ParseSource("var x = true ? 1 : 2;");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    EXPECT_TRUE(ast->Statements()[0]->IsVariableDeclaration());
}

// Test assignment expression
TEST_F(ParserTest, TestAssignmentExpression)
{
    auto program = ParseSource("x = 42;");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsExpressionStatement());
    auto *exprStmt = ast->Statements()[0]->AsExpressionStatement();
    EXPECT_NE(exprStmt, nullptr);
    EXPECT_NE(exprStmt->GetExpression(), nullptr);
    EXPECT_TRUE(exprStmt->GetExpression()->IsAssignmentExpression());
}

// Test compound assignment
TEST_F(ParserTest, TestCompoundAssignment)
{
    auto program = ParseSource("x += 1; x -= 1; x *= 2; x /= 2;");
    const int expectedStaSize = 4;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
}

// Test increment/decrement
TEST_F(ParserTest, TestIncrementDecrement)
{
    auto program = ParseSource("x++; ++x; x--; --x;");
    const int expectedStaSize = 4;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
}

// Test template literal
TEST_F(ParserTest, TestTemplateLiteral)
{
    auto program = ParseSource("var str = `hello ${name}`;");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test spread operator
TEST_F(ParserTest, TestSpreadOperator)
{
    auto program = ParseSource("var arr = [...other];");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test destructuring assignment
TEST_F(ParserTest, TestDestructuringAssignment)
{
    auto program = ParseSource("var [a, b] = [1, 2];");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test object destructuring
TEST_F(ParserTest, TestObjectDestructuring)
{
    auto program = ParseSource("var {a, b} = {a: 1, b: 2};");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test module export
TEST_F(ParserTest, TestModuleExport)
{
    auto program = ParseSource("export const x = 1;", ScriptExtension::JS, ScriptKind::MODULE);
    EXPECT_NE(program.Ast(), nullptr);
    EXPECT_EQ(program.Kind(), ScriptKind::MODULE);
}

// Test module import
TEST_F(ParserTest, TestModuleImport)
{
    auto program = ParseSource("import { x } from 'module';", ScriptExtension::JS, ScriptKind::MODULE);
    EXPECT_NE(program.Ast(), nullptr);
    EXPECT_EQ(program.Kind(), ScriptKind::MODULE);
}

// Test default export
TEST_F(ParserTest, TestDefaultExport)
{
    auto program = ParseSource("export default function() {};", ScriptExtension::JS, ScriptKind::MODULE);
    EXPECT_NE(program.Ast(), nullptr);
}

// Test parser extension detection
TEST_F(ParserTest, TestParserExtension)
{
    ParserImpl jsParser(ScriptExtension::JS);
    EXPECT_EQ(jsParser.Extension(), ScriptExtension::JS);

    ParserImpl tsParser(ScriptExtension::TS);
    EXPECT_EQ(tsParser.Extension(), ScriptExtension::TS);
}

// Test empty program
TEST_F(ParserTest, TestEmptyProgram)
{
    auto program = ParseSource("");
    const int expectedStaSize = 0;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_TRUE(ast->IsBlockStatement());
    // Empty program should have no statements or empty statements vector
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
}

// Test expression statement
TEST_F(ParserTest, TestExpressionStatement)
{
    auto program = ParseSource("1 + 2;");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsExpressionStatement());
    auto *exprStmt = ast->Statements()[0]->AsExpressionStatement();
    EXPECT_NE(exprStmt, nullptr);
    EXPECT_NE(exprStmt->GetExpression(), nullptr);
}

// Test labeled statement
TEST_F(ParserTest, TestLabeledStatement)
{
    auto program = ParseSource("label: while (true) { break label; }");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsLabelledStatement());
    auto *labelStmt = ast->Statements()[0]->AsLabelledStatement();
    EXPECT_NE(labelStmt, nullptr);
    EXPECT_NE(labelStmt->Body(), nullptr);
}

// Test do-while statement
TEST_F(ParserTest, TestDoWhileStatement)
{
    auto program = ParseSource("do { } while (true);");
    const int expectedStaSize = 1;
    EXPECT_NE(program.Ast(), nullptr);

    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize);
    ASSERT_NE(ast->Statements()[0], nullptr);
    EXPECT_TRUE(ast->Statements()[0]->IsDoWhileStatement());
    auto *doWhileStmt = ast->Statements()[0]->AsDoWhileStatement();
    EXPECT_NE(doWhileStmt, nullptr);
    EXPECT_NE(doWhileStmt->Body(), nullptr);
    EXPECT_NE(doWhileStmt->Test(), nullptr);
}

// Test empty statement
TEST_F(ParserTest, TestEmptyStatement)
{
    auto program = ParseSource(";;");
    const int expectedStaSize = 2;
    EXPECT_NE(program.Ast(), nullptr);
    
    auto *ast = program.Ast();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->Statements().size(), expectedStaSize); // Two empty statements
    for (const auto *stmt : ast->Statements()) {
        EXPECT_TRUE(stmt->IsEmptyStatement());
    }
}

// Test logical expressions
TEST_F(ParserTest, TestLogicalExpressions)
{
    auto program = ParseSource("var x = true && false; var y = true || false; var z = null ?? 'default';");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test comparison expressions
TEST_F(ParserTest, TestComparisonExpressions)
{
    auto program = ParseSource("var x = 1 < 2; var y = 1 > 2; var z = 1 === 2;");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test typeof expression
TEST_F(ParserTest, TestTypeofExpression)
{
    auto program = ParseSource("var x = typeof obj;");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test instanceof expression
TEST_F(ParserTest, TestInstanceofExpression)
{
    auto program = ParseSource("var x = obj instanceof MyClass;");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test in expression
TEST_F(ParserTest, TestInExpression)
{
    auto program = ParseSource("var x = 'prop' in obj;");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test void expression
TEST_F(ParserTest, TestVoidExpression)
{
    auto program = ParseSource("void 0;");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test delete expression
TEST_F(ParserTest, TestDeleteExpression)
{
    auto program = ParseSource("delete obj.prop;");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test this expression
TEST_F(ParserTest, TestThisExpression)
{
    auto program = ParseSource("function foo() { return this; }");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test super expression
TEST_F(ParserTest, TestSuperExpression)
{
    auto program = ParseSource("class Child extends Parent { method() { super.method(); } }");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test yield expression
TEST_F(ParserTest, TestYieldExpression)
{
    auto program = ParseSource("function* gen() { yield 1; }");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test await expression
TEST_F(ParserTest, TestAwaitExpression)
{
    auto program = ParseSource("async function foo() { await promise; }");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test optional chaining
TEST_F(ParserTest, TestOptionalChaining)
{
    auto program = ParseSource("obj?.prop; obj?.['key']; func?.();");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test nullish coalescing
TEST_F(ParserTest, TestNullishCoalescing)
{
    auto program = ParseSource("var x = a ?? b;");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test parser error handling - syntax error
TEST_F(ParserTest, TestSyntaxError)
{
    EXPECT_THROW(ParseSource("var x = ;"), es2panda::Error);
}

// Test parser error handling - unexpected token
TEST_F(ParserTest, TestUnexpectedToken)
{
    EXPECT_THROW(ParseSource("var x = 1 2;"), es2panda::Error);
}

// Test parser error handling - missing semicolon (should not throw in some cases)
TEST_F(ParserTest, TestMissingSemicolon)
{
    // Some missing semicolons are allowed by ASI (Automatic Semicolon Insertion)
    auto program = ParseSource("var x = 1\nvar y = 2");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test complex nested structures
TEST_F(ParserTest, TestComplexNestedStructures)
{
    auto program = ParseSource(
        "function outer() {"
        "  function inner() {"
        "    if (true) {"
        "      for (var i = 0; i < 10; i++) {"
        "        try {"
        "          return i;"
        "        } catch (e) {"
        "          throw e;"
        "        }"
        "      }"
        "    }"
        "  }"
        "  return inner();"
        "}");
    EXPECT_NE(program.Ast(), nullptr);
}

// Test parser with TypeScript extension
TEST_F(ParserTest, TestTypeScriptExtension)
{
    ParserImpl parser(ScriptExtension::TS);
    EXPECT_EQ(parser.Extension(), ScriptExtension::TS);
}

// Test parser allocator
TEST_F(ParserTest, TestParserAllocator)
{
    ParserImpl parser(ScriptExtension::JS);
    EXPECT_NE(parser.Allocator(), nullptr);
}

// Test stack limit functionality
TEST_F(ParserTest, TestStackLimit)
{
    ParserImpl parser(ScriptExtension::JS);
    uintptr_t limit = 1000;
    parser.SetStackLimit(limit);
    EXPECT_EQ(parser.StackLimit(), limit);
}

// Test isDtsFile detection
TEST_F(ParserTest, TestIsDtsFile)
{
    ParserImpl parser(ScriptExtension::TS);
    SourceFile sourceFile("test.d.ts", "test", ScriptKind::SCRIPT, ScriptExtension::TS);
    sourceFile.source = "declare const x: number;";
    CompilerOptions options;
    auto program = parser.Parse(sourceFile, options);
    // After parsing, the program should recognize it as a .d.ts file
    EXPECT_TRUE(program.IsDtsFile());
}

}  // namespace panda::es2panda::parser
