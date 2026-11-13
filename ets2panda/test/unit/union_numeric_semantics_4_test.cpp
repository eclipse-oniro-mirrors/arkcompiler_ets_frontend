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

TEST_F(UnionNumericSemanticsTest, CastFloatToByte)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByte)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShort)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShort)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteShort)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteShortReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteShortUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteShortObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteShort)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteShortReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteShortUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteShortObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToLongDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToLongDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToLongDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToLongDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToLongDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToLongDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToLongDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToLongDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToFloatDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToFloatDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToFloatDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToFloatDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToFloatDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToFloatDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToFloatDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToFloatDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteShortInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteShortIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteShortIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteShortIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteShortInt)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteShortIntReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteShortIntUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteShortIntObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortIntLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortIntLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteShortLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteShortLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteShortLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteShortLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteShortLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteShortLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteShortLongUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteShortLongObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteDoubleFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteDoubleFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteDoubleFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteDoubleFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortDoubleFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortDoubleFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortDoubleFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortDoubleFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntLongDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntLongDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntLongDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntLongDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntLongDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntLongDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntLongDoubleUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntLongDoubleObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntLongFloatUndef)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSUndefinedType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntLongFloatObject)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GetGlobalTypesHolder()->GlobalETSObjectType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteShortIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteShortIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteShortIntLongUndef)
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
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteShortIntLongObject)
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
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteShortIntLong)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteShortIntLongReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteShortIntLongUndef)
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
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteShortIntLongObject)
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
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteLongDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteLongDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteLongDoubleFloatUndef)
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
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToByteLongDoubleFloatObject)
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
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteLongDoubleFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalByteBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteLongDoubleFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalByteBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteLongDoubleFloatUndef)
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
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToByteLongDoubleFloatObject)
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
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortIntLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortIntLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortIntLongFloatUndef)
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
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToShortIntLongFloatObject)
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
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortIntLongFloat)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalShortBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortIntLongFloatReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
        checker->GlobalShortBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortIntLongFloatUndef)
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
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_FALSE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToShortIntLongFloatObject)
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
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntLongFloatDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntLongFloatDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntLongFloatDoubleUndef)
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
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastFloatToIntLongFloatDoubleObject)
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
    auto *const source = checker->GlobalFloatBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntLongFloatDouble)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalIntBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalDoubleBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntLongFloatDoubleReversed)
{
    InitializeChecker("_.ats", "");
    auto *checker = Checker();
    auto *const unionType = checker->CreateETSUnionType(std::vector<checker::Type *> {
        checker->GlobalDoubleBuiltinType(),
        checker->GlobalFloatBuiltinType(),
        checker->GlobalLongBuiltinType(),
        checker->GlobalIntBuiltinType(),
    });
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntLongFloatDoubleUndef)
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
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

TEST_F(UnionNumericSemanticsTest, CastDoubleToIntLongFloatDoubleObject)
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
    auto *const source = checker->GlobalDoubleBuiltinType();
    EXPECT_TRUE(checker->Relation()->IsCastableTo(source, unionType));
}

}  // namespace ark::es2panda::gtests
