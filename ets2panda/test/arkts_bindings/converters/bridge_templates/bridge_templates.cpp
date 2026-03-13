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

#include "converters-ani.h"
#include "test_helper.h"

#include "ani.h"

EtsInt impl_Bridge_0()
{
    return 0;
}
ETS_INTEROP_0(Bridge_0, EtsInt)

EtsInt impl_Bridge_1(EtsInt p1)
{
    ASSERT_EQ_DEFAULT(p1, 1, 0);
    return p1;
}
ETS_INTEROP_1(Bridge_1, EtsInt, EtsInt)

EtsInt impl_Bridge_2(EtsInt p1, EtsInt p2)
{
    ASSERT_EQ_DEFAULT(p1, 1, 0);
    ASSERT_EQ_DEFAULT(p2, 2, 0);
    return p1 + p2;
}
ETS_INTEROP_2(Bridge_2, EtsInt, EtsInt, EtsInt)

EtsInt impl_Bridge_3(EtsInt p1, EtsInt p2, EtsInt p3)
{
    ASSERT_EQ_DEFAULT(p1, 1, 0);
    ASSERT_EQ_DEFAULT(p2, 2, 0);
    ASSERT_EQ_DEFAULT(p3, 3, 0);
    return p1 + p2 + p3;
}
ETS_INTEROP_3(Bridge_3, EtsInt, EtsInt, EtsInt, EtsInt)

EtsInt impl_Bridge_4(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4)
{
    ASSERT_EQ_DEFAULT(p1, 1, 0);
    ASSERT_EQ_DEFAULT(p2, 2, 0);
    ASSERT_EQ_DEFAULT(p3, 3, 0);
    ASSERT_EQ_DEFAULT(p4, 4, 0);
    return p1 + p2 + p3 + p4;
}
ETS_INTEROP_4(Bridge_4, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt)

EtsInt impl_Bridge_5(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5)
{
    ASSERT_EQ_DEFAULT(p1, 1, 0);
    ASSERT_EQ_DEFAULT(p2, 2, 0);
    ASSERT_EQ_DEFAULT(p3, 3, 0);
    ASSERT_EQ_DEFAULT(p4, 4, 0);
    ASSERT_EQ_DEFAULT(p5, 5, 0);
    return p1 + p2 + p3 + p4 + p5;
}
ETS_INTEROP_5(Bridge_5, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt)

EtsInt impl_Bridge_6(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6)
{
    ASSERT_EQ_DEFAULT(p1, 1, 0);
    ASSERT_EQ_DEFAULT(p2, 2, 0);
    ASSERT_EQ_DEFAULT(p3, 3, 0);
    ASSERT_EQ_DEFAULT(p4, 4, 0);
    ASSERT_EQ_DEFAULT(p5, 5, 0);
    ASSERT_EQ_DEFAULT(p6, 6, 0);
    return p1 + p2 + p3 + p4 + p5 + p6;
}
ETS_INTEROP_6(Bridge_6, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt)

EtsInt impl_Bridge_7(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7)
{
    ASSERT_EQ_DEFAULT(p1, 1, 0);
    ASSERT_EQ_DEFAULT(p2, 2, 0);
    ASSERT_EQ_DEFAULT(p3, 3, 0);
    ASSERT_EQ_DEFAULT(p4, 4, 0);
    ASSERT_EQ_DEFAULT(p5, 5, 0);
    ASSERT_EQ_DEFAULT(p6, 6, 0);
    ASSERT_EQ_DEFAULT(p7, 7, 0);
    return p1 + p2 + p3 + p4 + p5 + p6 + p7;
}
ETS_INTEROP_7(Bridge_7, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt)

EtsInt impl_Bridge_8(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7, EtsInt p8)
{
    ASSERT_EQ_DEFAULT(p1, 1, 0);
    ASSERT_EQ_DEFAULT(p2, 2, 0);
    ASSERT_EQ_DEFAULT(p3, 3, 0);
    ASSERT_EQ_DEFAULT(p4, 4, 0);
    ASSERT_EQ_DEFAULT(p5, 5, 0);
    ASSERT_EQ_DEFAULT(p6, 6, 0);
    ASSERT_EQ_DEFAULT(p7, 7, 0);
    ASSERT_EQ_DEFAULT(p8, 8, 0);
    return p1 + p2 + p3 + p4 + p5 + p6 + p7 + p8;
}
ETS_INTEROP_8(Bridge_8, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt)

