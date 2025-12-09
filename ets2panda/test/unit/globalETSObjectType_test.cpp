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
#include <gtest/gtest.h>

#include "checker/ETSAnalyzer.h"
#include "checker/ETSchecker.h"
#include "compiler/core/compilerImpl.h"
#include "compiler/core/ETSCompiler.h"
#include "compiler/core/ETSemitter.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/regSpiller.h"
#include "compiler/lowering/phase.h"
#include "es2panda.h"
#include "libarkbase/mem/arena_allocator.h"
#include "libarkbase/mem/pool_manager.h"
#include "public/public.h"
#include "util/arktsconfig.h"
#include "util/generateBin.h"
#include "varbinder/ETSBinder.h"
#include "test/utils/panda_executable_path_getter.h"
#include "checker/types/globalTypesHolder.h"
#include "test/utils/checker_test.h"

using GlobalETSObjectTypeTest = test::utils::CheckerTest;

namespace ark::es2panda {
TEST_F(GlobalETSObjectTypeTest, TypeDeclNodeTest)
{
    // The DeclNode of GlobalETSObjectType will be modified accidentally
    // when we don't use explicit type declaration in for-statments (issue20054)
    // Test case added to check the DeclNode of GlobalETSObjectType
    std::stringstream src;
    src << "class A {\n"
        << "    testObj: Object = {};\n"
        << "    testBool: boolean = false;\n"
        << "    prop: int = 1;\n}\n"
        << "function main() {"
        << "     let arr = new A[10]\n;"
        << "     for (let a of arr) {\n"
        << "         if (a.testBool) {\n"
        << "}\n}\n}" << std::endl;

    InitializeChecker("_.ets", src.str());
    auto *checker = Checker();
    auto *globalETSObjectType = checker->GlobalETSObjectType();
    [[maybe_unused]] auto *declNode = globalETSObjectType->Variable()->Declaration()->Node();
    ASSERT(declNode->IsClassDefinition());
}

TEST_F(GlobalETSObjectTypeTest, ObjectPartialGenTest)
{
    std::stringstream src;
    src << "class A<T> {\n"
        << "    constructor(obj: Object) {this.testObj = obj;}"
        << "    testObj: Object;\n}\n"
        << "let a = new A<int>({})\n;" << std::endl;

    InitializeChecker("_.ets", src.str());
    auto checker = Checker();
    ASSERT(checker);
    auto *globalETSObjectType = checker->GlobalETSObjectType();
    [[maybe_unused]] auto *declNode = globalETSObjectType->Variable()->Declaration()->Node();
    ASSERT(declNode->IsClassDefinition());
}

TEST_F(GlobalETSObjectTypeTest, ETSArrayContainGlobalETSObject)
{
    // The ETSArray which contains GlobalETSObject, its type is cached in the checker's ArrayTypes map
    // We should ensure that it is not polluted by certain modifiers, such as "TypeFlag::READONLY"
    std::stringstream src;
    src << "class A<K> {\n"
        << "    private readonly prop: number;\n"
        << "    constructor() {\n"
        << "        this.prop = 0.0;}\n"
        << "    constructor(a: FixedArray<K>) {\n"
        << "        this.prop = 1.0;}\n}"
        << "class B<K> {\n"
        << "    private readonly prop: number;\n"
        << "    constructor() {\n"
        << "        this.prop = 0.0;}\n"
        << "    constructor(a: readonly FixedArray<K>) {\n"
        << "        this.prop = 1.0;}\n}"
        << "let a = new A<Object>();\n let b = new B<Object>();";

    InitializeChecker("_.ets", src.str());
    auto checker = Checker();
    ASSERT(checker);
    auto *globalETSObjectType = checker->GlobalETSObjectType();
    [[maybe_unused]] auto arrayType = checker->CreateETSArrayType(globalETSObjectType, false);
    [[maybe_unused]] auto readonlyArrayType = checker->CreateETSArrayType(globalETSObjectType, true);
    ASSERT(!arrayType->HasTypeFlag(checker::TypeFlag::READONLY));
    ASSERT(readonlyArrayType->HasTypeFlag(checker::TypeFlag::READONLY));
}
}  // namespace ark::es2panda
