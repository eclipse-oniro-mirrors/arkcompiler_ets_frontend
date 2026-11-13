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
#include <algorithm>
#include "checker/ETSAnalyzer.h"
#include "checker/ETSchecker.h"
#include "compiler/core/compilerImpl.h"
#include "compiler/core/ETSCompiler.h"
#include "compiler/core/ETSemitter.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/regSpiller.h"
#include "compiler/lowering/phase.h"
#include "es2panda.h"
#include "util/eheap.h"
#include "public/public.h"
#include "util/arktsconfig.h"
#include "util/generateBin.h"
#include "varbinder/ETSBinder.h"
#include "checker/types/globalTypesHolder.h"
#include "test/unit/union_numeric_semantics_test.h"

namespace ark::es2panda::gtests {

TEST_F(UnionNumericSemanticsTest, ByteToStringNoMatchCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToStringNoMatchCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToObjectNoMatchCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToStringNoMatchCast_V2)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToStringNoMatchCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToByteStringNoMatch)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToByteStringUndefNoMatch)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToShortStringNoMatch)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToStringNumberCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToStringByteNoMatch)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToShortStringNoMatch)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToUndefObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToStringObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToByteObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToShortObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToIntObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToDoubleObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToUndefObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToStringObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToByteObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToShortObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToIntObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToLongObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToDoubleObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToUndefObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToStringObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToByteObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToShortObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToIntObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToLongObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToDoubleObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToUndefObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToStringObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToByteObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToShortObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToIntObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToLongObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToDoubleObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToUndefObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToStringObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToByteObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToShortObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToIntObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToLongObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToDoubleObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToUndefObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToStringObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToByteObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToShortObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToIntObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToLongObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToDoubleObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToByteShortObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToByteShortObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToLongObjectCast_V2)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToIntLongCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToByteIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToShortIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToByteShortIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToByteIntCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToShortIntCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToByteShortIntCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToIntLongObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToIntLongCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToByteIntCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToShortIntCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToByteLongCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToShortLongCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByte)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByte)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShort)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShort)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteShort)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteShortReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteShortUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteShortObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteShort)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteShortReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteShortUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteShortObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToLongDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToLongDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToLongDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToLongDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToLongDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToLongDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToLongDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToLongDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToFloatDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToFloatDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToFloatDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToFloatDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToFloatDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToFloatDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToFloatDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToFloatDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteShortInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteShortIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteShortIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteShortIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteShortInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteShortIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteShortIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteShortIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteShortLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteShortLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteShortLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteShortLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteShortLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteShortLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteShortLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteShortLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteDoubleFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteDoubleFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteDoubleFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteDoubleFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortDoubleFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortDoubleFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortDoubleFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortDoubleFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntLongDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntLongDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntLongDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntLongDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntLongDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntLongDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntLongDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntLongDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteShortIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteShortIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteShortIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteShortIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteShortIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteShortIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteShortIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteShortIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteLongDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteLongDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteLongDoubleFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToByteLongDoubleFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteLongDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteLongDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteLongDoubleFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToByteLongDoubleFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortIntLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortIntLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortIntLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToShortIntLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortIntLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortIntLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortIntLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToShortIntLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntLongFloatDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntLongFloatDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntLongFloatDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastByteToIntLongFloatDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntLongFloatDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntLongFloatDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntLongFloatDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastShortToIntLongFloatDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

}  // namespace ark::es2panda::gtests
