/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

function test_signed_right_shift_int(): void
{
    let tests: int[][] = [
                          [0x7FFFFFFF,   0, 0x7FFFFFFF],
                          [0x7FFFFFFF,  32, 0x7FFFFFFF],
                          [0x7FFFFFFF, -32, 0x7FFFFFFF],
                          [0x7FFFFFFF,   1, 0x3FFFFFFF],
                          [0x7FFFFFFF,  33, 0x3FFFFFFF],
                          [0x7FFFFFFF,  30, 1],
                          [0x7FFFFFFF,  62, 1],
                          [0x7FFFFFFF,  31, 0],
                          [0x7FFFFFFF,  63, 0],
                          [0x7FFFFFFF,  -1, 0],
                          [-1,  0, -1],
                          [-1,  1, -1],
                          [-1, 31, -1],
                          [-4,  0, -4],
                          [-4,  1, -2],
                          [-4, 31, -1],
                         ];

    // runtime tests
    for (let i: int = 0; i<tests.length; i++)
    {
        arktest.assertEQ(tests[i][0] >> tests[i][1], tests[i][2])
    }

    // compile time tests
    arktest.assertEQ(0x7FFFFFFF >>   0, 0x7FFFFFFF)
    arktest.assertEQ(0x7FFFFFFF >>  32, 0x7FFFFFFF)
    arktest.assertEQ(0x7FFFFFFF >> -32, 0x7FFFFFFF)
    arktest.assertEQ(0x7FFFFFFF >>   1, 0x3FFFFFFF)
    arktest.assertEQ(0x7FFFFFFF >>  33, 0x3FFFFFFF)
    arktest.assertEQ(0x7FFFFFFF >>  30, 1)
    arktest.assertEQ(0x7FFFFFFF >>  62, 1)
    arktest.assertEQ(0x7FFFFFFF >>  31, 0)
    arktest.assertEQ(0x7FFFFFFF >>  63, 0)
    arktest.assertEQ(0x7FFFFFFF >>  -1, 0)
    arktest.assertEQ(-1 >>  0, -1)
    arktest.assertEQ(-1 >>  1, -1)
    arktest.assertEQ(-1 >> 31, -1)
    arktest.assertEQ(-4 >>  0, -4)
    arktest.assertEQ(-4 >>  1, -2)
    arktest.assertEQ(-4 >> 31, -1)
}

function test_signed_right_shift_long(): void
{
    let tests: long[][] = [
                          [0x7FFFFFFFFFFFFFFF,    0, 0x7FFFFFFFFFFFFFFF],
                          [0x7FFFFFFFFFFFFFFF,   64, 0x7FFFFFFFFFFFFFFF],
                          [0x7FFFFFFFFFFFFFFF,  -64, 0x7FFFFFFFFFFFFFFF],
                          [0x7FFFFFFFFFFFFFFF,    1, 0x3FFFFFFFFFFFFFFF],
                          [0x7FFFFFFFFFFFFFFF,   65, 0x3FFFFFFFFFFFFFFF],
                          [0x7FFFFFFFFFFFFFFF,   62, 1],
                          [0x7FFFFFFFFFFFFFFF,  126, 1],
                          [0x7FFFFFFFFFFFFFFF,   63, 0],
                          [0x7FFFFFFFFFFFFFFF,  127, 0],
                          [0x7FFFFFFFFFFFFFFF,   -1, 0],
                          [-1,  0, -1],
                          [-1,  1, -1],
                          [-1, 63, -1],
                          [-4,  0, -4],
                          [-4,  1, -2],
                          [-4, 63, -1],
                         ];

    // runtime tests
    for (let i: int = 0; i<tests.length; i++)
    {
        arktest.assertEQ(tests[i][0] >> tests[i][1], tests[i][2])
    }

    // compile time tests
    arktest.assertEQ(0x7FFFFFFFFFFFFFFF >>    0, 0x7FFFFFFFFFFFFFFF)
    arktest.assertEQ(0x7FFFFFFFFFFFFFFF >>   64, 0x7FFFFFFFFFFFFFFF)
    arktest.assertEQ(0x7FFFFFFFFFFFFFFF >>  -64, 0x7FFFFFFFFFFFFFFF)
    arktest.assertEQ(0x7FFFFFFFFFFFFFFF >>    1, 0x3FFFFFFFFFFFFFFF)
    arktest.assertEQ(0x7FFFFFFFFFFFFFFF >>   65, 0x3FFFFFFFFFFFFFFF)
    arktest.assertEQ(0x7FFFFFFFFFFFFFFF >>   62, 1)
    arktest.assertEQ(0x7FFFFFFFFFFFFFFF >>  126, 1)
    arktest.assertEQ(0x7FFFFFFFFFFFFFFF >>   63, 0)
    arktest.assertEQ(0x7FFFFFFFFFFFFFFF >>  127, 0)
    arktest.assertEQ(0x7FFFFFFFFFFFFFFF >>   -1, 0)
    arktest.assertEQ(-1 >>  0, -1)
    arktest.assertEQ(-1 >>  1, -1)
    arktest.assertEQ(-1 >> 63, -1)
    arktest.assertEQ(-4 >>  0, -4)
    arktest.assertEQ(-4 >>  1, -2)
    arktest.assertEQ(-4 >> 63, -1)
}


function main(): void
{
    test_signed_right_shift_int();
    test_signed_right_shift_long();
}
