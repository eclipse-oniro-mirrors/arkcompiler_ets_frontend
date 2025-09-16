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

#include "lowering_test.h"
#include "compiler/lowering/ets/topLevelStmts/topLevelStmts.h"

namespace ark::es2panda {

TEST_F(LoweringTest, TestTopLevelStmtsSyntheticModuleGlobalClass)
{
    char const *text = R"(
        function foo() { } // foo is a member of *module* class
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        const auto *const ast = GetAst();
        [[maybe_unused]] auto *classDef = ast->FindChild([](ir::AstNode *child) {
            return child->IsClassDefinition() &&
                   (child->AsClassDefinition()->InternalName().Mutf8() == "dummy.ETSGLOBAL");
        });
        ASSERT(classDef != nullptr);
        ASSERT(classDef->AsClassDefinition()->IsGlobalInitialized());
    }
}

TEST_F(LoweringTest, TestTopLevelStmtsSyntheticModuleClass)
{
    char const *text = R"(
        namespace X {
            export function bar() { } // bar is a member of another *module* class
        }
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        const auto *const ast = GetAst();
        [[maybe_unused]] auto *classDef = ast->FindChild([](ir::AstNode *child) {
            return child->IsClassDefinition() && (child->AsClassDefinition()->InternalName().Mutf8() == "dummy.X");
        });
        ASSERT(classDef != nullptr);
        ASSERT(classDef->AsClassDefinition()->IsNamespaceTransformed());
    }
}

TEST_F(LoweringTest, TestTopLevelStmtsExportedClass)
{
    char const *text = R"(
        export class A {}
        class B {}
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        const auto *const ast = GetAst();
        auto *classDef = ast->FindChild([](ir::AstNode *child) {
            return child->IsClassDefinition() && child->AsClassDefinition()->IsGlobalInitialized();
        });
        ASSERT_NE(classDef, nullptr);

        const auto &exportedClasses = classDef->AsClassDefinition()->ExportedClasses();
        ASSERT_EQ(exportedClasses.size(), 1);
        ASSERT_TRUE(exportedClasses[0]->IsExported());
        ASSERT_EQ(exportedClasses[0]->Definition()->InternalName().Mutf8(), "dummy.A");
    }
}

TEST_F(LoweringTest, TestTopLevelStmtsExportedNamespace)
{
    char const *text = R"(
        export namespace A {}
        namespace B {}
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        const auto *const ast = GetAst();
        auto *classDef = ast->FindChild([](ir::AstNode *child) {
            return child->IsClassDefinition() && child->AsClassDefinition()->IsGlobalInitialized();
        });
        ASSERT_NE(classDef, nullptr);

        const auto &exportedClasses = classDef->AsClassDefinition()->ExportedClasses();
        ASSERT_EQ(exportedClasses.size(), 1);
        ASSERT_TRUE(exportedClasses[0]->IsExported());
        ASSERT_EQ(exportedClasses[0]->Definition()->InternalName().Mutf8(), "dummy.A");
    }
}

TEST_F(LoweringTest, TestTopLevelStmtsExportedNamespaceNested)
{
    char const *text = R"(
        export namespace A {
            export namespace B {}
            namespace C {}
            export class D {}
            class E {}
        }
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        const auto *const ast = GetAst();
        auto *classDef = ast->FindChild([](ir::AstNode *child) {
            return child->IsClassDefinition() && child->AsClassDefinition()->IsNamespaceTransformed();
        });
        ASSERT(classDef != nullptr);

        const auto &exportedClasses = classDef->AsClassDefinition()->ExportedClasses();
        constexpr uint32_t EXPORTED_CLASSES_NUM = 2;
        ASSERT_EQ(exportedClasses.size(), EXPORTED_CLASSES_NUM);
        ASSERT_TRUE(exportedClasses[0]->IsExported());
        ASSERT_TRUE(exportedClasses[1]->IsExported());
        ASSERT_EQ(exportedClasses[0]->Definition()->InternalName().Mutf8(), "dummy.A.B");
        ASSERT_EQ(exportedClasses[1]->Definition()->InternalName().Mutf8(), "dummy.A.D");
    }
}

TEST_F(LoweringTest, TestTopLevelStmtsExportedEnum)
{
    char const *text = R"(
        export let a = 10;
        export enum Color {
            Red = 1
        }
    )";

    CONTEXT(ES2PANDA_STATE_LOWERED, text)
    {
        const auto *const ast = GetAst();
        auto *classDef = ast->FindChild([](ir::AstNode *child) {
            return child->IsClassDefinition() && child->AsClassDefinition()->IsGlobalInitialized();
        });
        ASSERT(classDef != nullptr);

        const auto &exportedClasses = classDef->AsClassDefinition()->ExportedClasses();
        constexpr uint32_t EXPORTED_CLASSES_NUM = 2;
        ASSERT_EQ(exportedClasses.size(), EXPORTED_CLASSES_NUM);
        ASSERT_TRUE(exportedClasses[1]->IsExported());
        ASSERT_EQ(exportedClasses[0]->Definition()->InternalName().Mutf8(), "dummy.ETSGLOBAL");
        ASSERT_EQ(exportedClasses[1]->Definition()->InternalName().Mutf8(), "dummy.Color");
    }
}

TEST_F(LoweringTest, TestTrailingLambdaSourceRange)
{
    char const *text = R"(
        function foo(ff: ()=>void) {}
        function testfoo() {}
        foo() {
            testfoo()
        }
    )";

    CONTEXT(ES2PANDA_STATE_ASM_GENERATED, text)
    {
        const auto *const ast = GetAst();
        constexpr uint32_t EXPECTED_STARTLINE = 4;
        constexpr uint32_t EXPECTED_ENDLINE = 4;
        auto *callExpr = ast->FindChild([](ir::AstNode *child) {
            return child->IsCallExpression() && child->AsCallExpression()->Callee()->IsIdentifier() &&
                   child->AsCallExpression()->Callee()->AsIdentifier()->Name().Is("testfoo");
        });
        ASSERT(callExpr != nullptr);
        ASSERT_EQ(callExpr->Start().line, EXPECTED_STARTLINE);
        ASSERT_EQ(callExpr->End().line, EXPECTED_ENDLINE);
    }
}

}  // namespace ark::es2panda