EtsInt impl_Bridge_9(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7, EtsInt p8, EtsInt p9)
{
    ASSERT_EQ_DEFAULT(p1, 1, 0);
    ASSERT_EQ_DEFAULT(p2, 2, 0);
    ASSERT_EQ_DEFAULT(p3, 3, 0);
    ASSERT_EQ_DEFAULT(p4, 4, 0);
    ASSERT_EQ_DEFAULT(p5, 5, 0);
    ASSERT_EQ_DEFAULT(p6, 6, 0);
    ASSERT_EQ_DEFAULT(p7, 7, 0);
    ASSERT_EQ_DEFAULT(p8, 8, 0);
    ASSERT_EQ_DEFAULT(p9, 9, 0);
    return p1 + p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;
}
ETS_INTEROP_9(Bridge_9, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt)

EtsInt impl_Bridge_10(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7, EtsInt p8, EtsInt p9,
                      EtsInt p10)
{
    ASSERT_EQ_DEFAULT(p1, 1, 0);
    ASSERT_EQ_DEFAULT(p2, 2, 0);
    ASSERT_EQ_DEFAULT(p3, 3, 0);
    ASSERT_EQ_DEFAULT(p4, 4, 0);
    ASSERT_EQ_DEFAULT(p5, 5, 0);
    ASSERT_EQ_DEFAULT(p6, 6, 0);
    ASSERT_EQ_DEFAULT(p7, 7, 0);
    ASSERT_EQ_DEFAULT(p8, 8, 0);
    ASSERT_EQ_DEFAULT(p9, 9, 0);
    ASSERT_EQ_DEFAULT(p10, 10, 0);
    return p1 + p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9 + p10;
}
ETS_INTEROP_10(Bridge_10, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt)

EtsInt impl_Bridge_11(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7, EtsInt p8, EtsInt p9,
                      EtsInt p10, EtsInt p11)
{
    ASSERT_EQ_DEFAULT(p1, 1, 0);
    ASSERT_EQ_DEFAULT(p2, 2, 0);
    ASSERT_EQ_DEFAULT(p3, 3, 0);
    ASSERT_EQ_DEFAULT(p4, 4, 0);
    ASSERT_EQ_DEFAULT(p5, 5, 0);
    ASSERT_EQ_DEFAULT(p6, 6, 0);
    ASSERT_EQ_DEFAULT(p7, 7, 0);
    ASSERT_EQ_DEFAULT(p8, 8, 0);
    ASSERT_EQ_DEFAULT(p9, 9, 0);
    ASSERT_EQ_DEFAULT(p10, 10, 0);
    ASSERT_EQ_DEFAULT(p11, 11, 0);
    return p1 + p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9 + p10 + p11;
}
ETS_INTEROP_11(Bridge_11, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt,
               EtsInt)

EtsInt impl_Bridge_12(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7, EtsInt p8, EtsInt p9,
                      EtsInt p10, EtsInt p11, EtsInt p12)
{
    ASSERT_EQ_DEFAULT(p1, 1, 0);
    ASSERT_EQ_DEFAULT(p2, 2, 0);
    ASSERT_EQ_DEFAULT(p3, 3, 0);
    ASSERT_EQ_DEFAULT(p4, 4, 0);
    ASSERT_EQ_DEFAULT(p5, 5, 0);
    ASSERT_EQ_DEFAULT(p6, 6, 0);
    ASSERT_EQ_DEFAULT(p7, 7, 0);
    ASSERT_EQ_DEFAULT(p8, 8, 0);
    ASSERT_EQ_DEFAULT(p9, 9, 0);
    ASSERT_EQ_DEFAULT(p10, 10, 0);
    ASSERT_EQ_DEFAULT(p11, 11, 0);
    ASSERT_EQ_DEFAULT(p12, 12, 0);
    return p1 + p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9 + p10 + p11 + p12;
}
ETS_INTEROP_12(Bridge_12, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt,
               EtsInt, EtsInt)

