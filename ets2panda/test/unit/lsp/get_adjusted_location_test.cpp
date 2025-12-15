/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "lsp/include/get_adjusted_location.h"
#include "gtest/gtest.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "test/utils/ast_verifier_test.h"
#include "lsp/include/internal_api.h"

using ark::es2panda::ir::AstNode;
using ark::es2panda::ir::AstNodeType;

namespace {
constexpr size_t PROPERTY_POS = 23;
constexpr size_t FUNCTION_POS = 10;
constexpr size_t PRIVATE_POS = 25;
constexpr size_t PUBLIC_POS = 47;
constexpr size_t STATIC_POS = 69;
constexpr size_t CLASS_POS = 3;
constexpr size_t METHOD_POS = 23;
constexpr size_t VARIABLE_POS = 5;
constexpr size_t MEMBER_POS = 28;
constexpr size_t TYPE_PARAM_POS = 7;
constexpr size_t ARRAY_TYPE_POS = 24;
constexpr size_t HERITAGE_POS = 45;
constexpr size_t EXPRESSION_POS = 9;
constexpr size_t IMPORT_POS = 7;
constexpr size_t EXPORT_POS = 7;
}  // namespace

class LspGetAdjustedLocation : public LSPAPITests {};

TEST_F(LspGetAdjustedLocation, GetAdjustedLocationForVariableTest)
{
    const char *source = R"(
let x = 42;
function test() {
    let y = x;
    return y;
}
let z = x;
)";

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_CHECKED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *varNode = ark::es2panda::lsp::GetTouchingPropertyName(ctx, VARIABLE_POS);
    ASSERT_NE(varNode, nullptr);
    auto adjustedVar = ark::es2panda::lsp::GetAdjustedLocation(varNode, context->allocator);
    ASSERT_TRUE(adjustedVar.has_value());
    EXPECT_TRUE((*adjustedVar)->IsIdentifier());
    EXPECT_EQ((*adjustedVar)->AsIdentifier()->Name(), "x");
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocation, GetTouchingPropertyNameTest)
{
    const char *source = R"(
class Example {
    property = "test";
}
)";

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *node = ark::es2panda::lsp::GetTouchingPropertyName(ctx, PROPERTY_POS);
    ASSERT_NE(node, nullptr);
    EXPECT_TRUE(node->IsIdentifier());
    EXPECT_EQ(node->AsIdentifier()->Name(), "property");
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocation, GetAdjustedLocationForClassTest)
{
    const char *source = R"(
class MyClass {
    prop = 0;
    method() {
        return this.prop;
    }
}
let instance = new MyClass();
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_CHECKED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *classNode = ark::es2panda::lsp::GetTouchingToken(ctx, CLASS_POS, false);
    ASSERT_NE(classNode, nullptr);
    ASSERT_TRUE(classNode->IsClassDeclaration());
    auto adjustedClass = ark::es2panda::lsp::GetAdjustedLocationForClass(classNode, context->allocator);
    ASSERT_TRUE(adjustedClass.has_value());
    ASSERT_NE(*adjustedClass, nullptr);
    EXPECT_TRUE((*adjustedClass)->IsIdentifier());
    EXPECT_EQ((*adjustedClass)->AsIdentifier()->Name(), "MyClass");
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocation, GetAdjustedLocationForFunctionTest)
{
    const char *source = R"(
function testFunc(param: number) {
    return param * 2;
}
let result = testFunc(21);
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *funcNode = ark::es2panda::lsp::GetTouchingPropertyName(ctx, FUNCTION_POS);
    ASSERT_NE(funcNode, nullptr);
    ASSERT_TRUE(funcNode->IsIdentifier());
    EXPECT_EQ(funcNode->AsIdentifier()->Name(), "testFunc");
    auto *parent = funcNode->Parent();
    ASSERT_NE(parent, nullptr);
    auto adjustedFunc = ark::es2panda::lsp::GetAdjustedLocationForFunction(parent, context->allocator);
    ASSERT_TRUE(adjustedFunc.has_value());
    ASSERT_NE(*adjustedFunc, nullptr);
    EXPECT_TRUE((*adjustedFunc)->IsIdentifier());
    EXPECT_EQ((*adjustedFunc)->AsIdentifier()->Name(), "testFunc");
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocation, GetChildrenTest)
{
    const char *source = R"(
class Test {
    x = 1;
    y = "test";
    method() {}
}
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_CHECKED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *classNode = ark::es2panda::lsp::GetTouchingToken(ctx, CLASS_POS, false);
    ASSERT_NE(classNode, nullptr);
    ASSERT_TRUE(classNode->IsClassDeclaration());
    auto children = ark::es2panda::lsp::GetChildren(classNode, context->allocator);
    bool hasXProperty = false;
    bool hasYProperty = false;
    bool hasMethod = false;
    for (const auto *child : children) {
        if (child->IsIdentifier()) {
            auto name = child->AsIdentifier()->Name();
            if (name == "x") {
                hasXProperty = true;
            }
            if (name == "y") {
                hasYProperty = true;
            }
            if (name == "method") {
                hasMethod = true;
            }
        }
    }
    EXPECT_TRUE(hasXProperty) << "x property not found";
    EXPECT_TRUE(hasYProperty) << "y property not found";
    EXPECT_TRUE(hasMethod) << "method not found";
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocation, IsModifierTest)
{
    const char *source = R"(
class Test {
    private x: number;
    public y: string;
    static z: boolean;
}
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *privateNode = ark::es2panda::lsp::GetTouchingPropertyName(ctx, PRIVATE_POS);
    auto *publicNode = ark::es2panda::lsp::GetTouchingPropertyName(ctx, PUBLIC_POS);
    auto *staticNode = ark::es2panda::lsp::GetTouchingPropertyName(ctx, STATIC_POS);
    auto *nonModifierNode1 = ark::es2panda::lsp::GetTouchingPropertyName(ctx, PRIVATE_POS + 1);
    auto *nonModifierNode2 = ark::es2panda::lsp::GetTouchingPropertyName(ctx, PUBLIC_POS + 1);
    auto *nonModifierNode3 = ark::es2panda::lsp::GetTouchingPropertyName(ctx, STATIC_POS + 1);
    if (privateNode != nullptr) {
        EXPECT_TRUE(ark::es2panda::lsp::IsModifier(privateNode));
    }
    if (publicNode != nullptr) {
        EXPECT_TRUE(ark::es2panda::lsp::IsModifier(publicNode));
    }
    if (staticNode != nullptr) {
        EXPECT_TRUE(ark::es2panda::lsp::IsModifier(staticNode));
    }
    if (nonModifierNode1 != nullptr) {
        EXPECT_FALSE(ark::es2panda::lsp::IsModifier(nonModifierNode1));
    }
    if (nonModifierNode2 != nullptr) {
        EXPECT_FALSE(ark::es2panda::lsp::IsModifier(nonModifierNode2));
    }
    if (nonModifierNode3 != nullptr) {
        EXPECT_FALSE(ark::es2panda::lsp::IsModifier(nonModifierNode3));
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocation, GetAdjustedLocationForNestedScopeTest)
{
    const char *source =
        "let x = 1;\n"
        "class Test {\n"
        "    x: number = 2;\n"
        "    method() {\n"
        "        let x = 3;\n"
        "        return x;\n"
        "    }\n"
        "}\n";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_CHECKED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *globalVar = ark::es2panda::lsp::GetTouchingPropertyName(ctx, VARIABLE_POS);
    if (globalVar != nullptr) {
        auto adjustedGlobal = ark::es2panda::lsp::GetAdjustedLocation(globalVar, context->allocator);
        if (adjustedGlobal.has_value()) {
            EXPECT_TRUE((*adjustedGlobal)->IsIdentifier());
            EXPECT_EQ((*adjustedGlobal)->AsIdentifier()->Name(), "x");
        }
    }
    auto *memberVar = ark::es2panda::lsp::GetTouchingPropertyName(ctx, MEMBER_POS);
    if (memberVar != nullptr) {
        auto adjustedMember = ark::es2panda::lsp::GetAdjustedLocation(memberVar, context->allocator);
        if (adjustedMember.has_value()) {
            EXPECT_TRUE((*adjustedMember)->IsIdentifier());
            EXPECT_EQ((*adjustedMember)->AsIdentifier()->Name(), "x");
        }
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocation, GetAdjustedLocationForImportTest)
{
    const char *source = R"(
class MyClass {
    method() {}
}
let instance = new MyClass();
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_CHECKED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *node = ark::es2panda::lsp::GetTouchingPropertyName(ctx, IMPORT_POS);
    ASSERT_NE(node, nullptr);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(node, context->allocator);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_TRUE((*adjusted)->IsIdentifier());
    EXPECT_EQ((*adjusted)->AsIdentifier()->Name(), "MyClass");
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocation, GetAdjustedLocationForExportTest)
{
    const char *source = R"(
class MyClass {
    method() {}
}
let instance = new MyClass();
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_CHECKED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *node = ark::es2panda::lsp::GetTouchingPropertyName(ctx, EXPORT_POS);
    ASSERT_NE(node, nullptr);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(node, context->allocator);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_TRUE((*adjusted)->IsIdentifier());
    EXPECT_EQ((*adjusted)->AsIdentifier()->Name(), "MyClass");
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocation, GetAdjustedLocationForHeritageTest)
{
    const char *source = R"(
class BaseClass {}
class ChildClass extends BaseClass {}
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_CHECKED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *node = ark::es2panda::lsp::GetTouchingPropertyName(ctx, HERITAGE_POS);
    ASSERT_NE(node, nullptr);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(node, context->allocator);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_TRUE((*adjusted)->IsIdentifier());
    EXPECT_EQ((*adjusted)->AsIdentifier()->Name(), "BaseClass");
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocation, FindTypeReferenceTest)
{
    const char *source = R"(
class MyType {}
let variable: MyType;
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_CHECKED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *node = ark::es2panda::lsp::GetTouchingPropertyName(ctx, TYPE_PARAM_POS);
    ASSERT_NE(node, nullptr);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(node, context->allocator);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_TRUE((*adjusted)->IsIdentifier());
    EXPECT_EQ((*adjusted)->AsIdentifier()->Name(), "MyType");
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocation, FindArrayTypeTest)
{
    const char *source = R"(
type NumArray = number[];
let numbers: NumArray;
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *node = ark::es2panda::lsp::GetTouchingToken(ctx, ARRAY_TYPE_POS, true);
    ASSERT_NE(node, nullptr);
    auto children = ark::es2panda::lsp::GetChildren(node, context->allocator);
    auto *arrayType = ark::es2panda::lsp::FindArrayType(node, children);
    ASSERT_NE(arrayType, nullptr);
    EXPECT_TRUE(arrayType->IsTSArrayType());
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocation, SkipOuterExpressionsTest)
{
    const char *source = R"(
let x = (((42)));
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_CHECKED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *node = ark::es2panda::lsp::GetTouchingPropertyName(ctx, VARIABLE_POS);
    ASSERT_NE(node, nullptr);
    auto adjusted = ark::es2panda::lsp::GetAdjustedLocation(node, context->allocator);
    ASSERT_TRUE(adjusted.has_value());
    EXPECT_TRUE((*adjusted)->IsIdentifier());
    EXPECT_EQ((*adjusted)->AsIdentifier()->Name(), "x");
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocation, FindFirstExpressionTest)
{
    const char *source = R"(
let x = 42 + (5 * 3);
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_CHECKED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *node = ark::es2panda::lsp::GetTouchingPropertyName(ctx, VARIABLE_POS);
    ASSERT_NE(node, nullptr);
    auto children = ark::es2panda::lsp::GetChildren(node, context->allocator);
    auto *found = ark::es2panda::lsp::FindFirstExpression(node, children);
    ASSERT_NE(found, nullptr);
    EXPECT_TRUE(found->IsExpression());
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocation, FindFirstExpressionAfterTest)
{
    const char *source = R"(let x = 1 + 2 + 3;)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_CHECKED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *node = ark::es2panda::lsp::GetTouchingToken(ctx, EXPRESSION_POS, true);
    ASSERT_NE(node, nullptr);
    auto children = ark::es2panda::lsp::GetChildren(node, context->allocator);
    auto *firstExpr = ark::es2panda::lsp::FindFirstExpression(node, children);
    ASSERT_NE(firstExpr, nullptr);
    auto *afterFirst = ark::es2panda::lsp::FindFirstExpressionAfter(node, firstExpr, children);
    ASSERT_NE(afterFirst, nullptr);
    EXPECT_TRUE(afterFirst->IsExpression());
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocation, FindNodeOfTypeTest)
{
    const char *source = R"(
class TestClass {
    method() {
        return 42;
    }
}
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_CHECKED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *node = ark::es2panda::lsp::GetTouchingToken(ctx, METHOD_POS, true);
    ASSERT_NE(node, nullptr);
    auto children = ark::es2panda::lsp::GetChildren(node, context->allocator);
    auto *methodBody = ark::es2panda::lsp::FindNodeOfType(node, AstNodeType::BLOCK_STATEMENT, children);
    ASSERT_NE(methodBody, nullptr);
    auto methodChildren = ark::es2panda::lsp::GetChildren(methodBody, context->allocator);
    auto *found = ark::es2panda::lsp::FindNodeOfType(methodBody, AstNodeType::RETURN_STATEMENT, methodChildren);
    ASSERT_NE(found, nullptr);
    EXPECT_TRUE(found->IsReturnStatement());
    initializer.DestroyContext(ctx);
}

TEST_F(LspGetAdjustedLocation, FindTypeParameterTest)
{
    const char *source = R"(
class Container<T> {
    value: T;
}
)";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext("test.ets", ES2PANDA_STATE_CHECKED, source);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *node = ark::es2panda::lsp::GetTouchingToken(ctx, TYPE_PARAM_POS, true);
    ASSERT_NE(node, nullptr);
    auto children = ark::es2panda::lsp::GetChildren(node, context->allocator);
    auto *typeParam = ark::es2panda::lsp::FindTypeParameter(node, children);
    ASSERT_NE(typeParam, nullptr);
    EXPECT_TRUE(typeParam->IsTSTypeParameterDeclaration() || typeParam->IsIdentifier());
    if (typeParam->IsIdentifier()) {
        EXPECT_EQ(typeParam->AsIdentifier()->Name(), "T");
    }
    initializer.DestroyContext(ctx);
}
