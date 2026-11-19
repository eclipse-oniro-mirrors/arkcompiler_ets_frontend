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
/*
 * Null-pointer safety and memory-leak regression tests.
 *
 * Related issue: https://gitcode.com/openharmony/arkcompiler_ets_frontend/issues/12320
 *
 * Coverage areas:
 *   1. Checker null-safety: unresolved identifiers must be reported as ordinary
 *      semantic errors instead of dereferencing null pointers in
 *      ETSChecker::ValidateResolvedIdentifier / ValidateMemberIdentifier /
 *      ValidateAssignmentIdentifier (checker/ets/validateHelpers.cpp).
 *   2. Dumper null-safety: dumping method definitions, including ambient
 *      (declare) declarations and annotated methods, must not crash in
 *      MethodDefinition::Dump / DumpSingleOverload (ir/base/methodDefinition.cpp).
 *   3. Memory-leak checks: repeated public API reads (names, class bodies,
 *      dumps, context metadata) must not grow the EHeap arena
 *      (util/eheap.h), following the patterns of api_result_storage_test.cpp.
 *
 * Sources with semantic errors are checked with the two-state CONTEXT form
 * (proceed to CHECKED, expect ES2PANDA_STATE_ERROR), so checker error
 * recovery is exercised end to end without crashing.
 */
#include <string>

#include "ir/astNode.h"
#include "ir/base/classDefinition.h"
#include "ir/expressions/identifier.h"
#include "public/public.h"
#include "test/utils/ast_verifier_test.h"
#include "util/eheap.h"

namespace {

using NullSafetyMemoryTest = test::utils::AstVerifierTest;

// ---------------------------------------------------------------------------
// Section 1: parse robustness
// Parsing edge-case sources must complete without null dereferences.
// ---------------------------------------------------------------------------

// Parses empty source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_00_empty)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX()XXX", "parse_00.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_EQ(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses whitespace source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_01_whitespace)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(

)XXX",
            "parse_01.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_EQ(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses line_comment source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_02_line_comment)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    // only a comment

    )XXX",
            "parse_02.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_EQ(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses block_comment source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_03_block_comment)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    /* block
    comment */

    )XXX",
            "parse_03.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_EQ(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses simple_class source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_04_simple_class)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P0 {})XXX", "parse_04.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses two_classes source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_05_two_classes)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P1 {}
    class P2 {})XXX",
            "parse_05.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses class_fields source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_06_class_fields)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P3 { a: int = 1; b: string = 'x'; c: boolean = true; })XXX",
            "parse_06.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses class_methods source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_07_class_methods)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P4 { m(): void {} n(): int { return 1; } })XXX", "parse_07.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses class_getset source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_08_class_getset)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P5 { p: int = 0
     get v(): int { return this.p }
     set v(n: int) { this.p = n } })XXX",
            "parse_08.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses class_ctor source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_09_class_ctor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P6 { v: int
     constructor(n: int) { this.v = n } })XXX",
            "parse_09.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses class_static source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_10_class_static)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P7 { static f: int = 1
     static m(): void {} })XXX",
            "parse_10.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses class_extends source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_11_class_extends)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P8 { b: int = 1 }
    class P9 extends P8 { d: int = 2 })XXX",
            "parse_11.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses class_generic source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_12_class_generic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class PA<T> { v: T })XXX", "parse_12.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses class_readonly source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_13_class_readonly)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class PB { readonly k: int = 9 })XXX", "parse_13.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses class_optional_field source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_14_class_optional_field)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class PC { f?: int })XXX", "parse_14.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses abstract_class source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_15_abstract_class)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    abstract class PD { m(): void {} })XXX", "parse_15.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses interface_basic source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_16_interface_basic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface PE { m(): void })XXX", "parse_16.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses interface_props source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_17_interface_props)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface PF { p: int
     m(): void })XXX",
            "parse_17.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses enum_basic source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_18_enum_basic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    enum PG { A, B })XXX", "parse_18.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses enum_values source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_19_enum_values)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    enum PH { X = 1, Y = 2 })XXX", "parse_19.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses declare_class source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_20_declare_class)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class PI { m(): void })XXX", "parse_20.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses declare_class_members source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_21_declare_class_members)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class PJ { m(a: int): string
     f: boolean })XXX",
            "parse_21.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses declare_function source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_22_declare_function)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare function PK(a: int): string)XXX", "parse_22.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses declare_interface source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_23_declare_interface)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare interface PL { m(): void })XXX", "parse_23.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses function_plain source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_24_function_plain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function PM(): void {})XXX", "parse_24.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses function_params source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_25_function_params)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function PN(a: int, b: string): void {})XXX", "parse_25.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses function_optional source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_26_function_optional)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function PO(a?: int): void {})XXX", "parse_26.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses function_default source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_27_function_default)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function PQ(a: int = 1): void {})XXX", "parse_27.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses function_rest source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_28_function_rest)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function PR(...rest: int[]): void {})XXX", "parse_28.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses function_arrow source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_29_function_arrow)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function PS(): void { let f = (x: int): int => x; })XXX", "parse_29.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses function_lambda_body source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_30_function_lambda_body)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function PT(): void { let f = (x: int): int => { return x; }; })XXX",
            "parse_30.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses annotations source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_31_annotations)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface Mark2 { }
    class PU { @Mark2 m(): void {} })XXX",
            "parse_31.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses annotation_props source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_32_annotation_props)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface Mark3 { v: string }
    class PV { @Mark3('p') m(): void {} })XXX",
            "parse_32.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses locals source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_33_locals)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            R"XXX(    function PW(): void { let a: int = 1; const b: int = 2; let s: string = 'x'; })XXX",
            "parse_33.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses arrays source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_34_arrays)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            R"XXX(    function PX(): void { let a: int[] = [1, 2, 3]; let t: string[] = ['a', 'b']; })XXX",
            "parse_34.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses template_literal source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_35_template_literal)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function PY(): void { let s: string = `v${1}`; })XXX", "parse_35.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses control_flow source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_36_control_flow)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function PZ(): void { for (let i = 0; i < 10; i++) {}
      for (let v of [1, 2]) {}
      while (false) {}
      do {} while (false); })XXX",
            "parse_36.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses try_catch source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_37_try_catch)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function QA(): void { try { } catch (e) { } finally { } })XXX",
            "parse_37.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses switch_stmt source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_38_switch_stmt)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function QB(): void { switch (1) { case 1: break; default: break; } })XXX",
            "parse_38.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses if_else source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_39_if_else)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function QC(): void { if (true) { } else { } })XXX", "parse_39.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses ternary_expr source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_40_ternary_expr)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function QD(): void { let x: int = true ? 1 : 2; })XXX", "parse_40.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses mixed_decls source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_41_mixed_decls)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class QE { v: int = 1 }
    enum QF { A }
    function QG(): void { let e: QF = QF.A; })XXX",
            "parse_41.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses union_type source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_42_union_type)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function QH(): void { let u: int | string = 1; })XXX", "parse_42.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses multi_line_class source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_43_multi_line_class)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class QI {
        a: int = 1;
        b: string = 'x';

        m(): int {
            return this.a;
        }
    })XXX",
            "parse_43.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_decl_class_basic source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_44_dump_decl_class_basic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC0 { m(): void })XXX", "parse_44.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_decl_class_params source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_45_dump_decl_class_params)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC1 { m(a: int): string
     n(b: boolean, c: string): int })XXX",
            "parse_45.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_decl_class_optional source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_46_dump_decl_class_optional)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC2 { m(a?: int): void
     n(b: string?): string })XXX",
            "parse_46.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_decl_class_static source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_47_dump_decl_class_static)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC3 { static m(): void
     static f: int })XXX",
            "parse_47.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_decl_class_public source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_48_dump_decl_class_public)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC4 { public m(): void })XXX", "parse_48.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_decl_class_multi source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_49_dump_decl_class_multi)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC5 { a(): void
     b(): int
     c(): string })XXX",
            "parse_49.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_decl_class_props source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_50_dump_decl_class_props)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC6 { p: int
     q: string })XXX",
            "parse_50.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_decl_class_overloads source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_51_dump_decl_class_overloads)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC7 { m(a: int): void
     m(a: string): void })XXX",
            "parse_51.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_decl_func source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_52_dump_decl_func)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare function df(a: int): string)XXX", "parse_52.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_decl_interface source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_53_dump_decl_interface)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare interface DI0 { m(): void })XXX", "parse_53.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_interface_basic source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_54_dump_interface_basic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IB0 { m(): void })XXX", "parse_54.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_interface_props source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_55_dump_interface_props)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IP0 { p: int
     m(): void })XXX",
            "parse_55.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_interface_getset source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_56_dump_interface_getset)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IG0 { get v(): int
     set v(n: int) })XXX",
            "parse_56.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_class_methods source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_57_dump_class_methods)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CM0 { m(): void {} n(): int { return 1; } })XXX", "parse_57.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_class_getset source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_58_dump_class_getset)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CG0 { p: int = 0
     get v(): int { return this.p }
     set v(n: int) { this.p = n } })XXX",
            "parse_58.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_class_static source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_59_dump_class_static)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CS0 { static f: int = 1
     static m(): void {} })XXX",
            "parse_59.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_class_annot source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_60_dump_class_annot)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface Mark0 { }
    class CA0 { @Mark0 m(): void {} })XXX",
            "parse_60.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_class_annot_params source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_61_dump_class_annot_params)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface Mark1 { v: string }
    class CA1 { @Mark1('x') m(a: int): string { return '' } })XXX",
            "parse_61.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_class_ctor source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_62_dump_class_ctor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CC0 { v: int
     constructor(n: int) { this.v = n } })XXX",
            "parse_62.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_class_generic source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_63_dump_class_generic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CG1<T> { v: T })XXX", "parse_63.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_class_extends source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_64_dump_class_extends)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class B0 { b: int = 1 }
    class D0 extends B0 { d: int = 2 })XXX",
            "parse_64.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_class_readonly source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_65_dump_class_readonly)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CR0 { readonly k: int = 9 })XXX", "parse_65.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_class_private source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_66_dump_class_private)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CP0 { private p: int = 1
     public q: int = 2 })XXX",
            "parse_66.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_class_nested source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_67_dump_class_nested)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CN0 { inner: CN1 = new CN1()
     class CN1 { v: int = 1 } })XXX",
            "parse_67.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_enum_basic source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_68_dump_enum_basic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    enum E0 { A, B, C })XXX", "parse_68.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_enum_values source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_69_dump_enum_values)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    enum E1 { X = 1, Y = 2, Z = 3 })XXX", "parse_69.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_function_body source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_70_dump_function_body)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function fb(a: int): int { return a + 1; })XXX", "parse_70.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump_function_lambda source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_71_dump_function_lambda)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function fl(): void { let f = (x: int): int => x; })XXX", "parse_71.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump2_class_implements source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_72_dump2_class_implements)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class VW0 { m(): void {} }
    class VX0 implements VW0 { m(): void {} })XXX",
            "parse_72.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump2_abstract_method source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_73_dump2_abstract_method)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    abstract class VY0 { m(): void {} })XXX", "parse_73.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump2_decl_class_extends source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_74_dump2_decl_class_extends)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class VZ0 extends Unknown0 { m(): void })XXX", "parse_74.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump2_multi_annotations source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_75_dump2_multi_annotations)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface A0 { }
    @interface B0 { }
    class WA0 { @A0 @B0 m(): void {} })XXX",
            "parse_75.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump2_annot_getter source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_76_dump2_annot_getter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface A1 { }
    class WB0 { @A1 get v(): int { return 1 } })XXX",
            "parse_76.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump2_annot_setter source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_77_dump2_annot_setter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface A2 { }
    class WC0 { p: int = 0
     @A2 set v(n: int) { this.p = n } })XXX",
            "parse_77.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump2_decl_class_annot source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_78_dump2_decl_class_annot)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface A3 { }
    declare class WD0 { @A3 m(): void })XXX",
            "parse_78.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump2_interface_extends source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_79_dump2_interface_extends)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IE0 { m(): void }
    interface IF0 extends IE0 { m(): void })XXX",
            "parse_79.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump2_function_annot source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_80_dump2_function_annot)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface A5 { }
    @A5
    function FA0(): void {})XXX",
            "parse_80.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump2_decl_class_many source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_81_dump2_decl_class_many)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class WH0 { a(): void
     b(): void
     c(): void
     d(): void })XXX",
            "parse_81.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump2_jsdoc_class source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_82_dump2_jsdoc_class)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    /** doc */
    class WJ0 { m(): void {} })XXX",
            "parse_82.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses dump2_mixed_members source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_83_dump2_mixed_members)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class WK1 { a: int = 1
     static b: int = 2
     m(): void {}
     get c(): int { return 1 }
     n(x: string): string { return x } })XXX",
            "parse_83.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses parse2_arrow_no_params source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_84_parse2_arrow_no_params)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function R0(): void { let f = (): int => 1; })XXX", "parse_84.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses parse2_let_reassign source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_85_parse2_let_reassign)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function R1(): void { let s: string = 'a'; s = 'b'; })XXX", "parse_85.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses parse2_empty_function source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_86_parse2_empty_function)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function R2(): void {})XXX", "parse_86.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses parse2_nested_if source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_87_parse2_nested_if)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function R3(): void { if (true) { if (false) { } } })XXX", "parse_87.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses parse2_empty_class_multi source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_88_parse2_empty_class_multi)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class R4 {}
    class R5 {}
    class R6 {})XXX",
            "parse_88.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses parse2_method_default_params source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_89_parse2_method_default_params)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class R7 { m(a: int = 1, b?: string): void {} })XXX", "parse_89.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses parse2_field_initializer_expr source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_90_parse2_field_initializer_expr)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class RA0 { a: int = 1 + 2 * 3; })XXX", "parse_90.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses parse2_boolean_ops source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_91_parse2_boolean_ops)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function RC0(): void { let a: boolean = true; let b: boolean = !a; })XXX",
            "parse_91.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses parse2_number_formats source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_92_parse2_number_formats)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            R"XXX(    function RD0(): void { let h: int = 0xFF; let o: int = 0o17; let b: int = 0b101; })XXX",
            "parse_92.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses parse2_array_of_arrays source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_93_parse2_array_of_arrays)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function RE0(): void { let m: int[][] = [[1], [2, 3]]; })XXX",
            "parse_93.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses parse2_conditional_nested source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_94_parse2_conditional_nested)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function RF0(): void { let x: int = true ? (false ? 1 : 2) : 3; })XXX",
            "parse_94.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses parse2_class_static_block source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_95_parse2_class_static_block)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class RG0 { static { } })XXX", "parse_95.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses parse2_string_simple source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_96_parse2_string_simple)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function RH0(): void { let s: string = 'a'; let t: string = s; })XXX",
            "parse_96.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses parse2_enum_nested_use source and verifies the resulting AST is complete, walkable and