EtsInt impl_Bridge_13(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7, EtsInt p8, EtsInt p9,
                      EtsInt p10, EtsInt p11, EtsInt p12, EtsInt p13)
{
    ASSERT_EQ_DEFAULT(p1, 1, 0);
    ASSERT_EQ_DEFAULT(p2, 2, 0);
    ASSERT_EQ_DEFAULT(p3, 3, 0);
    ASSERT_EQ_DEFAULT(p4, 4, 0);
    ASSERT_EQ_DEFAULT(p5, 5, 0);
    ASSERT_EQ_DEFAULT(p6, 6, 0);
    ASSERT_EQ_DEFAULT(p7, 7, 0);
    ASSERT_EQ_DEFAULT(p8, 8, 0);
    ASSERT_EQ_DEFAULT(p9, 9, 0);
    ASSERT_EQ_DEFAULT(p10, 10, 0);
    ASSERT_EQ_DEFAULT(p11, 11, 0);
    ASSERT_EQ_DEFAULT(p12, 12, 0);
    ASSERT_EQ_DEFAULT(p13, 13, 0);
    return p1 + p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9 + p10 + p11 + p12 + p13;
}
ETS_INTEROP_13(Bridge_13, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt,
               EtsInt, EtsInt, EtsInt)

EtsInt impl_Bridge_14(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7, EtsInt p8, EtsInt p9,
                      EtsInt p10, EtsInt p11, EtsInt p12, EtsInt p13, EtsInt p14)
{
    ASSERT_EQ_DEFAULT(p1, 1, 0);
    ASSERT_EQ_DEFAULT(p2, 2, 0);
    ASSERT_EQ_DEFAULT(p3, 3, 0);
    ASSERT_EQ_DEFAULT(p4, 4, 0);
    ASSERT_EQ_DEFAULT(p5, 5, 0);
    ASSERT_EQ_DEFAULT(p6, 6, 0);
    ASSERT_EQ_DEFAULT(p7, 7, 0);
    ASSERT_EQ_DEFAULT(p8, 8, 0);
    ASSERT_EQ_DEFAULT(p9, 9, 0);
    ASSERT_EQ_DEFAULT(p10, 10, 0);
    ASSERT_EQ_DEFAULT(p11, 11, 0);
    ASSERT_EQ_DEFAULT(p12, 12, 0);
    ASSERT_EQ_DEFAULT(p13, 13, 0);
    ASSERT_EQ_DEFAULT(p14, 14, 0);
    return p1 + p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9 + p10 + p11 + p12 + p13 + p14;
}
ETS_INTEROP_14(Bridge_14, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt,
               EtsInt, EtsInt, EtsInt, EtsInt)

EtsInt impl_Bridge_15(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7, EtsInt p8, EtsInt p9,
                      EtsInt p10, EtsInt p11, EtsInt p12, EtsInt p13, EtsInt p14, EtsInt p15)
{
    ASSERT_EQ_DEFAULT(p1, 1, 0);
    ASSERT_EQ_DEFAULT(p2, 2, 0);
    ASSERT_EQ_DEFAULT(p3, 3, 0);
    ASSERT_EQ_DEFAULT(p4, 4, 0);
    ASSERT_EQ_DEFAULT(p5, 5, 0);
    ASSERT_EQ_DEFAULT(p6, 6, 0);
    ASSERT_EQ_DEFAULT(p7, 7, 0);
    ASSERT_EQ_DEFAULT(p8, 8, 0);
    ASSERT_EQ_DEFAULT(p9, 9, 0);
    ASSERT_EQ_DEFAULT(p10, 10, 0);
    ASSERT_EQ_DEFAULT(p11, 11, 0);
    ASSERT_EQ_DEFAULT(p12, 12, 0);
    ASSERT_EQ_DEFAULT(p13, 13, 0);
    ASSERT_EQ_DEFAULT(p14, 14, 0);
    ASSERT_EQ_DEFAULT(p15, 15, 0);
    return p1 + p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9 + p10 + p11 + p12 + p13 + p14 + p15;
}
ETS_INTEROP_15(Bridge_15, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt,
               EtsInt, EtsInt, EtsInt, EtsInt, EtsInt)

void impl_Bridge_V1(EtsInt p1)
{
    ASSERT_EQ(p1, 1);
}
ETS_INTEROP_V1(Bridge_V1, EtsInt)

