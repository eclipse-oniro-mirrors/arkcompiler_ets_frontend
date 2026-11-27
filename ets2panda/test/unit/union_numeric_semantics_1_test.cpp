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

TEST_F(UnionNumericSemanticsTest, IntToDoubleUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToDoubleUndefinedReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToDoubleUndefinedObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToDoubleUndefinedObjectReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToDoubleUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToDoubleUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToDoubleUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToDoubleUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToDoubleUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToFloatUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToFloatUndefinedReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToFloatUndefinedObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToFloatUndefinedObjectReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToFloatUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToFloatUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToFloatUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToFloatUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToFloatUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongUndefinedReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongUndefinedObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongUndefinedObjectReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToLongUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToLongUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToLongUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToLongUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToLongUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToByteUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToByteUndefinedReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToByteUndefinedObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToByteUndefinedObjectReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToByteUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToByteUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToByteUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToByteUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToByteUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToShortUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToShortUndefinedReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToShortUndefinedObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToShortUndefinedObjectReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToShortUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToShortUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToShortUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToShortUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToShortUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToIntUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToIntUndefinedReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToIntUndefinedObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToIntUndefinedObjectReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToIntUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToIntUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToIntUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToIntUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToIntUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToStringUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToStringUndefinedReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToStringUndefinedObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToStringUndefinedObjectReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToStringUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToStringUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToStringUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToStringUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToStringUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalBuiltinETSStringType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToObjectUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToObjectUndefinedReversed)
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

TEST_F(UnionNumericSemanticsTest, IntToObjectUndefinedObjectCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToObjectUndefinedObjectReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToObjectUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToObjectUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToObjectUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToObjectUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToObjectUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongDoubleAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongDoubleAmbiguousReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongDoubleUndefinedAmbiguous)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongDoubleObjectAmbiguous)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToLongDoubleAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToLongDoubleAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToLongDoubleAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToLongDoubleAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToLongDoubleAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongDoubleStringAmbiguous)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToFloatDoubleAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToFloatDoubleAmbiguousReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToFloatDoubleUndefinedAmbiguous)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToFloatDoubleObjectAmbiguous)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToFloatDoubleAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToFloatDoubleAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToFloatDoubleAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToFloatDoubleAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToFloatDoubleAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToFloatDoubleStringAmbiguous)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongFloatAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongFloatAmbiguousReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongFloatUndefinedAmbiguous)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongFloatObjectAmbiguous)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToLongFloatAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToLongFloatAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToLongFloatAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToLongFloatAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToLongFloatAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongFloatStringAmbiguous)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLFDAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLFDUndefinedAmbiguous)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongSourceToDoubleUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongSourceToDoubleUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongSourceToFloatUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongSourceToFloatUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongSourceToLongUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongSourceToLongUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongSourceToByteUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongSourceToByteUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongSourceToShortUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongSourceToShortUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongSourceToIntUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongSourceToIntUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToFDAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteSourceToShortUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteSourceToShortUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteSourceToIntUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteSourceToIntUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteSourceToLongUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteSourceToLongUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteSourceToFloatUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteSourceToFloatUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteSourceToDoubleUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteSourceToDoubleUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteSourceToByteUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteSourceToByteUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToLDAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToFDAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToSIAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortSourceToIntUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortSourceToIntUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortSourceToLongUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortSourceToLongUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortSourceToFloatUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortSourceToFloatUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortSourceToDoubleUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortSourceToDoubleUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortSourceToShortUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortSourceToShortUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortSourceToByteUndefinedCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortSourceToByteUndefinedCastDetailed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToLDAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToFDAmbiguousCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToDoubleUndefinedCast_V2)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToFloatUndefinedCast_V2)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToDoubleUndefinedCast_V2)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToFloatUndefinedCast_V2)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, DoubleToDoubleCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, FloatToFloatCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToLongUniqueWidenCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToLongPairUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToLongTripleUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToLongUndefUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToLongObjectUniqueWiden)
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