// dumpable without null dereferences.
TEST_F(NullSafetyMemoryTest, ParseRobust_97_parse2_enum_nested_use)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    enum RI0 { A }
    function RJ0(): void { let e: RI0 = RI0.A; })XXX",
            "parse_97.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t nodeCount = 0U;
        size_t methodCount = 0U;
        size_t classCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            ++nodeCount;
            methodCount += node->IsMethodDefinition() ? 1U : 0U;
            classCount += node->IsClassDefinition() ? 1U : 0U;
        });
        EXPECT_GT(nodeCount, 0U);
        auto *root = reinterpret_cast<es2panda_AstNode *>(ast);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Parses empty source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_00)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX()XXX", "parseheavy_00.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses whitespace source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_01)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(

)XXX",
            "parseheavy_01.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses line_comment source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_02)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    // only a comment

    )XXX",
            "parseheavy_02.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses block_comment source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_03)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    /* block
    comment */

    )XXX",
            "parseheavy_03.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses simple_class source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_04)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P0 {})XXX", "parseheavy_04.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses two_classes source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_05)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P1 {}
    class P2 {})XXX",
            "parseheavy_05.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses class_fields source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_06)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P3 { a: int = 1; b: string = 'x'; c: boolean = true; })XXX",
            "parseheavy_06.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses class_methods source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_07)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P4 { m(): void {} n(): int { return 1; } })XXX", "parseheavy_07.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses class_getset source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_08)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P5 { p: int = 0
     get v(): int { return this.p }
     set v(n: int) { this.p = n } })XXX",
            "parseheavy_08.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses class_ctor source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_09)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P6 { v: int
     constructor(n: int) { this.v = n } })XXX",
            "parseheavy_09.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses class_static source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_10)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P7 { static f: int = 1
     static m(): void {} })XXX",
            "parseheavy_10.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses class_extends source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_11)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P8 { b: int = 1 }
    class P9 extends P8 { d: int = 2 })XXX",
            "parseheavy_11.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses class_generic source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_12)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class PA<T> { v: T })XXX", "parseheavy_12.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses class_readonly source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_13)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class PB { readonly k: int = 9 })XXX", "parseheavy_13.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses class_optional_field source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_14)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class PC { f?: int })XXX", "parseheavy_14.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses abstract_class source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_15)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    abstract class PD { m(): void {} })XXX", "parseheavy_15.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses interface_basic source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_16)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface PE { m(): void })XXX", "parseheavy_16.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses interface_props source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_17)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface PF { p: int
     m(): void })XXX",
            "parseheavy_17.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses enum_basic source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_18)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    enum PG { A, B })XXX", "parseheavy_18.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses enum_values source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_19)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    enum PH { X = 1, Y = 2 })XXX", "parseheavy_19.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses arrow_no_params source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_20)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function R0(): void { let f = (): int => 1; })XXX", "parseheavy_20.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses let_reassign source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_21)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function R1(): void { let s: string = 'a'; s = 'b'; })XXX",
            "parseheavy_21.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses empty_function source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_22)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function R2(): void {})XXX", "parseheavy_22.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses nested_if source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_23)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function R3(): void { if (true) { if (false) { } } })XXX",
            "parseheavy_23.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses empty_class_multi source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_24)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class R4 {}
    class R5 {}
    class R6 {})XXX",
            "parseheavy_24.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses method_default_params source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_25)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class R7 { m(a: int = 1, b?: string): void {} })XXX", "parseheavy_25.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses field_initializer_expr source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_26)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class RA0 { a: int = 1 + 2 * 3; })XXX", "parseheavy_26.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses boolean_ops source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_27)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function RC0(): void { let a: boolean = true; let b: boolean = !a; })XXX",
            "parseheavy_27.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses number_formats source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_28)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            R"XXX(    function RD0(): void { let h: int = 0xFF; let o: int = 0o17; let b: int = 0b101; })XXX",
            "parseheavy_28.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses array_of_arrays source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_29)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function RE0(): void { let m: int[][] = [[1], [2, 3]]; })XXX",
            "parseheavy_29.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses conditional_nested source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_30)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function RF0(): void { let x: int = true ? (false ? 1 : 2) : 3; })XXX",
            "parseheavy_30.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses class_static_block source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_31)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class RG0 { static { } })XXX", "parseheavy_31.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses string_simple source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_32)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function RH0(): void { let s: string = 'a'; let t: string = s; })XXX",
            "parseheavy_32.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Parses enum_nested_use source, walks the tree and re-reads context metadata repeatedly;
// none of these operations may crash or grow the EHeap arena.
TEST_F(NullSafetyMemoryTest, ParseRobustHeavy_33)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    enum RI0 { A }
    function RJ0(): void { let e: RI0 = RI0.A; })XXX",
            "parseheavy_33.ats")
    {
        auto *ast = GetAst();
        ASSERT_NE(ast, nullptr);
        size_t identifierCount = 0U;
        size_t statementCount = 0U;
        ast->IterateRecursively([&](ir_alias::AstNode *node) {
            identifierCount += node->IsIdentifier() ? 1U : 0U;
            statementCount += node->IsStatement() ? 1U : 0U;
        });
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 500U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_PARSED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}

// ---------------------------------------------------------------------------
// Section 2: checker validation on resolved identifiers
// Valid sources exercise ValidateMemberIdentifier / ValidateAssignmentIdentifier and related helpers with non-null
// variables.
// ---------------------------------------------------------------------------

// Fully checks flat_fields source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_00_flat_fields)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VF0 { a: int = 1; b: string = 'x'; c: boolean = true; })XXX",
            "valid_00.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks with_method source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_01_with_method)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VF1 { m(): void {} })XXX", "valid_01.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks field_read source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_02_field_read)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VF2 { value: int = 1; }
    function main(): void { let c = new VF2(); let v: int = c.value; })XXX",
            "valid_02.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks method_call source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_03_method_call)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VF3 { m(): int { return 1; } }
    function main(): void { let c = new VF3(); let v: int = c.m(); })XXX",
            "valid_03.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks local_assignment source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_04_local_assignment)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    function main(): void { let x: int = 1; x = 2; x = x + 1; })XXX",
            "valid_04.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks binary_arith source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_05_binary_arith)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let a: int = 1; let b: int = a + 2; let c: boolean = a > b; })XXX",
            "valid_05.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks logical_ops source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_06_logical_ops)
{
    CONTEXT(
        ES2PANDA_STATE_CHECKED,
        R"XXX(    function main(): void { let a: boolean = true; let b: boolean = !a; let c: boolean = a && b; })XXX",
        "valid_06.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks update_expr source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_07_update_expr)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let i: int = 0; i++; ++i; i--; let j: int = i; })XXX", "valid_07.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks unary_ops source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_08_unary_ops)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let a: int = 1; let b: int = -a; let c: int = +b; })XXX", "valid_08.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks enum_access source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_09_enum_access)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    enum VG0 { RED, GREEN, BLUE }
    function main(): void { let c: VG0 = VG0.RED; })XXX",
            "valid_09.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks enum_values source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_10_enum_values)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    enum VG1 { A = 1, B = 2, C = 3 }
    function main(): void { let o: VG1 = VG1.B; })XXX",
            "valid_10.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks getter_usage source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_11_getter_usage)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG2 { p: int = 0
     get v(): int { return this.p } }
    function main(): void { let g = new VG2(); let n: int = g.v; })XXX",
            "valid_11.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks setter_usage source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_12_setter_usage)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG3 { p: int = 0
     set v(n: int) { this.p = n } }
    function main(): void { let g = new VG3(); g.v = 5; })XXX",
            "valid_12.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks static_method source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_13_static_method)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG4 { static m(): int { return 42; } }
    function main(): void { let v: int = VG4.m(); })XXX",
            "valid_13.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks static_field source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_14_static_field)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG5 { static f: int = 7; }
    function main(): void { let v: int = VG5.f; })XXX",
            "valid_14.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks inheritance source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_15_inheritance)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG6 { b: int = 1; }
    class VG7 extends VG6 { d: int = 2; }
    function main(): void { let x = new VG7(); let v: int = x.b; })XXX",
            "valid_15.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks ctor_params source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_16_ctor_params)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG8 { v: int
     constructor(n: int) { this.v = n } }
    function main(): void { let p = new VG8(3); })XXX",
            "valid_16.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks optional_param source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_17_optional_param)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    function opt(a: int, b?: int): int { return a; }
    function main(): void { let v: int = opt(1); })XXX",
            "valid_17.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks lambda_simple source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_18_lambda_simple)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let f = (x: int): int => { return x + 1; }; let v: int = f(1); })XXX",
            "valid_18.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks two_classes source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_19_two_classes)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VH0 { x: int = 1; }
    class VH1 { y: int = 2; }
    function main(): void { let a = new VH0(); let b = new VH1(); })XXX",
            "valid_19.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks readonly_field source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_20_readonly_field)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VH2 { readonly k: int = 9; }
    function main(): void { let r = new VH2(); let v: int = r.k; })XXX",
            "valid_20.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks const_local source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_21_const_local)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    function main(): void { const v: int = 5; let w: int = v; })XXX",
            "valid_21.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks ternary source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_22_ternary)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let a: int = 1; let b: int = a > 0 ? 1 : 2; })XXX", "valid_22.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks member_chain source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_23_member_chain)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VH3 { inner: VH4 = new VH4() }
    class VH4 { v: int = 3 }
    function main(): void { let o = new VH3(); let x: int = o.inner.v; })XXX",
            "valid_23.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks assign_member source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_24_assign_member)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VH5 { v: int = 0 }
    function main(): void { let o = new VH5(); o.v = 1; })XXX",
            "valid_24.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks nullish_optional source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_25_nullish_optional)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    function nn(a?: int): int { return a ?? 0; }
    function main(): void { let v: int = nn(); })XXX",
            "valid_25.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks int_div_mod source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_26_int_div_mod)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let a: int = 7; let b: int = a / 2; let c: int = a % 3; })XXX",
            "valid_26.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks string_concat source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_27_string_concat)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let a: string = 'x'; let b: string = a + 'y'; })XXX", "valid_27.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks double_type source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_28_double_type)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let d: double = 1.5; let e: double = d + 0.5; })XXX", "valid_28.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks long_type source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_29_long_type)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    function main(): void { let l: long = 100; })XXX", "valid_29.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks boolean_field source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_30_boolean_field)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VJ0 { flag: boolean = false }
    function main(): void { let v = new VJ0(); let f: boolean = v.flag; })XXX",
            "valid_30.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks nested_call source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_31_nested_call)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VJ1 { m(): int { return 1; } }
    function main(): void { let c = new VJ1(); let v: int = c.m() + c.m(); })XXX",
            "valid_31.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks array_type source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_32_array_type)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    function main(): void { let a: int[] = [1, 2]; let n: int = a[0]; })XXX",
            "valid_32.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks array_field source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_33_array_field)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VJ2 { arr: int[] = [1, 2] }
    function main(): void { let v = new VJ2(); let x: int = v.arr[0]; })XXX",
            "valid_33.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks void_method source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_34_void_method)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VJ3 { m(): void { } }
    function main(): void { let c = new VJ3(); c.m(); })XXX",
            "valid_34.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}