void impl_Bridge_V2(EtsInt p1, EtsInt p2)
{
    ASSERT_EQ(p1, 1);
    ASSERT_EQ(p2, 2);
}
ETS_INTEROP_V2(Bridge_V2, EtsInt, EtsInt)

void impl_Bridge_V3(EtsInt p1, EtsInt p2, EtsInt p3)
{
    ASSERT_EQ(p1, 1);
    ASSERT_EQ(p2, 2);
    ASSERT_EQ(p3, 3);
}
ETS_INTEROP_V3(Bridge_V3, EtsInt, EtsInt, EtsInt)

void impl_Bridge_V4(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4)
{
    ASSERT_EQ(p1, 1);
    ASSERT_EQ(p2, 2);
    ASSERT_EQ(p3, 3);
    ASSERT_EQ(p4, 4);
}
ETS_INTEROP_V4(Bridge_V4, EtsInt, EtsInt, EtsInt, EtsInt)

void impl_Bridge_V5(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5)
{
    ASSERT_EQ(p1, 1);
    ASSERT_EQ(p2, 2);
    ASSERT_EQ(p3, 3);
    ASSERT_EQ(p4, 4);
    ASSERT_EQ(p5, 5);
}
ETS_INTEROP_V5(Bridge_V5, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt)

void impl_Bridge_V6(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6)
{
    ASSERT_EQ(p1, 1);
    ASSERT_EQ(p2, 2);
    ASSERT_EQ(p3, 3);
    ASSERT_EQ(p4, 4);
    ASSERT_EQ(p5, 5);
    ASSERT_EQ(p6, 6);
}
ETS_INTEROP_V6(Bridge_V6, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt)

void impl_Bridge_V7(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7)
{
    ASSERT_EQ(p1, 1);
    ASSERT_EQ(p2, 2);
    ASSERT_EQ(p3, 3);
    ASSERT_EQ(p4, 4);
    ASSERT_EQ(p5, 5);
    ASSERT_EQ(p6, 6);
    ASSERT_EQ(p7, 7);
}
ETS_INTEROP_V7(Bridge_V7, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt)

// CC-OFFNXT(WordsTool.190) sensitive word conflict
void impl_Bridge_V8(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7, EtsInt p8)
{
    ASSERT_EQ(p1, 1);
    ASSERT_EQ(p2, 2);
    ASSERT_EQ(p3, 3);
    ASSERT_EQ(p4, 4);
    ASSERT_EQ(p5, 5);
    ASSERT_EQ(p6, 6);
    ASSERT_EQ(p7, 7);
    ASSERT_EQ(p8, 8);
}
// CC-OFFNXT(WordsTool.190) sensitive word conflict
ETS_INTEROP_V8(Bridge_V8, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt)

void impl_Bridge_V9(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7, EtsInt p8, EtsInt p9)
{
    ASSERT_EQ(p1, 1);
    ASSERT_EQ(p2, 2);
    ASSERT_EQ(p3, 3);
    ASSERT_EQ(p4, 4);
    ASSERT_EQ(p5, 5);
    ASSERT_EQ(p6, 6);
    ASSERT_EQ(p7, 7);
    ASSERT_EQ(p8, 8);
    ASSERT_EQ(p9, 9);
}
ETS_INTEROP_V9(Bridge_V9, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt)

void impl_Bridge_V10(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7, EtsInt p8, EtsInt p9,
                     EtsInt p10)
{
    ASSERT_EQ(p1, 1);
    ASSERT_EQ(p2, 2);
    ASSERT_EQ(p3, 3);
    ASSERT_EQ(p4, 4);
    ASSERT_EQ(p5, 5);
    ASSERT_EQ(p6, 6);
    ASSERT_EQ(p7, 7);
    ASSERT_EQ(p8, 8);
    ASSERT_EQ(p9, 9);
    ASSERT_EQ(p10, 10);
}
ETS_INTEROP_V10(Bridge_V10, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt)

