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

TEST_F(UnionNumericSemanticsTest, CastIntToByte)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShort)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteShort)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteShortReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteShortUndef)
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
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteShortObject)
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
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntLongUndef)
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
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntLongObject)
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
}

TEST_F(UnionNumericSemanticsTest, CastIntToLongDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToLongDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToLongDoubleUndef)
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
}

TEST_F(UnionNumericSemanticsTest, CastIntToLongDoubleObject)
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
}

TEST_F(UnionNumericSemanticsTest, CastIntToFloatDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToFloatDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToFloatDoubleUndef)
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
}

TEST_F(UnionNumericSemanticsTest, CastIntToFloatDoubleObject)
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
}

TEST_F(UnionNumericSemanticsTest, CastIntToLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToLongFloatUndef)
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
}

TEST_F(UnionNumericSemanticsTest, CastIntToLongFloatObject)
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
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteShortInt)
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
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteShortIntReversed)
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
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteShortIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteShortIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteShortLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteShortLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteShortLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteShortLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteDoubleFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteDoubleFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortDoubleFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortDoubleFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntLongDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntLongDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntLongDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntLongDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteShortIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteShortIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteShortIntLongUndef)
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
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteShortIntLongObject)
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
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteLongDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteLongDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteLongDoubleFloatUndef)
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
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToByteLongDoubleFloatObject)
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
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortIntLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortIntLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortIntLongFloatUndef)
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
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToShortIntLongFloatObject)
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
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntLongFloatDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntLongFloatDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntLongFloatDoubleUndef)
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
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastIntToIntLongFloatDoubleObject)
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
    auto *const source = checker->GlobalIntBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByte)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShort)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteShort)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteShortReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteShortUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteShortObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToLongDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToLongDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToLongDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToLongDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToFloatDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToFloatDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToFloatDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToFloatDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteShortInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteShortIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteShortIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteShortIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteShortLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteShortLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteShortLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteShortLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteDoubleFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteDoubleFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortDoubleFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortDoubleFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntLongDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntLongDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntLongDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntLongDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteShortIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteShortIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteShortIntLongUndef)
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
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteShortIntLongObject)
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
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteLongDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteLongDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteLongDoubleFloatUndef)
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
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToByteLongDoubleFloatObject)
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
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortIntLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortIntLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortIntLongFloatUndef)
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
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToShortIntLongFloatObject)
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
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntLongFloatDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntLongFloatDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntLongFloatDoubleUndef)
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
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastLongToIntLongFloatDoubleObject)
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
    auto *const source = checker->GlobalLongBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

}  // namespace ark::es2panda::gtests