// Fully checks if_chain source. All identifiers resolve, so the checker validation
// helpers run their non-null paths and the context must reach CHECKED.
TEST_F(NullSafetyMemoryTest, CheckValid_35_if_chain)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let a: int = 1; if (a > 0) { } else if (a > 1) { } else { } })XXX",
            "valid_35.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(dump, nullptr);
        EXPECT_FALSE(std::string(dump).empty());
    }
}

// ---------------------------------------------------------------------------
// Section 3: checker null-safety on unresolved identifiers
// Unresolved identifiers must end in ES2PANDA_STATE_ERROR, not a crash (issue #12320).
// ---------------------------------------------------------------------------

// Checks member_read source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_00_member_read)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { um0.field; })XXX",
            "unres_00.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_chain source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_01_member_chain)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { um1.a.b; })XXX",
            "unres_01.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_deep source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_02_member_deep)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { um2.a.b.c; })XXX",
            "unres_02.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_write source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_03_member_write)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { um3.field = 1; })XXX",
            "unres_03.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_write_chain source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_04_member_write_chain)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { um4.a.b = 2; })XXX",
            "unres_04.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks computed_int source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_05_computed_int)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { um5[0]; })XXX",
            "unres_05.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks computed_string source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_06_computed_string)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { um6['key']; })XXX",
            "unres_06.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks computed_write source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_07_computed_write)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { um7[0] = 1; })XXX",
            "unres_07.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks assign_lhs source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_08_assign_lhs)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { ul0 = 1; })XXX",
            "unres_08.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks assign_rhs source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_09_assign_rhs)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { ul1 = ur1; })XXX",
            "unres_09.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks assign_both_unresolved source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_10_assign_both_unresolved)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { ul2 = ur2; })XXX",
            "unres_10.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks arith_left source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_11_arith_left)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let x: int = un0 + 1; })XXX", "unres_11.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks arith_right source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_12_arith_right)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let x: int = 1 - un1; })XXX", "unres_12.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks arith_both source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_13_arith_both)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let x: int = un2 * un3; })XXX", "unres_13.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks compare source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_14_compare)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let b: boolean = un4 > 0; })XXX", "unres_14.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks logical_and source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_15_logical_and)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let b: boolean = un5 && true; })XXX", "unres_15.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks logical_or source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_16_logical_or)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let b: boolean = false || un6; })XXX", "unres_16.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks nullish source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_17_nullish)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let x: int = un7 ?? 0; })XXX", "unres_17.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks unary_neg source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_18_unary_neg)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { let x: int = -un8; })XXX",
            "unres_18.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks unary_not source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_19_unary_not)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let b: boolean = !un9; })XXX", "unres_19.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks unary_pos source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_20_unary_pos)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { let x: int = +un10; })XXX",
            "unres_20.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks update_postfix source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_21_update_postfix)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { uc0++; })XXX",
            "unres_21.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks update_prefix source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_22_update_prefix)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { --uc1; })XXX",
            "unres_22.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks update_write source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_23_update_write)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { uc2--; })XXX",
            "unres_23.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks plain_ident source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_24_plain_ident)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { us0; })XXX", "unres_24.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks decl_init source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_25_decl_init)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { let x: int = ud0; })XXX",
            "unres_25.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks let_unresolved_type source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_26_let_unresolved_type)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { let x: Missing0 = 1; })XXX",
            "unres_26.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks return_unresolved source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_27_return_unresolved)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function f(): int { return ur3; })XXX",
            "unres_27.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks return_member source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_28_return_member)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function f(): int { return ur4.field; })XXX",
            "unres_28.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks ternary_cond source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_29_ternary_cond)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let x: int = ut0 ? 1 : 2; })XXX", "unres_29.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_in_class source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_30_member_in_class)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    class UK0 { m(): void { this.p; } })XXX",
            "unres_30.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_unresolved_in_class source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_31_member_unresolved_in_class)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    class UK1 { m(): void { uk0.f; } })XXX",
            "unres_31.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks undef_in_lambda source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_32_undef_in_lambda)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let f = (): void => { uk1.f; }; })XXX", "unres_32.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks multi_error source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_33_multi_error)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { ue0.f; ue1.f; ue2 = 1; })XXX", "unres_33.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks annotation_as_value source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_34_annotation_as_value)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    @interface Tag0 { }
    function main(): void { let t = Tag0; })XXX",
            "unres_34.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks annotation_as_value2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_35_annotation_as_value2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    @interface Tag1 { v: string }
    function main(): void { let t = Tag1; })XXX",
            "unres_35.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks anno_and_unresolved source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_36_anno_and_unresolved)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    @interface Tag2 { }
    function main(): void { let t = Tag2; ue3.f; })XXX",
            "unres_36.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks nested_member_call_left source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_37_nested_member_call_left)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let x: int = un11.a.b; })XXX", "unres_37.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_of_return source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_38_member_of_return)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { un12.field = un13; })XXX",
            "unres_38.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks assign_chain source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_39_assign_chain)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { ua0 = ua1; })XXX",
            "unres_39.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks update_in_loop source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_40_update_in_loop)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { for (let i = 0; i < 3; i++) { uc3++; } })XXX", "unres_40.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_in_if source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_41_member_in_if)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { if (ub0.field) { } else { } })XXX", "unres_41.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_in_try source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_42_member_in_try)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { try { ut1.f; } catch (e) { } })XXX", "unres_42.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_in_switch source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_43_member_in_switch)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { switch (us1.f) { case 1: break; default: break; } })XXX", "unres_43.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks while_unresolved source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_44_while_unresolved)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { while (uw0.field) { break; } })XXX", "unres_44.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks do_while_unresolved source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_45_do_while_unresolved)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { do { break; } while (ud1.field); })XXX", "unres_45.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks for_of_unresolved source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_46_for_of_unresolved)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { for (let v of uo0.items) { } })XXX", "unres_46.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_read_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_47_member_read_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { vm0.prop; })XXX",
            "unres_47.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_write_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_48_member_write_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { vm1.prop = 9; })XXX",
            "unres_48.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks computed_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_49_computed_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { vm2[1]; })XXX",
            "unres_49.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks assign_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_50_assign_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { va0 = 5; })XXX",
            "unres_50.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks arith_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_51_arith_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let x: int = vn0 * 2; })XXX", "unres_51.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks logical_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_52_logical_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let b: boolean = vn1 || false; })XXX", "unres_52.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks unary_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_53_unary_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { let x: int = -vn2; })XXX",
            "unres_53.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks update_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_54_update_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { vc0++; })XXX",
            "unres_54.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks plain_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_55_plain_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { vs0; })XXX", "unres_55.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks decl_init_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_56_decl_init_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { let s: string = vd0; })XXX",
            "unres_56.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_deep_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_57_member_deep_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { vm3.a.b.c.d; })XXX",
            "unres_57.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks multi_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_58_multi_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { ve0.f; ve1 = 2; ve2.g; })XXX", "unres_58.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks ternary_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_59_ternary_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let x: int = vt0 ? 1 : 2; })XXX", "unres_59.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks class_method_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_60_class_method_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    class VK2 { m(): void { vk0.f; } })XXX",
            "unres_60.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks lambda_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_61_lambda_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let f = (): int => vl0.f; })XXX", "unres_61.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks return_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_62_return_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function f(): int { return vr0; })XXX",
            "unres_62.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks if_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_63_if_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { if (vb1.field) { } })XXX",
            "unres_63.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks loop_v2 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_64_loop_v2)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { while (vw1.field) { break; } })XXX", "unres_64.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_write_deep_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_65_member_write_deep_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { w0.a.b = 1; })XXX",
            "unres_65.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks computed_deep_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_66_computed_deep_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { w1.a[0]; })XXX",
            "unres_66.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks assign_in_decl_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_67_assign_in_decl_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let x: int = wa0 = 1; })XXX", "unres_67.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks multi_stmt_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_68_multi_stmt_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let a: int = wb0 + 1; wc1.f; wd2 = 3; })XXX", "unres_68.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks nested_paren_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_69_nested_paren_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let x: int = ((we0.f)); })XXX", "unres_69.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks unary_nested_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_70_unary_nested_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let x: int = -(-wf0); })XXX", "unres_70.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks logical_chain_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_71_logical_chain_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let b: boolean = wg0 && wh1 && true; })XXX", "unres_71.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks nullish_chain_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_72_nullish_chain_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let x: int = wi0 ?? wj1 ?? 0; })XXX", "unres_72.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks compare_chain_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_73_compare_chain_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let b: boolean = wk0 > wl1; })XXX", "unres_73.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks ternary_member_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_74_ternary_member_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let x: int = wm0.f ? 1 : 2; })XXX", "unres_74.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks ternary_alt_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_75_ternary_alt_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let x: int = true ? wn0.f : 2; })XXX", "unres_75.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks update_in_expr_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_76_update_in_expr_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { let x: int = wo0++; })XXX",
            "unres_76.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_return_class_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_77_member_return_class_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    class WK3 { m(): int { return wk3.f; } })XXX",
            "unres_77.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_lambda_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_78_member_lambda_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let f = (): int => { return wl2.f; }; })XXX", "unres_78.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks assign_in_if_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_79_assign_in_if_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { if (true) { wm2 = 1; } })XXX", "unres_79.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks member_do_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_80_member_do_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { do { wn2.f; } while (false); })XXX", "unres_80.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks computed_for_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_81_computed_for_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { for (let i = 0; i < 3; i++) { wo2[i]; } })XXX", "unres_81.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks decl_multi_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_82_decl_multi_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let a: int = wp0; let b: int = wp1; let c: int = wp2; })XXX",
            "unres_82.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks type_and_value_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_83_type_and_value_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function main(): void { let a: Missing1 = wq0; })XXX", "unres_83.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks annotation_mix_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_84_annotation_mix_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    @interface Tag3 { }
    function main(): void { let t = Tag3; let u: int = wr0; })XXX",
            "unres_84.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks self_assign_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_85_self_assign_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR, R"XXX(    function main(): void { ws0.f = ws0.f; })XXX",
            "unres_85.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}
// Checks nested_func_v3 source. The unresolved identifier used to be able to crash the
// checker validation helpers (issue #12320); it must now be reported as an
// ordinary semantic error leaving a walkable AST behind.
TEST_F(NullSafetyMemoryTest, CheckUnresolved_86_nested_func_v3)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, ES2PANDA_STATE_ERROR,
            R"XXX(    function outer(): void { function inner(): void { wt0.f; } inner(); })XXX", "unres_86.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        auto *errors = GetImpl()->GetAllErrorMessages(GetContext());
        ASSERT_NE(errors, nullptr);
        EXPECT_FALSE(std::string(errors).empty());
        size_t nodeCount = 0U;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            (void)node;
            ++nodeCount;
        });
        EXPECT_GT(nodeCount, 0U);
    }
}

// ---------------------------------------------------------------------------
// Section 4: dumper null-safety regression
// Dumps must cover ambient declarations, annotated methods and interfaces without crashing (issue #12320).
// ---------------------------------------------------------------------------