void impl_Bridge_V11(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7, EtsInt p8, EtsInt p9,
                     EtsInt p10, EtsInt p11)
{
    ASSERT_EQ(p1, 1);
    ASSERT_EQ(p2, 2);
    ASSERT_EQ(p3, 3);
    ASSERT_EQ(p4, 4);
    ASSERT_EQ(p5, 5);
    ASSERT_EQ(p6, 6);
    ASSERT_EQ(p7, 7);
    ASSERT_EQ(p8, 8);
    ASSERT_EQ(p9, 9);
    ASSERT_EQ(p10, 10);
    ASSERT_EQ(p11, 11);
}
ETS_INTEROP_V11(Bridge_V11, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt)

void impl_Bridge_V12(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7, EtsInt p8, EtsInt p9,
                     EtsInt p10, EtsInt p11, EtsInt p12)
{
    ASSERT_EQ(p1, 1);
    ASSERT_EQ(p2, 2);
    ASSERT_EQ(p3, 3);
    ASSERT_EQ(p4, 4);
    ASSERT_EQ(p5, 5);
    ASSERT_EQ(p6, 6);
    ASSERT_EQ(p7, 7);
    ASSERT_EQ(p8, 8);
    ASSERT_EQ(p9, 9);
    ASSERT_EQ(p10, 10);
    ASSERT_EQ(p11, 11);
    ASSERT_EQ(p12, 12);
}
ETS_INTEROP_V12(Bridge_V12, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt,
                EtsInt)

void impl_Bridge_V13(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7, EtsInt p8, EtsInt p9,
                     EtsInt p10, EtsInt p11, EtsInt p12, EtsInt p13)
{
    ASSERT_EQ(p1, 1);
    ASSERT_EQ(p2, 2);
    ASSERT_EQ(p3, 3);
    ASSERT_EQ(p4, 4);
    ASSERT_EQ(p5, 5);
    ASSERT_EQ(p6, 6);
    ASSERT_EQ(p7, 7);
    ASSERT_EQ(p8, 8);
    ASSERT_EQ(p9, 9);
    ASSERT_EQ(p10, 10);
    ASSERT_EQ(p11, 11);
    ASSERT_EQ(p12, 12);
    ASSERT_EQ(p13, 13);
}
ETS_INTEROP_V13(Bridge_V13, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt,
                EtsInt, EtsInt)

void impl_Bridge_V14(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7, EtsInt p8, EtsInt p9,
                     EtsInt p10, EtsInt p11, EtsInt p12, EtsInt p13, EtsInt p14)
{
    ASSERT_EQ(p1, 1);
    ASSERT_EQ(p2, 2);
    ASSERT_EQ(p3, 3);
    ASSERT_EQ(p4, 4);
    ASSERT_EQ(p5, 5);
    ASSERT_EQ(p6, 6);
    ASSERT_EQ(p7, 7);
    ASSERT_EQ(p8, 8);
    ASSERT_EQ(p9, 9);
    ASSERT_EQ(p10, 10);
    ASSERT_EQ(p11, 11);
    ASSERT_EQ(p12, 12);
    ASSERT_EQ(p13, 13);
    ASSERT_EQ(p14, 14);
}
ETS_INTEROP_V14(Bridge_V14, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt,
                EtsInt, EtsInt, EtsInt)

void impl_Bridge_V15(EtsInt p1, EtsInt p2, EtsInt p3, EtsInt p4, EtsInt p5, EtsInt p6, EtsInt p7, EtsInt p8, EtsInt p9,
                     EtsInt p10, EtsInt p11, EtsInt p12, EtsInt p13, EtsInt p14, EtsInt p15)
{
    ASSERT_EQ(p1, 1);
    ASSERT_EQ(p2, 2);
    ASSERT_EQ(p3, 3);
    ASSERT_EQ(p4, 4);
    ASSERT_EQ(p5, 5);
    ASSERT_EQ(p6, 6);
    ASSERT_EQ(p7, 7);
    ASSERT_EQ(p8, 8);
    ASSERT_EQ(p9, 9);
    ASSERT_EQ(p10, 10);
    ASSERT_EQ(p11, 11);
    ASSERT_EQ(p12, 12);
    ASSERT_EQ(p13, 13);
    ASSERT_EQ(p14, 14);
    ASSERT_EQ(p15, 15);
}
ETS_INTEROP_V15(Bridge_V15, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt, EtsInt,
                EtsInt, EtsInt, EtsInt, EtsInt)