TEST_F(UnionNumericSemanticsTest, ByteToLongStringPair)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToDoubleUniqueWidenCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToDoublePairUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToDoubleTripleUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToDoubleUndefUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToDoubleObjectUniqueWiden)
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

TEST_F(UnionNumericSemanticsTest, ByteToDoubleStringPair)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToFloatUniqueWidenCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToFloatPairUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToFloatTripleUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToFloatUndefUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToFloatObjectUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToFloatStringPair)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToLongUniqueWidenCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToLongPairUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToLongTripleUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToLongUndefUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToLongObjectUniqueWiden)
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

TEST_F(UnionNumericSemanticsTest, ShortToLongStringPair)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToDoubleUniqueWidenCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToDoublePairUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToDoubleTripleUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToDoubleUndefUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToDoubleObjectUniqueWiden)
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

TEST_F(UnionNumericSemanticsTest, ShortToDoubleStringPair)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToFloatUniqueWidenCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToFloatPairUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToFloatTripleUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToFloatUndefUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToFloatObjectUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToFloatStringPair)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongUniqueWidenCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongPairUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongTripleUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongUndefUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToLongObjectUniqueWiden)
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

TEST_F(UnionNumericSemanticsTest, IntToLongStringPair)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToDoubleUniqueWidenCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToDoublePairUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToDoubleTripleUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToDoubleUndefUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToDoubleObjectUniqueWiden)
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

TEST_F(UnionNumericSemanticsTest, IntToDoubleStringPair)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToFloatUniqueWidenCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToFloatPairUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToFloatTripleUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToFloatUndefUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToFloatObjectUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToFloatStringPair)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToDoubleUniqueWidenCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToDoublePairUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToDoubleTripleUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToDoubleUndefUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToDoubleObjectUniqueWiden)
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

TEST_F(UnionNumericSemanticsTest, LongToDoubleStringPair)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToFloatUniqueWidenCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToFloatPairUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToFloatTripleUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToFloatUndefUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToFloatObjectUniqueWiden)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, LongToFloatStringPair)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToByteShortCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, IntToByteShortUndefCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ByteToByteShortCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalByteBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_TRUE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShortToByteShortCast)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalShortBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
    EXPECT_FALSE(checker->Relation()->IsAssignableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, ShapeByteUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 2U);
}

TEST_F(UnionNumericSemanticsTest, ShapeShortUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 2U);
}

TEST_F(UnionNumericSemanticsTest, ShapeIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 2U);
}

TEST_F(UnionNumericSemanticsTest, ShapeLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 2U);
}

TEST_F(UnionNumericSemanticsTest, ShapeFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 2U);
}

TEST_F(UnionNumericSemanticsTest, ShapeDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 2U);
}

TEST_F(UnionNumericSemanticsTest, ShapeIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 2U);
}

TEST_F(UnionNumericSemanticsTest, ShapeByteShortUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 3U);
}

TEST_F(UnionNumericSemanticsTest, ShapeLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 2U);
}

TEST_F(UnionNumericSemanticsTest, ShapeLongDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 2U);
}

TEST_F(UnionNumericSemanticsTest, ShapeFloatDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 2U);
}

TEST_F(UnionNumericSemanticsTest, ShapeByteObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 2U);
}

TEST_F(UnionNumericSemanticsTest, ShapeIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 2U);
}

TEST_F(UnionNumericSemanticsTest, ShapeByteString)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalBuiltinETSStringType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 2U);
}

TEST_F(UnionNumericSemanticsTest, ShapeLongFloatDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 4U);
}

TEST_F(UnionNumericSemanticsTest, ShapeIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 3U);
}

TEST_F(UnionNumericSemanticsTest, ShapeByteShortInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 3U);
}

TEST_F(UnionNumericSemanticsTest, ShapeShortDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 2U);
}

TEST_F(UnionNumericSemanticsTest, ShapeByteFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    EXPECT_TRUE(unionType->IsETSUnionType());
    EXPECT_EQ(unionType->AsETSUnionType()->ConstituentTypes().size(), 2U);
}

}  // namespace ark::es2panda::gtests