// Dumps a decl_class_basic AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_00_decl_class_basic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC0 { m(): void })XXX", "dumpreg_00.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("m"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a decl_class_params AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_01_decl_class_params)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC1 { m(a: int): string
     n(b: boolean, c: string): int })XXX",
            "dumpreg_01.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("n"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a decl_class_optional AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_02_decl_class_optional)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC2 { m(a?: int): void
     n(b: string?): string })XXX",
            "dumpreg_02.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("n"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a decl_class_static AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_03_decl_class_static)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC3 { static m(): void
     static f: int })XXX",
            "dumpreg_03.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("static"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a decl_class_public AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_04_decl_class_public)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC4 { public m(): void })XXX", "dumpreg_04.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("m"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a decl_class_multi AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_05_decl_class_multi)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC5 { a(): void
     b(): int
     c(): string })XXX",
            "dumpreg_05.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("c"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a decl_class_props AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_06_decl_class_props)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC6 { p: int
     q: string })XXX",
            "dumpreg_06.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("q"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a decl_class_overloads AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_07_decl_class_overloads)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC7 { m(a: int): void
     m(a: string): void })XXX",
            "dumpreg_07.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("m"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a decl_func AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_08_decl_func)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare function df(a: int): string)XXX", "dumpreg_08.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("df"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a decl_interface AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_09_decl_interface)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare interface DI0 { m(): void })XXX", "dumpreg_09.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("m"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a interface_basic AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_10_interface_basic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IB0 { m(): void })XXX", "dumpreg_10.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("m"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a interface_props AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_11_interface_props)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IP0 { p: int
     m(): void })XXX",
            "dumpreg_11.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("p"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a interface_getset AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_12_interface_getset)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IG0 { get v(): int
     set v(n: int) })XXX",
            "dumpreg_12.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("v"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a class_methods AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_13_class_methods)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CM0 { m(): void {} n(): int { return 1; } })XXX", "dumpreg_13.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("n"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a class_getset AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_14_class_getset)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CG0 { p: int = 0
     get v(): int { return this.p }
     set v(n: int) { this.p = n } })XXX",
            "dumpreg_14.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("v"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a class_static AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_15_class_static)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CS0 { static f: int = 1
     static m(): void {} })XXX",
            "dumpreg_15.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("static"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a class_annot AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_16_class_annot)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface Mark0 { }
    class CA0 { @Mark0 m(): void {} })XXX",
            "dumpreg_16.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("m"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a class_annot_params AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_17_class_annot_params)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface Mark1 { v: string }
    class CA1 { @Mark1('x') m(a: int): string { return '' } })XXX",
            "dumpreg_17.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("Mark1"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a class_ctor AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_18_class_ctor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CC0 { v: int
     constructor(n: int) { this.v = n } })XXX",
            "dumpreg_18.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("constructor"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a class_generic AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_19_class_generic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CG1<T> { v: T })XXX", "dumpreg_19.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("CG1"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a class_extends AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_20_class_extends)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class B0 { b: int = 1 }
    class D0 extends B0 { d: int = 2 })XXX",
            "dumpreg_20.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("D0"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a class_readonly AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_21_class_readonly)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CR0 { readonly k: int = 9 })XXX", "dumpreg_21.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("k"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a class_private AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_22_class_private)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CP0 { private p: int = 1
     public q: int = 2 })XXX",
            "dumpreg_22.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("q"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a class_nested AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_23_class_nested)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CN0 { inner: CN1 = new CN1()
     class CN1 { v: int = 1 } })XXX",
            "dumpreg_23.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("CN1"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a enum_basic AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_24_enum_basic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    enum E0 { A, B, C })XXX", "dumpreg_24.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("E0"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a enum_values AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_25_enum_values)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    enum E1 { X = 1, Y = 2, Z = 3 })XXX", "dumpreg_25.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("Y"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a function_body AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_26_function_body)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function fb(a: int): int { return a + 1; })XXX", "dumpreg_26.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("fb"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a function_lambda AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_27_function_lambda)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function fl(): void { let f = (x: int): int => x; })XXX", "dumpreg_27.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("fl"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a class_implements AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_28_class_implements)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class VW0 { m(): void {} }
    class VX0 implements VW0 { m(): void {} })XXX",
            "dumpreg_28.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("VX0"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a abstract_method AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_29_abstract_method)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    abstract class VY0 { m(): void {} })XXX", "dumpreg_29.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("VY0"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a decl_class_extends AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_30_decl_class_extends)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class VZ0 extends Unknown0 { m(): void })XXX", "dumpreg_30.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("VZ0"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a multi_annotations AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_31_multi_annotations)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface A0 { }
    @interface B0 { }
    class WA0 { @A0 @B0 m(): void {} })XXX",
            "dumpreg_31.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("m"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a annot_getter AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_32_annot_getter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface A1 { }
    class WB0 { @A1 get v(): int { return 1 } })XXX",
            "dumpreg_32.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("v"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a annot_setter AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_33_annot_setter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface A2 { }
    class WC0 { p: int = 0
     @A2 set v(n: int) { this.p = n } })XXX",
            "dumpreg_33.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("v"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a decl_class_annot AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_34_decl_class_annot)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface A3 { }
    declare class WD0 { @A3 m(): void })XXX",
            "dumpreg_34.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("m"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a interface_extends AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_35_interface_extends)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IE0 { m(): void }
    interface IF0 extends IE0 { m(): void })XXX",
            "dumpreg_35.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("IF0"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a function_annot AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_36_function_annot)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface A5 { }
    @A5
    function FA0(): void {})XXX",
            "dumpreg_36.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("FA0"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a decl_class_many AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_37_decl_class_many)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class WH0 { a(): void
     b(): void
     c(): void
     d(): void })XXX",
            "dumpreg_37.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("d"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a jsdoc_class AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_38_jsdoc_class)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    /** doc */
    class WJ0 { m(): void {} })XXX",
            "dumpreg_38.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("WJ0"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}
// Dumps a mixed_members AST. Method definitions without a value (ambient declarations)
// and annotated methods used to risk null dereferences in DumpSingleOverload
// (issue #12320); the dump must now succeed and stay stable.
TEST_F(NullSafetyMemoryTest, DumpRegression_39_mixed_members)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class WK1 { a: int = 1
     static b: int = 2
     m(): void {}
     get c(): int { return 1 }
     n(x: string): string { return x } })XXX",
            "dumpreg_39.ats")
    {
        ASSERT_NE(GetAst(), nullptr);
        size_t methodCount = 0U;
        GetAst()->IterateRecursively(
            [&](ir_alias::AstNode *node) { methodCount += node->IsMethodDefinition() ? 1U : 0U; });
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        ASSERT_NE(first, nullptr);
        const std::string original(first);
        EXPECT_NE(original.find("n"), std::string::npos);
        auto *second = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(second, first);
        EXPECT_STREQ(second, original.c_str());
        (void)methodCount;
    }
}

// ---------------------------------------------------------------------------
// Section 5: identifier-name read leak checks
// Repeated IdentifierNameConst reads must not grow the EHeap arena.
// ---------------------------------------------------------------------------

// Repeated identifier-name reads over Probe00 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_00)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe00 { alpha00: int = 1; beta00: string = 'b'; })XXX",
            "names_00.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha00")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha00");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 100U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha00");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha00Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha00Renamed");
    }
}
// Repeated identifier-name reads over Probe01 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_01)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe01 { alpha01: int = 1; beta01: string = 'b'; })XXX",
            "names_01.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha01")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha01");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 200U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha01");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha01Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha01Renamed");
    }
}
// Repeated identifier-name reads over Probe02 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_02)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe02 { alpha02: int = 1; beta02: string = 'b'; })XXX",
            "names_02.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha02")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha02");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 300U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha02");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha02Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha02Renamed");
    }
}
// Repeated identifier-name reads over Probe03 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_03)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe03 { alpha03: int = 1; beta03: string = 'b'; })XXX",
            "names_03.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha03")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha03");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 400U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha03");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha03Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha03Renamed");
    }
}
// Repeated identifier-name reads over Probe04 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_04)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe04 { alpha04: int = 1; beta04: string = 'b'; })XXX",
            "names_04.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha04")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha04");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 500U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha04");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha04Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha04Renamed");
    }
}
// Repeated identifier-name reads over Probe05 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_05)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe05 { alpha05: int = 1; beta05: string = 'b'; })XXX",
            "names_05.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha05")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha05");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 100U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha05");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha05Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha05Renamed");
    }
}
// Repeated identifier-name reads over Probe06 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_06)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe06 { alpha06: int = 1; beta06: string = 'b'; })XXX",
            "names_06.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha06")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha06");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 200U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha06");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha06Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha06Renamed");
    }
}
// Repeated identifier-name reads over Probe07 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_07)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe07 { alpha07: int = 1; beta07: string = 'b'; })XXX",
            "names_07.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha07")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha07");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 300U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha07");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha07Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha07Renamed");
    }
}
// Repeated identifier-name reads over Probe08 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_08)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe08 { alpha08: int = 1; beta08: string = 'b'; })XXX",
            "names_08.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha08")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha08");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 400U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha08");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha08Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha08Renamed");
    }
}
// Repeated identifier-name reads over Probe09 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_09)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe09 { alpha09: int = 1; beta09: string = 'b'; })XXX",
            "names_09.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha09")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha09");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 500U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha09");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha09Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha09Renamed");
    }
}
// Repeated identifier-name reads over Probe10 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_10)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe10 { alpha10: int = 1; beta10: string = 'b'; })XXX",
            "names_10.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha10")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha10");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 100U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha10");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha10Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha10Renamed");
    }
}
// Repeated identifier-name reads over Probe11 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_11)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe11 { alpha11: int = 1; beta11: string = 'b'; })XXX",
            "names_11.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha11")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha11");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 200U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha11");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha11Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha11Renamed");
    }
}
// Repeated identifier-name reads over Probe12 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_12)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe12 { alpha12: int = 1; beta12: string = 'b'; })XXX",
            "names_12.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha12")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha12");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 300U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha12");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha12Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha12Renamed");
    }
}
// Repeated identifier-name reads over Probe13 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_13)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe13 { alpha13: int = 1; beta13: string = 'b'; })XXX",
            "names_13.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha13")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha13");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 400U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha13");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha13Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha13Renamed");
    }
}
// Repeated identifier-name reads over Probe14 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_14)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe14 { alpha14: int = 1; beta14: string = 'b'; })XXX",
            "names_14.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha14")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha14");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 500U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha14");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha14Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha14Renamed");
    }
}
// Repeated identifier-name reads over Probe15 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_15)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe15 { alpha15: int = 1; beta15: string = 'b'; })XXX",
            "names_15.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha15")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha15");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 100U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha15");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha15Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha15Renamed");
    }
}
// Repeated identifier-name reads over Probe16 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_16)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe16 { alpha16: int = 1; beta16: string = 'b'; })XXX",
            "names_16.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha16")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha16");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 200U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha16");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha16Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha16Renamed");
    }
}
// Repeated identifier-name reads over Probe17 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_17)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe17 { alpha17: int = 1; beta17: string = 'b'; })XXX",
            "names_17.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha17")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha17");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 300U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha17");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha17Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha17Renamed");
    }
}
// Repeated identifier-name reads over Probe18 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_18)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe18 { alpha18: int = 1; beta18: string = 'b'; })XXX",
            "names_18.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha18")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha18");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 400U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha18");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha18Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha18Renamed");
    }
}
// Repeated identifier-name reads over Probe19 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_19)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe19 { alpha19: int = 1; beta19: string = 'b'; })XXX",
            "names_19.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha19")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha19");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 500U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha19");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha19Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha19Renamed");
    }
}
// Repeated identifier-name reads over Probe20 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_20)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe20 { alpha20: int = 1; beta20: string = 'b'; })XXX",
            "names_20.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha20")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha20");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 100U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha20");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha20Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha20Renamed");
    }
}
// Repeated identifier-name reads over Probe21 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_21)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe21 { alpha21: int = 1; beta21: string = 'b'; })XXX",
            "names_21.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha21")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha21");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 200U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha21");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha21Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha21Renamed");
    }
}
// Repeated identifier-name reads over Probe22 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_22)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe22 { alpha22: int = 1; beta22: string = 'b'; })XXX",
            "names_22.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha22")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha22");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 300U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha22");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha22Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha22Renamed");
    }
}
// Repeated identifier-name reads over Probe23 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_23)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe23 { alpha23: int = 1; beta23: string = 'b'; })XXX",
            "names_23.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha23")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha23");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 400U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha23");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha23Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha23Renamed");
    }
}
// Repeated identifier-name reads over Probe24 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_24)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe24 { alpha24: int = 1; beta24: string = 'b'; })XXX",
            "names_24.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha24")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha24");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 500U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha24");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha24Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha24Renamed");
    }
}
// Repeated identifier-name reads over Probe25 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_25)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe25 { alpha25: int = 1; beta25: string = 'b'; })XXX",
            "names_25.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha25")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha25");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 100U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha25");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha25Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha25Renamed");
    }
}
// Repeated identifier-name reads over Probe26 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_26)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe26 { alpha26: int = 1; beta26: string = 'b'; })XXX",
            "names_26.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha26")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha26");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 200U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha26");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha26Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha26Renamed");
    }
}
// Repeated identifier-name reads over Probe27 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_27)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe27 { alpha27: int = 1; beta27: string = 'b'; })XXX",
            "names_27.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha27")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha27");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 300U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha27");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha27Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha27Renamed");
    }
}
// Repeated identifier-name reads over Probe28 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_28)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe28 { alpha28: int = 1; beta28: string = 'b'; })XXX",
            "names_28.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha28")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha28");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 400U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha28");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha28Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha28Renamed");
    }
}
// Repeated identifier-name reads over Probe29 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_29)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe29 { alpha29: int = 1; beta29: string = 'b'; })XXX",
            "names_29.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha29")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha29");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 500U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha29");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha29Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha29Renamed");
    }
}
// Repeated identifier-name reads over Probe30 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_30)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe30 { alpha30: int = 1; beta30: string = 'b'; })XXX",
            "names_30.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha30")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha30");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 100U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha30");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha30Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha30Renamed");
    }
}
// Repeated identifier-name reads over Probe31 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_31)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe31 { alpha31: int = 1; beta31: string = 'b'; })XXX",
            "names_31.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha31")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha31");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 200U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha31");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha31Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha31Renamed");
    }
}
// Repeated identifier-name reads over Probe32 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_32)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe32 { alpha32: int = 1; beta32: string = 'b'; })XXX",
            "names_32.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha32")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha32");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 300U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha32");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha32Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha32Renamed");
    }
}
// Repeated identifier-name reads over Probe33 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_33)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe33 { alpha33: int = 1; beta33: string = 'b'; })XXX",
            "names_33.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha33")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha33");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 400U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha33");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha33Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha33Renamed");
    }
}
// Repeated identifier-name reads over Probe34 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_34)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe34 { alpha34: int = 1; beta34: string = 'b'; })XXX",
            "names_34.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha34")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha34");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 500U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha34");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha34Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha34Renamed");
    }
}
// Repeated identifier-name reads over Probe35 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_35)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe35 { alpha35: int = 1; beta35: string = 'b'; })XXX",
            "names_35.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha35")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha35");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 100U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha35");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha35Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha35Renamed");
    }
}
// Repeated identifier-name reads over Probe36 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_36)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe36 { alpha36: int = 1; beta36: string = 'b'; })XXX",
            "names_36.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha36")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha36");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 200U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha36");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha36Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha36Renamed");
    }
}
// Repeated identifier-name reads over Probe37 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_37)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe37 { alpha37: int = 1; beta37: string = 'b'; })XXX",
            "names_37.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha37")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha37");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 300U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha37");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha37Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha37Renamed");
    }
}
// Repeated identifier-name reads over Probe38 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_38)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe38 { alpha38: int = 1; beta38: string = 'b'; })XXX",
            "names_38.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha38")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha38");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 400U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha38");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha38Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha38Renamed");
    }
}
// Repeated identifier-name reads over Probe39 must reuse cached API results and
// must not grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakNameReads_39)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe39 { alpha39: int = 1; beta39: string = 'b'; })XXX",
            "names_39.ats")
    {
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("alpha39")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(id);
        ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha39");
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t nameReads = 500U;
        for (size_t i = 0; i < nameReads; ++i) {
            ASSERT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha39");
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
        id->SetName(ark::es2panda::util::StringView("alpha39Renamed"));
        EXPECT_STREQ(GetImpl()->IdentifierNameConst(GetContext(), peer), "alpha39Renamed");
    }
}

// ---------------------------------------------------------------------------
// Section 6: class-body read leak checks
// Repeated ClassDefinitionBodyConst reads must not grow the EHeap arena.
// ---------------------------------------------------------------------------

// Repeated class-body reads of Body00 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_00)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body00 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
    })XXX",
            "body_00.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body00")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 3U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 100U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body01 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_01)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body01 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
    })XXX",
            "body_01.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body01")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 4U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 200U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body02 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_02)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body02 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
    })XXX",
            "body_02.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body02")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 5U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 300U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body03 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_03)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body03 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
        f5: int = 5;
    })XXX",
            "body_03.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body03")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 6U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 400U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body04 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_04)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body04 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
        f5: int = 5;
        f6: int = 6;
    })XXX",
            "body_04.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body04")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 7U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 100U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body05 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_05)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body05 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
        f5: int = 5;
        f6: int = 6;
        f7: int = 7;
    })XXX",
            "body_05.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body05")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 8U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 200U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body06 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_06)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body06 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
    })XXX",
            "body_06.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body06")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 3U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 300U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body07 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_07)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body07 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
    })XXX",
            "body_07.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body07")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 4U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 400U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body08 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_08)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body08 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
    })XXX",
            "body_08.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body08")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 5U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 100U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body09 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_09)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body09 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
        f5: int = 5;
    })XXX",
            "body_09.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body09")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 6U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 200U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body10 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_10)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body10 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
        f5: int = 5;
        f6: int = 6;
    })XXX",
            "body_10.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body10")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 7U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 300U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body11 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_11)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body11 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
        f5: int = 5;
        f6: int = 6;
        f7: int = 7;
    })XXX",
            "body_11.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body11")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 8U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 400U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body12 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_12)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body12 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
    })XXX",
            "body_12.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body12")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 3U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 100U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body13 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_13)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body13 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
    })XXX",
            "body_13.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body13")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 4U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 200U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body14 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_14)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body14 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
    })XXX",
            "body_14.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body14")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 5U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 300U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body15 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_15)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body15 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
        f5: int = 5;
    })XXX",
            "body_15.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body15")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 6U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 400U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body16 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_16)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body16 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
        f5: int = 5;
        f6: int = 6;
    })XXX",
            "body_16.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body16")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 7U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 100U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body17 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_17)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body17 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
        f5: int = 5;
        f6: int = 6;
        f7: int = 7;
    })XXX",
            "body_17.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body17")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 8U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 200U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body18 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_18)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body18 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
    })XXX",
            "body_18.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body18")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 3U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 300U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body19 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_19)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body19 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
    })XXX",
            "body_19.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body19")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 4U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 400U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body20 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_20)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body20 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
    })XXX",
            "body_20.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body20")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 5U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 100U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body21 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_21)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body21 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
        f5: int = 5;
    })XXX",
            "body_21.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body21")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 6U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 200U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body22 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_22)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body22 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
        f5: int = 5;
        f6: int = 6;
    })XXX",
            "body_22.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body22")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 7U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 300U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body23 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_23)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body23 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
        f5: int = 5;
        f6: int = 6;
        f7: int = 7;
    })XXX",
            "body_23.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body23")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 8U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 400U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body24 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_24)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body24 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
    })XXX",
            "body_24.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body24")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 3U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 100U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body25 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_25)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body25 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
    })XXX",
            "body_25.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body25")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 4U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 200U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body26 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_26)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body26 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
    })XXX",
            "body_26.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body26")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 5U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 300U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body27 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_27)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body27 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
        f5: int = 5;
    })XXX",
            "body_27.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body27")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 6U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 400U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body28 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_28)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body28 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
        f5: int = 5;
        f6: int = 6;
    })XXX",
            "body_28.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body28")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 7U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 100U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}
// Repeated class-body reads of Body29 must return the same snapshot and must not
// grow the EHeap arena (memory-leak check).
TEST_F(NullSafetyMemoryTest, LeakBodyReads_29)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Body29 {
        f0: int = 0;
        f1: int = 1;
        f2: int = 2;
        f3: int = 3;
        f4: int = 4;
        f5: int = 5;
        f6: int = 6;
        f7: int = 7;
    })XXX",
            "body_29.ats")
    {
        ir_alias::ClassDefinition *definition = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsClassDefinition() && node->AsClassDefinition()->Ident()->Name().Is("Body29")) {
                definition = node->AsClassDefinition();
            }
        });
        ASSERT_NE(definition, nullptr);
        auto *peer = reinterpret_cast<es2panda_AstNode *>(definition);
        size_t count = 0U;
        auto *oldBody = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &count);
        ASSERT_GE(count, 8U);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t bodyReads = 200U;
        for (size_t i = 0; i < bodyReads; ++i) {
            size_t length = 0U;
            auto *body = GetImpl()->ClassDefinitionBodyConst(GetContext(), peer, &length);
            ASSERT_EQ(length, count);
            EXPECT_EQ(body, oldBody);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), before);
    }
}

// ---------------------------------------------------------------------------
// Section 7: dump read leak checks
// Repeated AstNodeDumpEtsSrcConst reads must stay identical and bounded.
// ---------------------------------------------------------------------------

// Repeated dumps of the decl_class_basic AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_00_decl_class_basic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC0 { m(): void })XXX", "dumpreads_00.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the decl_class_params AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_01_decl_class_params)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC1 { m(a: int): string
     n(b: boolean, c: string): int })XXX",
            "dumpreads_01.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the decl_class_optional AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_02_decl_class_optional)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC2 { m(a?: int): void
     n(b: string?): string })XXX",
            "dumpreads_02.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the decl_class_static AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_03_decl_class_static)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC3 { static m(): void
     static f: int })XXX",
            "dumpreads_03.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the decl_class_public AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_04_decl_class_public)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC4 { public m(): void })XXX", "dumpreads_04.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the decl_class_multi AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_05_decl_class_multi)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC5 { a(): void
     b(): int
     c(): string })XXX",
            "dumpreads_05.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the decl_class_props AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_06_decl_class_props)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC6 { p: int
     q: string })XXX",
            "dumpreads_06.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the decl_class_overloads AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_07_decl_class_overloads)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC7 { m(a: int): void
     m(a: string): void })XXX",
            "dumpreads_07.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the decl_func AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_08_decl_func)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare function df(a: int): string)XXX", "dumpreads_08.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the decl_interface AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_09_decl_interface)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare interface DI0 { m(): void })XXX", "dumpreads_09.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the interface_basic AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_10_interface_basic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IB0 { m(): void })XXX", "dumpreads_10.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the interface_props AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_11_interface_props)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IP0 { p: int
     m(): void })XXX",
            "dumpreads_11.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the interface_getset AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_12_interface_getset)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IG0 { get v(): int
     set v(n: int) })XXX",
            "dumpreads_12.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the class_methods AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_13_class_methods)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CM0 { m(): void {} n(): int { return 1; } })XXX", "dumpreads_13.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the class_getset AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_14_class_getset)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CG0 { p: int = 0
     get v(): int { return this.p }
     set v(n: int) { this.p = n } })XXX",
            "dumpreads_14.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the class_static AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_15_class_static)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CS0 { static f: int = 1
     static m(): void {} })XXX",
            "dumpreads_15.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the class_annot AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_16_class_annot)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface Mark0 { }
    class CA0 { @Mark0 m(): void {} })XXX",
            "dumpreads_16.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the class_annot_params AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_17_class_annot_params)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface Mark1 { v: string }
    class CA1 { @Mark1('x') m(a: int): string { return '' } })XXX",
            "dumpreads_17.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the class_ctor AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_18_class_ctor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CC0 { v: int
     constructor(n: int) { this.v = n } })XXX",
            "dumpreads_18.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the class_generic AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_19_class_generic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CG1<T> { v: T })XXX", "dumpreads_19.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the class_extends AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_20_class_extends)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class B0 { b: int = 1 }
    class D0 extends B0 { d: int = 2 })XXX",
            "dumpreads_20.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the class_readonly AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_21_class_readonly)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CR0 { readonly k: int = 9 })XXX", "dumpreads_21.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the class_private AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_22_class_private)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CP0 { private p: int = 1
     public q: int = 2 })XXX",
            "dumpreads_22.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the class_nested AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_23_class_nested)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CN0 { inner: CN1 = new CN1()
     class CN1 { v: int = 1 } })XXX",
            "dumpreads_23.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the enum_basic AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_24_enum_basic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    enum E0 { A, B, C })XXX", "dumpreads_24.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the enum_values AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_25_enum_values)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    enum E1 { X = 1, Y = 2, Z = 3 })XXX", "dumpreads_25.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the function_body AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_26_function_body)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function fb(a: int): int { return a + 1; })XXX", "dumpreads_26.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the function_lambda AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_27_function_lambda)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function fl(): void { let f = (x: int): int => x; })XXX",
            "dumpreads_27.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the class_implements AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_28_class_implements)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class VW0 { m(): void {} }
    class VX0 implements VW0 { m(): void {} })XXX",
            "dumpreads_28.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the abstract_method AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_29_abstract_method)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    abstract class VY0 { m(): void {} })XXX", "dumpreads_29.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the decl_class_extends AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_30_decl_class_extends)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class VZ0 extends Unknown0 { m(): void })XXX", "dumpreads_30.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the multi_annotations AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_31_multi_annotations)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface A0 { }
    @interface B0 { }
    class WA0 { @A0 @B0 m(): void {} })XXX",
            "dumpreads_31.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the annot_getter AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_32_annot_getter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface A1 { }
    class WB0 { @A1 get v(): int { return 1 } })XXX",
            "dumpreads_32.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the annot_setter AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_33_annot_setter)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface A2 { }
    class WC0 { p: int = 0
     @A2 set v(n: int) { this.p = n } })XXX",
            "dumpreads_33.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the decl_class_annot AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_34_decl_class_annot)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface A3 { }
    declare class WD0 { @A3 m(): void })XXX",
            "dumpreads_34.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the interface_extends AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_35_interface_extends)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IE0 { m(): void }
    interface IF0 extends IE0 { m(): void })XXX",
            "dumpreads_35.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the function_annot AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_36_function_annot)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    @interface A5 { }
    @A5
    function FA0(): void {})XXX",
            "dumpreads_36.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the decl_class_many AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_37_decl_class_many)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class WH0 { a(): void
     b(): void
     c(): void
     d(): void })XXX",
            "dumpreads_37.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the jsdoc_class AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_38_jsdoc_class)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    /** doc */
    class WJ0 { m(): void {} })XXX",
            "dumpreads_38.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the mixed_members AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_39_mixed_members)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class WK1 { a: int = 1
     static b: int = 2
     m(): void {}
     get c(): int { return 1 }
     n(x: string): string { return x } })XXX",
            "dumpreads_39.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_simple_class AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_40_parse_simple_class)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P0 {})XXX", "dumpreads_40.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_two_classes AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_41_parse_two_classes)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P1 {}
    class P2 {})XXX",
            "dumpreads_41.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_class_fields AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_42_parse_class_fields)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P3 { a: int = 1; b: string = 'x'; c: boolean = true; })XXX",
            "dumpreads_42.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_class_methods AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_43_parse_class_methods)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P4 { m(): void {} n(): int { return 1; } })XXX", "dumpreads_43.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_class_getset AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_44_parse_class_getset)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P5 { p: int = 0
     get v(): int { return this.p }
     set v(n: int) { this.p = n } })XXX",
            "dumpreads_44.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_class_ctor AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_45_parse_class_ctor)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P6 { v: int
     constructor(n: int) { this.v = n } })XXX",
            "dumpreads_45.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_class_static AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_46_parse_class_static)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P7 { static f: int = 1
     static m(): void {} })XXX",
            "dumpreads_46.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_class_extends AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_47_parse_class_extends)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class P8 { b: int = 1 }
    class P9 extends P8 { d: int = 2 })XXX",
            "dumpreads_47.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_class_generic AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_48_parse_class_generic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class PA<T> { v: T })XXX", "dumpreads_48.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_class_readonly AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_49_parse_class_readonly)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class PB { readonly k: int = 9 })XXX", "dumpreads_49.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_class_optional_field AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_50_parse_class_optional_field)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class PC { f?: int })XXX", "dumpreads_50.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_abstract_class AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_51_parse_abstract_class)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    abstract class PD { m(): void {} })XXX", "dumpreads_51.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_interface_basic AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_52_parse_interface_basic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface PE { m(): void })XXX", "dumpreads_52.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_interface_props AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_53_parse_interface_props)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface PF { p: int
     m(): void })XXX",
            "dumpreads_53.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_enum_basic AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_54_parse_enum_basic)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    enum PG { A, B })XXX", "dumpreads_54.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_enum_values AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_55_parse_enum_values)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    enum PH { X = 1, Y = 2 })XXX", "dumpreads_55.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_declare_class AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_56_parse_declare_class)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class PI { m(): void })XXX", "dumpreads_56.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_declare_class_members AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_57_parse_declare_class_members)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class PJ { m(a: int): string
     f: boolean })XXX",
            "dumpreads_57.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_declare_function AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_58_parse_declare_function)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare function PK(a: int): string)XXX", "dumpreads_58.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_declare_interface AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_59_parse_declare_interface)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare interface PL { m(): void })XXX", "dumpreads_59.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_function_plain AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_60_parse_function_plain)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function PM(): void {})XXX", "dumpreads_60.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse_function_params AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_61_parse_function_params)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function PN(a: int, b: string): void {})XXX", "dumpreads_61.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse2_arrow_no_params AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_62_parse2_arrow_no_params)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function R0(): void { let f = (): int => 1; })XXX", "dumpreads_62.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse2_let_reassign AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_63_parse2_let_reassign)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function R1(): void { let s: string = 'a'; s = 'b'; })XXX",
            "dumpreads_63.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse2_empty_function AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_64_parse2_empty_function)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function R2(): void {})XXX", "dumpreads_64.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse2_nested_if AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_65_parse2_nested_if)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function R3(): void { if (true) { if (false) { } } })XXX",
            "dumpreads_65.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse2_empty_class_multi AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_66_parse2_empty_class_multi)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class R4 {}
    class R5 {}
    class R6 {})XXX",
            "dumpreads_66.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse2_method_default_params AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_67_parse2_method_default_params)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class R7 { m(a: int = 1, b?: string): void {} })XXX", "dumpreads_67.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse2_field_initializer_expr AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_68_parse2_field_initializer_expr)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class RA0 { a: int = 1 + 2 * 3; })XXX", "dumpreads_68.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse2_boolean_ops AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_69_parse2_boolean_ops)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function RC0(): void { let a: boolean = true; let b: boolean = !a; })XXX",
            "dumpreads_69.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse2_number_formats AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_70_parse2_number_formats)
{
    CONTEXT(ES2PANDA_STATE_PARSED,
            R"XXX(    function RD0(): void { let h: int = 0xFF; let o: int = 0o17; let b: int = 0b101; })XXX",
            "dumpreads_70.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse2_array_of_arrays AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_71_parse2_array_of_arrays)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function RE0(): void { let m: int[][] = [[1], [2, 3]]; })XXX",
            "dumpreads_71.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse2_conditional_nested AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_72_parse2_conditional_nested)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function RF0(): void { let x: int = true ? (false ? 1 : 2) : 3; })XXX",
            "dumpreads_72.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse2_class_static_block AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_73_parse2_class_static_block)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class RG0 { static { } })XXX", "dumpreads_73.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 300U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse2_string_simple AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_74_parse2_string_simple)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function RH0(): void { let s: string = 'a'; let t: string = s; })XXX",
            "dumpreads_74.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 100U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}
// Repeated dumps of the parse2_enum_nested_use AST must reuse the cached result: every read has to
// return the same pointer with identical contents (result-storage stability).
// Note: DumpEtsSrc internally performs small EHeap allocations per call for
// richer sources, so only the dedicated simple-shape tests below bound it.
TEST_F(NullSafetyMemoryTest, LeakDumpReads_75_parse2_enum_nested_use)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    enum RI0 { A }
    function RJ0(): void { let e: RI0 = RI0.A; })XXX",
            "dumpreads_75.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
    }
}

// ---------------------------------------------------------------------------
// Section 8: recheck leak and stability checks
// AstNodeRecheck must keep earlier dump results readable.
// ---------------------------------------------------------------------------

// Rechecks the flat_fields AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_00_flat_fields)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VF0 { a: int = 1; b: string = 'x'; c: boolean = true; })XXX",
            "recheck_00.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the with_method AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_01_with_method)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VF1 { m(): void {} })XXX", "recheck_01.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the field_read AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_02_field_read)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VF2 { value: int = 1; }
    function main(): void { let c = new VF2(); let v: int = c.value; })XXX",
            "recheck_02.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the method_call AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_03_method_call)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VF3 { m(): int { return 1; } }
    function main(): void { let c = new VF3(); let v: int = c.m(); })XXX",
            "recheck_03.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the local_assignment AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_04_local_assignment)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    function main(): void { let x: int = 1; x = 2; x = x + 1; })XXX",
            "recheck_04.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the binary_arith AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_05_binary_arith)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let a: int = 1; let b: int = a + 2; let c: boolean = a > b; })XXX",
            "recheck_05.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the logical_ops AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_06_logical_ops)
{
    CONTEXT(
        ES2PANDA_STATE_CHECKED,
        R"XXX(    function main(): void { let a: boolean = true; let b: boolean = !a; let c: boolean = a && b; })XXX",
        "recheck_06.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the update_expr AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_07_update_expr)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let i: int = 0; i++; ++i; i--; let j: int = i; })XXX", "recheck_07.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the unary_ops AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_08_unary_ops)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let a: int = 1; let b: int = -a; let c: int = +b; })XXX",
            "recheck_08.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the enum_access AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_09_enum_access)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    enum VG0 { RED, GREEN, BLUE }
    function main(): void { let c: VG0 = VG0.RED; })XXX",
            "recheck_09.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the enum_values AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_10_enum_values)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    enum VG1 { A = 1, B = 2, C = 3 }
    function main(): void { let o: VG1 = VG1.B; })XXX",
            "recheck_10.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the getter_usage AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_11_getter_usage)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG2 { p: int = 0
     get v(): int { return this.p } }
    function main(): void { let g = new VG2(); let n: int = g.v; })XXX",
            "recheck_11.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the setter_usage AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_12_setter_usage)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG3 { p: int = 0
     set v(n: int) { this.p = n } }
    function main(): void { let g = new VG3(); g.v = 5; })XXX",
            "recheck_12.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the static_method AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_13_static_method)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG4 { static m(): int { return 42; } }
    function main(): void { let v: int = VG4.m(); })XXX",
            "recheck_13.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the static_field AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_14_static_field)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG5 { static f: int = 7; }
    function main(): void { let v: int = VG5.f; })XXX",
            "recheck_14.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the inheritance AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_15_inheritance)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG6 { b: int = 1; }
    class VG7 extends VG6 { d: int = 2; }
    function main(): void { let x = new VG7(); let v: int = x.b; })XXX",
            "recheck_15.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the ctor_params AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_16_ctor_params)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG8 { v: int
     constructor(n: int) { this.v = n } }
    function main(): void { let p = new VG8(3); })XXX",
            "recheck_16.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the optional_param AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_17_optional_param)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    function opt(a: int, b?: int): int { return a; }
    function main(): void { let v: int = opt(1); })XXX",
            "recheck_17.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the lambda_simple AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_18_lambda_simple)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let f = (x: int): int => { return x + 1; }; let v: int = f(1); })XXX",
            "recheck_18.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the two_classes AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_19_two_classes)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VH0 { x: int = 1; }
    class VH1 { y: int = 2; }
    function main(): void { let a = new VH0(); let b = new VH1(); })XXX",
            "recheck_19.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the readonly_field AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_20_readonly_field)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VH2 { readonly k: int = 9; }
    function main(): void { let r = new VH2(); let v: int = r.k; })XXX",
            "recheck_20.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the const_local AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_21_const_local)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    function main(): void { const v: int = 5; let w: int = v; })XXX",
            "recheck_21.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the ternary AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_22_ternary)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let a: int = 1; let b: int = a > 0 ? 1 : 2; })XXX", "recheck_22.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the member_chain AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_23_member_chain)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VH3 { inner: VH4 = new VH4() }
    class VH4 { v: int = 3 }
    function main(): void { let o = new VH3(); let x: int = o.inner.v; })XXX",
            "recheck_23.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the assign_member AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_24_assign_member)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VH5 { v: int = 0 }
    function main(): void { let o = new VH5(); o.v = 1; })XXX",
            "recheck_24.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the nullish_optional AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_25_nullish_optional)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    function nn(a?: int): int { return a ?? 0; }
    function main(): void { let v: int = nn(); })XXX",
            "recheck_25.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the int_div_mod AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_26_int_div_mod)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let a: int = 7; let b: int = a / 2; let c: int = a % 3; })XXX",
            "recheck_26.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the string_concat AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_27_string_concat)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let a: string = 'x'; let b: string = a + 'y'; })XXX", "recheck_27.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the double_type AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_28_double_type)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let d: double = 1.5; let e: double = d + 0.5; })XXX", "recheck_28.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the long_type AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_29_long_type)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    function main(): void { let l: long = 100; })XXX", "recheck_29.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the boolean_field AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_30_boolean_field)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VJ0 { flag: boolean = false }
    function main(): void { let v = new VJ0(); let f: boolean = v.flag; })XXX",
            "recheck_30.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the nested_call AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_31_nested_call)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VJ1 { m(): int { return 1; } }
    function main(): void { let c = new VJ1(); let v: int = c.m() + c.m(); })XXX",
            "recheck_31.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the array_type AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_32_array_type)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    function main(): void { let a: int[] = [1, 2]; let n: int = a[0]; })XXX",
            "recheck_32.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the array_field AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_33_array_field)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VJ2 { arr: int[] = [1, 2] }
    function main(): void { let v = new VJ2(); let x: int = v.arr[0]; })XXX",
            "recheck_33.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the void_method AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_34_void_method)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VJ3 { m(): void { } }
    function main(): void { let c = new VJ3(); c.m(); })XXX",
            "recheck_34.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}
// Rechecks the if_chain AST after a clean check; previously returned dump snapshots
// must stay readable and stable (leak and dangling-pointer check).
TEST_F(NullSafetyMemoryTest, LeakRecheck_35_if_chain)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let a: int = 1; if (a > 0) { } else if (a > 1) { } else { } })XXX",
            "recheck_35.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        GetImpl()->AstNodeRecheck(GetContext(), root);
        ASSERT_NE(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_ERROR);
        EXPECT_STREQ(oldDump, original.c_str());
        auto *currentDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        EXPECT_EQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), currentDump);
    }
}

// ---------------------------------------------------------------------------
// Section 9: in-place edit stability
// Dump snapshots must reflect AST edits without dangling pointers.
// ---------------------------------------------------------------------------

// Dumps Probe00, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_00)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe00 { v: int = 1; })XXX", "edit_00.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe00")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe00Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe00Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe01, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_01)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe01 { v: int = 1; })XXX", "edit_01.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe01")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe01Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe01Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe02, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_02)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe02 { v: int = 1; })XXX", "edit_02.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe02")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe02Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe02Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe03, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_03)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe03 { v: int = 1; })XXX", "edit_03.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe03")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe03Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe03Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe04, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_04)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe04 { v: int = 1; })XXX", "edit_04.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe04")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe04Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe04Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe05, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_05)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe05 { v: int = 1; })XXX", "edit_05.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe05")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe05Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe05Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe06, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_06)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe06 { v: int = 1; })XXX", "edit_06.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe06")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe06Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe06Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe07, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_07)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe07 { v: int = 1; })XXX", "edit_07.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe07")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe07Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe07Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe08, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_08)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe08 { v: int = 1; })XXX", "edit_08.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe08")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe08Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe08Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe09, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_09)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe09 { v: int = 1; })XXX", "edit_09.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe09")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe09Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe09Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe10, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_10)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe10 { v: int = 1; })XXX", "edit_10.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe10")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe10Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe10Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe11, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_11)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe11 { v: int = 1; })XXX", "edit_11.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe11")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe11Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe11Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe12, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_12)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe12 { v: int = 1; })XXX", "edit_12.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe12")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe12Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe12Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe13, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_13)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe13 { v: int = 1; })XXX", "edit_13.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe13")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe13Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe13Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe14, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_14)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe14 { v: int = 1; })XXX", "edit_14.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe14")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe14Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe14Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe15, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_15)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe15 { v: int = 1; })XXX", "edit_15.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe15")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe15Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe15Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe16, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_16)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe16 { v: int = 1; })XXX", "edit_16.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe16")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe16Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe16Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}
// Dumps Probe17, renames the class in place and re-dumps; the edit must be visible
// while earlier snapshots stay valid (no use-after-free on dump results).
TEST_F(NullSafetyMemoryTest, DumpEditStability_17)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class Probe17 { v: int = 1; })XXX", "edit_17.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        ir_alias::Identifier *id = nullptr;
        GetAst()->IterateRecursively([&](ir_alias::AstNode *node) {
            if (node->IsIdentifier() && node->AsIdentifier()->Name().Is("Probe17")) {
                id = node->AsIdentifier();
            }
        });
        ASSERT_NE(id, nullptr);
        id->SetName(ark::es2panda::util::StringView("Probe17Renamed"));
        const std::string changed(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root));
        EXPECT_NE(changed, original);
        EXPECT_NE(changed.find("Probe17Renamed"), std::string::npos);
        EXPECT_STREQ(oldDump, original.c_str());
    }
}

// ---------------------------------------------------------------------------
// Section 10: checked-tree combined read leak checks
// Fully checked trees must keep dump and metadata reads leak-free.
// ---------------------------------------------------------------------------

// Checks flat_fields source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_00_flat_fields)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VF0 { a: int = 1; b: string = 'x'; c: boolean = true; })XXX",
            "checkread_00.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks with_method source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_01_with_method)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VF1 { m(): void {} })XXX", "checkread_01.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks field_read source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_02_field_read)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VF2 { value: int = 1; }
    function main(): void { let c = new VF2(); let v: int = c.value; })XXX",
            "checkread_02.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks method_call source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_03_method_call)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VF3 { m(): int { return 1; } }
    function main(): void { let c = new VF3(); let v: int = c.m(); })XXX",
            "checkread_03.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks local_assignment source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_04_local_assignment)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    function main(): void { let x: int = 1; x = 2; x = x + 1; })XXX",
            "checkread_04.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks binary_arith source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_05_binary_arith)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let a: int = 1; let b: int = a + 2; let c: boolean = a > b; })XXX",
            "checkread_05.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks logical_ops source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_06_logical_ops)
{
    CONTEXT(
        ES2PANDA_STATE_CHECKED,
        R"XXX(    function main(): void { let a: boolean = true; let b: boolean = !a; let c: boolean = a && b; })XXX",
        "checkread_06.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks update_expr source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_07_update_expr)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let i: int = 0; i++; ++i; i--; let j: int = i; })XXX", "checkread_07.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks unary_ops source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_08_unary_ops)
{
    CONTEXT(ES2PANDA_STATE_CHECKED,
            R"XXX(    function main(): void { let a: int = 1; let b: int = -a; let c: int = +b; })XXX",
            "checkread_08.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks enum_access source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_09_enum_access)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    enum VG0 { RED, GREEN, BLUE }
    function main(): void { let c: VG0 = VG0.RED; })XXX",
            "checkread_09.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks enum_values source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_10_enum_values)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    enum VG1 { A = 1, B = 2, C = 3 }
    function main(): void { let o: VG1 = VG1.B; })XXX",
            "checkread_10.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks getter_usage source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_11_getter_usage)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG2 { p: int = 0
     get v(): int { return this.p } }
    function main(): void { let g = new VG2(); let n: int = g.v; })XXX",
            "checkread_11.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks setter_usage source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_12_setter_usage)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG3 { p: int = 0
     set v(n: int) { this.p = n } }
    function main(): void { let g = new VG3(); g.v = 5; })XXX",
            "checkread_12.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks static_method source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_13_static_method)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG4 { static m(): int { return 42; } }
    function main(): void { let v: int = VG4.m(); })XXX",
            "checkread_13.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks static_field source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_14_static_field)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG5 { static f: int = 7; }
    function main(): void { let v: int = VG5.f; })XXX",
            "checkread_14.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks inheritance source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_15_inheritance)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG6 { b: int = 1; }
    class VG7 extends VG6 { d: int = 2; }
    function main(): void { let x = new VG7(); let v: int = x.b; })XXX",
            "checkread_15.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks ctor_params source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_16_ctor_params)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    class VG8 { v: int
     constructor(n: int) { this.v = n } }
    function main(): void { let p = new VG8(3); })XXX",
            "checkread_16.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}
// Checks optional_param source fully, then performs heavy dump and metadata reads on the
// checked tree; combined checking and reading must stay stable and leak-free
// for cached metadata.
TEST_F(NullSafetyMemoryTest, CheckThenLeakReads_17_optional_param)
{
    CONTEXT(ES2PANDA_STATE_CHECKED, R"XXX(    function opt(a: int, b?: int): int { return a; }
    function main(): void { let v: int = opt(1); })XXX",
            "checkread_17.ats")
    {
        ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const size_t dumpReads = 250U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        const auto beforeMetadata = ark::es2panda::EHeap::AllocatedSize();
        const size_t metadataReads = 250U;
        for (size_t i = 0; i < metadataReads; ++i) {
            ASSERT_EQ(GetImpl()->ContextState(GetContext()), ES2PANDA_STATE_CHECKED);
            ASSERT_NE(GetImpl()->ContextProgram(GetContext()), nullptr);
            ASSERT_NE(GetAst(), nullptr);
        }
        EXPECT_EQ(ark::es2panda::EHeap::AllocatedSize(), beforeMetadata);
    }
}

// ---------------------------------------------------------------------------
// Section 11: context lifecycle stability
// Sequential contexts over the same source must behave identically.
// ---------------------------------------------------------------------------

// Re-creates the context for the decl_class_basic source three times in a row; parsing must
// stay null-safe across context lifecycles and dumps must not interfere.
TEST_F(NullSafetyMemoryTest, ContextCycles_00_decl_class_basic)
{
    std::string first;
    {CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC0 { m(): void })XXX",
             "cycle_00.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
    ASSERT_NE(root, nullptr);
    first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
    ASSERT_FALSE(first.empty());
}
}  // namespace
{CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC0 { m(): void })XXX",
         "cycle_00.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
ASSERT_NE(root, nullptr);
EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
}
}
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC0 { m(): void })XXX", "cycle_00.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
    }
}
}
// Re-creates the context for the decl_class_params source three times in a row; parsing must
// stay null-safe across context lifecycles and dumps must not interfere.
TEST_F(NullSafetyMemoryTest, ContextCycles_01_decl_class_params)
{
    std::string first;
    {CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC1 { m(a: int): string
     n(b: boolean, c: string): int })XXX",
             "cycle_01.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
    ASSERT_NE(root, nullptr);
    first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
    ASSERT_FALSE(first.empty());
}
}
{CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC1 { m(a: int): string
     n(b: boolean, c: string): int })XXX",
         "cycle_01.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
ASSERT_NE(root, nullptr);
EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
}
}
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC1 { m(a: int): string
     n(b: boolean, c: string): int })XXX",
            "cycle_01.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
    }
}
}
// Re-creates the context for the decl_class_optional source three times in a row; parsing must
// stay null-safe across context lifecycles and dumps must not interfere.
TEST_F(NullSafetyMemoryTest, ContextCycles_02_decl_class_optional)
{
    std::string first;
    {CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC2 { m(a?: int): void
     n(b: string?): string })XXX",
             "cycle_02.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
    ASSERT_NE(root, nullptr);
    first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
    ASSERT_FALSE(first.empty());
}
}
{CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC2 { m(a?: int): void
     n(b: string?): string })XXX",
         "cycle_02.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
ASSERT_NE(root, nullptr);
EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
}
}
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC2 { m(a?: int): void
     n(b: string?): string })XXX",
            "cycle_02.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
    }
}
}
// Re-creates the context for the decl_class_static source three times in a row; parsing must
// stay null-safe across context lifecycles and dumps must not interfere.
TEST_F(NullSafetyMemoryTest, ContextCycles_03_decl_class_static)
{
    std::string first;
    {CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC3 { static m(): void
     static f: int })XXX",
             "cycle_03.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
    ASSERT_NE(root, nullptr);
    first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
    ASSERT_FALSE(first.empty());
}
}
{CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC3 { static m(): void
     static f: int })XXX",
         "cycle_03.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
ASSERT_NE(root, nullptr);
EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
}
}
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC3 { static m(): void
     static f: int })XXX",
            "cycle_03.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
    }
}
}
// Re-creates the context for the decl_class_public source three times in a row; parsing must
// stay null-safe across context lifecycles and dumps must not interfere.
TEST_F(NullSafetyMemoryTest, ContextCycles_04_decl_class_public)
{
    std::string first;
    {CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC4 { public m(): void })XXX",
             "cycle_04.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
    ASSERT_NE(root, nullptr);
    first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
    ASSERT_FALSE(first.empty());
}
}
{CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC4 { public m(): void })XXX",
         "cycle_04.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
ASSERT_NE(root, nullptr);
EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
}
}
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC4 { public m(): void })XXX", "cycle_04.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
    }
}
}
// Re-creates the context for the decl_class_multi source three times in a row; parsing must
// stay null-safe across context lifecycles and dumps must not interfere.
TEST_F(NullSafetyMemoryTest, ContextCycles_05_decl_class_multi)
{
    std::string first;
    {CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC5 { a(): void
     b(): int
     c(): string })XXX",
             "cycle_05.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
    ASSERT_NE(root, nullptr);
    first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
    ASSERT_FALSE(first.empty());
}
}
{CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC5 { a(): void
     b(): int
     c(): string })XXX",
         "cycle_05.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
ASSERT_NE(root, nullptr);
EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
}
}
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC5 { a(): void
     b(): int
     c(): string })XXX",
            "cycle_05.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
    }
}
}
// Re-creates the context for the decl_class_props source three times in a row; parsing must
// stay null-safe across context lifecycles and dumps must not interfere.
TEST_F(NullSafetyMemoryTest, ContextCycles_06_decl_class_props)
{
    std::string first;
    {CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC6 { p: int
     q: string })XXX",
             "cycle_06.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
    ASSERT_NE(root, nullptr);
    first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
    ASSERT_FALSE(first.empty());
}
}
{CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC6 { p: int
     q: string })XXX",
         "cycle_06.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
ASSERT_NE(root, nullptr);
EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
}
}
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC6 { p: int
     q: string })XXX",
            "cycle_06.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
    }
}
}
// Re-creates the context for the decl_class_overloads source three times in a row; parsing must
// stay null-safe across context lifecycles and dumps must not interfere.
TEST_F(NullSafetyMemoryTest, ContextCycles_07_decl_class_overloads)
{
    std::string first;
    {CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC7 { m(a: int): void
     m(a: string): void })XXX",
             "cycle_07.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
    ASSERT_NE(root, nullptr);
    first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
    ASSERT_FALSE(first.empty());
}
}
{CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC7 { m(a: int): void
     m(a: string): void })XXX",
         "cycle_07.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
ASSERT_NE(root, nullptr);
EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
}
}
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class DC7 { m(a: int): void
     m(a: string): void })XXX",
            "cycle_07.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
    }
}
}
// Re-creates the context for the decl_func source three times in a row; parsing must
// stay null-safe across context lifecycles and dumps must not interfere.
TEST_F(NullSafetyMemoryTest, ContextCycles_08_decl_func)
{
    std::string first;
    {CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare function df(a: int): string)XXX",
             "cycle_08.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
    ASSERT_NE(root, nullptr);
    first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
    ASSERT_FALSE(first.empty());
}
}
{CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare function df(a: int): string)XXX",
         "cycle_08.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
ASSERT_NE(root, nullptr);
EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
}
}
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare function df(a: int): string)XXX", "cycle_08.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
    }
}
}
// Re-creates the context for the decl_interface source three times in a row; parsing must
// stay null-safe across context lifecycles and dumps must not interfere.
TEST_F(NullSafetyMemoryTest, ContextCycles_09_decl_interface)
{
    std::string first;
    {CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare interface DI0 { m(): void })XXX",
             "cycle_09.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
    ASSERT_NE(root, nullptr);
    first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
    ASSERT_FALSE(first.empty());
}
}
{CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare interface DI0 { m(): void })XXX",
         "cycle_09.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
ASSERT_NE(root, nullptr);
EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
}
}
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare interface DI0 { m(): void })XXX", "cycle_09.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
    }
}
}
// Re-creates the context for the interface_basic source three times in a row; parsing must
// stay null-safe across context lifecycles and dumps must not interfere.
TEST_F(NullSafetyMemoryTest, ContextCycles_10_interface_basic)
{
    std::string first;
    {CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IB0 { m(): void })XXX",
             "cycle_10.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
    ASSERT_NE(root, nullptr);
    first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
    ASSERT_FALSE(first.empty());
}
}
{CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IB0 { m(): void })XXX",
         "cycle_10.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
ASSERT_NE(root, nullptr);
EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
}
}
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IB0 { m(): void })XXX", "cycle_10.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
    }
}
}
// Re-creates the context for the interface_props source three times in a row; parsing must
// stay null-safe across context lifecycles and dumps must not interfere.
TEST_F(NullSafetyMemoryTest, ContextCycles_11_interface_props)
{
    std::string first;
    {CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IP0 { p: int
     m(): void })XXX",
             "cycle_11.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
    ASSERT_NE(root, nullptr);
    first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
    ASSERT_FALSE(first.empty());
}
}
{CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IP0 { p: int
     m(): void })XXX",
         "cycle_11.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
ASSERT_NE(root, nullptr);
EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
}
}
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IP0 { p: int
     m(): void })XXX",
            "cycle_11.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
    }
}
}
// Re-creates the context for the interface_getset source three times in a row; parsing must
// stay null-safe across context lifecycles and dumps must not interfere.
TEST_F(NullSafetyMemoryTest, ContextCycles_12_interface_getset)
{
    std::string first;
    {CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IG0 { get v(): int
     set v(n: int) })XXX",
             "cycle_12.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
    ASSERT_NE(root, nullptr);
    first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
    ASSERT_FALSE(first.empty());
}
}
{CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IG0 { get v(): int
     set v(n: int) })XXX",
         "cycle_12.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
ASSERT_NE(root, nullptr);
EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
}
}
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface IG0 { get v(): int
     set v(n: int) })XXX",
            "cycle_12.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
    }
}
}
// Re-creates the context for the class_methods source three times in a row; parsing must
// stay null-safe across context lifecycles and dumps must not interfere.
TEST_F(NullSafetyMemoryTest, ContextCycles_13_class_methods)
{
    std::string first;
    {CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CM0 { m(): void {} n(): int { return 1; } })XXX",
             "cycle_13.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
    ASSERT_NE(root, nullptr);
    first = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
    ASSERT_FALSE(first.empty());
}
}
{CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CM0 { m(): void {} n(): int { return 1; } })XXX",
         "cycle_13.ats") {auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
ASSERT_NE(root, nullptr);
EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
}
}
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class CM0 { m(): void {} n(): int { return 1; } })XXX", "cycle_13.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        ASSERT_NE(root, nullptr);
        EXPECT_STREQ(GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root), first.c_str());
    }
}
}

// ---------------------------------------------------------------------------
// Section 12: strict dump result-storage leak bounds
// Simple shapes: repeated dumps must stay within the proven reads*16 EHeap budget.
// ---------------------------------------------------------------------------

// Repeated dumps of the simple empty_class AST must not grow the EHeap arena beyond the
// small per-read helper budget (memory-leak check for cached dump results).
TEST_F(NullSafetyMemoryTest, LeakDumpStable_00_empty_class)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class SD0 {})XXX", "dumpstable_00.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        EXPECT_LE(ark::es2panda::EHeap::AllocatedSize() - before, dumpReads * 16U);
    }
}
// Repeated dumps of the simple one_method AST must not grow the EHeap arena beyond the
// small per-read helper budget (memory-leak check for cached dump results).
TEST_F(NullSafetyMemoryTest, LeakDumpStable_01_one_method)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class SD1 { m(): void {} })XXX", "dumpstable_01.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t dumpReads = 400U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        EXPECT_LE(ark::es2panda::EHeap::AllocatedSize() - before, dumpReads * 16U);
    }
}
// Repeated dumps of the simple decl_class AST must not grow the EHeap arena beyond the
// small per-read helper budget (memory-leak check for cached dump results).
TEST_F(NullSafetyMemoryTest, LeakDumpStable_02_decl_class)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class SD2 { m(): void })XXX", "dumpstable_02.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        EXPECT_LE(ark::es2panda::EHeap::AllocatedSize() - before, dumpReads * 16U);
    }
}
// Repeated dumps of the simple decl_func AST must not grow the EHeap arena beyond the
// small per-read helper budget (memory-leak check for cached dump results).
TEST_F(NullSafetyMemoryTest, LeakDumpStable_03_decl_func)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare function sdf(a: int): string)XXX", "dumpstable_03.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t dumpReads = 400U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        EXPECT_LE(ark::es2panda::EHeap::AllocatedSize() - before, dumpReads * 16U);
    }
}
// Repeated dumps of the simple decl_interface AST must not grow the EHeap arena beyond the
// small per-read helper budget (memory-leak check for cached dump results).
TEST_F(NullSafetyMemoryTest, LeakDumpStable_04_decl_interface)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare interface SDI0 { m(): void })XXX", "dumpstable_04.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        EXPECT_LE(ark::es2panda::EHeap::AllocatedSize() - before, dumpReads * 16U);
    }
}
// Repeated dumps of the simple interface AST must not grow the EHeap arena beyond the
// small per-read helper budget (memory-leak check for cached dump results).
TEST_F(NullSafetyMemoryTest, LeakDumpStable_05_interface)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface SIB0 { m(): void })XXX", "dumpstable_05.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t dumpReads = 400U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        EXPECT_LE(ark::es2panda::EHeap::AllocatedSize() - before, dumpReads * 16U);
    }
}
// Repeated dumps of the simple plain_function AST must not grow the EHeap arena beyond the
// small per-read helper budget (memory-leak check for cached dump results).
TEST_F(NullSafetyMemoryTest, LeakDumpStable_06_plain_function)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function sfb(a: int): int { return a + 1; })XXX", "dumpstable_06.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t dumpReads = 200U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        EXPECT_LE(ark::es2panda::EHeap::AllocatedSize() - before, dumpReads * 16U);
    }
}
// Repeated dumps of the simple generic_class AST must not grow the EHeap arena beyond the
// small per-read helper budget (memory-leak check for cached dump results).
TEST_F(NullSafetyMemoryTest, LeakDumpStable_07_generic_class)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class SCG0<T> { v: T })XXX", "dumpstable_07.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t dumpReads = 400U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        EXPECT_LE(ark::es2panda::EHeap::AllocatedSize() - before, dumpReads * 16U);
    }
}
// Repeated dumps of the simple empty_class_long AST must not grow the EHeap arena beyond the
// small per-read helper budget (memory-leak check for cached dump results).
TEST_F(NullSafetyMemoryTest, LeakDumpStable_08_empty_class_long)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class SD0 {})XXX", "dumpstable_08.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t dumpReads = 1000U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        EXPECT_LE(ark::es2panda::EHeap::AllocatedSize() - before, dumpReads * 16U);
    }
}
// Repeated dumps of the simple one_method_long AST must not grow the EHeap arena beyond the
// small per-read helper budget (memory-leak check for cached dump results).
TEST_F(NullSafetyMemoryTest, LeakDumpStable_09_one_method_long)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class SD1 { m(): void {} })XXX", "dumpstable_09.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t dumpReads = 1000U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        EXPECT_LE(ark::es2panda::EHeap::AllocatedSize() - before, dumpReads * 16U);
    }
}
// Repeated dumps of the simple decl_class_long AST must not grow the EHeap arena beyond the
// small per-read helper budget (memory-leak check for cached dump results).
TEST_F(NullSafetyMemoryTest, LeakDumpStable_10_decl_class_long)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare class SD2 { m(): void })XXX", "dumpstable_10.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t dumpReads = 1000U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        EXPECT_LE(ark::es2panda::EHeap::AllocatedSize() - before, dumpReads * 16U);
    }
}
// Repeated dumps of the simple decl_func_long AST must not grow the EHeap arena beyond the
// small per-read helper budget (memory-leak check for cached dump results).
TEST_F(NullSafetyMemoryTest, LeakDumpStable_11_decl_func_long)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare function sdf(a: int): string)XXX", "dumpstable_11.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t dumpReads = 1000U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        EXPECT_LE(ark::es2panda::EHeap::AllocatedSize() - before, dumpReads * 16U);
    }
}
// Repeated dumps of the simple decl_interface_long AST must not grow the EHeap arena beyond the
// small per-read helper budget (memory-leak check for cached dump results).
TEST_F(NullSafetyMemoryTest, LeakDumpStable_12_decl_interface_long)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    declare interface SDI0 { m(): void })XXX", "dumpstable_12.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t dumpReads = 1000U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        EXPECT_LE(ark::es2panda::EHeap::AllocatedSize() - before, dumpReads * 16U);
    }
}
// Repeated dumps of the simple interface_long AST must not grow the EHeap arena beyond the
// small per-read helper budget (memory-leak check for cached dump results).
TEST_F(NullSafetyMemoryTest, LeakDumpStable_13_interface_long)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    interface SIB0 { m(): void })XXX", "dumpstable_13.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t dumpReads = 1000U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        EXPECT_LE(ark::es2panda::EHeap::AllocatedSize() - before, dumpReads * 16U);
    }
}
// Repeated dumps of the simple plain_function_long AST must not grow the EHeap arena beyond the
// small per-read helper budget (memory-leak check for cached dump results).
TEST_F(NullSafetyMemoryTest, LeakDumpStable_14_plain_function_long)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    function sfb(a: int): int { return a + 1; })XXX", "dumpstable_14.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t dumpReads = 1000U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        EXPECT_LE(ark::es2panda::EHeap::AllocatedSize() - before, dumpReads * 16U);
    }
}
// Repeated dumps of the simple generic_class_long AST must not grow the EHeap arena beyond the
// small per-read helper budget (memory-leak check for cached dump results).
TEST_F(NullSafetyMemoryTest, LeakDumpStable_15_generic_class_long)
{
    CONTEXT(ES2PANDA_STATE_PARSED, R"XXX(    class SCG0<T> { v: T })XXX", "dumpstable_15.ats")
    {
        auto *root = reinterpret_cast<es2panda_AstNode *>(GetAst());
        auto *oldDump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
        const std::string original(oldDump);
        const auto before = ark::es2panda::EHeap::AllocatedSize();
        const size_t dumpReads = 1000U;
        for (size_t i = 0; i < dumpReads; ++i) {
            auto *dump = GetImpl()->AstNodeDumpEtsSrcConst(GetContext(), root);
            EXPECT_STREQ(dump, original.c_str());
            EXPECT_EQ(dump, oldDump);
        }
        EXPECT_LE(ark::es2panda::EHeap::AllocatedSize() - before, dumpReads * 16U);
    }
}
}  // namespace
