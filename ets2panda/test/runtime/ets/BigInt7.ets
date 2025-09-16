/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

function test_minus(): void {
    arktest.assertEQ(new BigInt(10).operatorSubtract(new BigInt(2)), (8n))
    arktest.assertEQ(new BigInt(2).operatorSubtract(new BigInt(10)), (-8n))
    arktest.assertEQ(new BigInt(-10).operatorSubtract(new BigInt(-2)), (-8n))
    arktest.assertEQ(new BigInt(-100).operatorSubtract(new BigInt(1)), (-101n))
    arktest.assertEQ(new BigInt(-100).operatorSubtract(new BigInt(-1)), (-99n))
    arktest.assertEQ(new BigInt(-1000).operatorSubtract(new BigInt(10)), (-1010n))
    arktest.assertEQ(new BigInt(1237840127434312471243n).operatorSubtract(new BigInt(234112342314526914672342143621463921469n)), (-234112342314526913434502016187151450226n))
    arktest.assertEQ(new BigInt(-1237840127434312471243n).operatorSubtract(new BigInt(234112342314526914672342143621463921469n)), (-234112342314526915910182271055776392712n))
    arktest.assertEQ(new BigInt(1237840127434312471243n).operatorSubtract(new BigInt(-234112342314526914672342143621463921469n)), (234112342314526915910182271055776392712n))
    arktest.assertEQ(new BigInt(-1237840127434312471243n).operatorSubtract(new BigInt(-234112342314526914672342143621463921469n)), (234112342314526913434502016187151450226n))
    arktest.assertEQ(new BigInt(-1000).operatorSubtract(new BigInt(-10)), (-990n))
    arktest.assertEQ(new BigInt(-100).operatorSubtract(new BigInt(0)), (-100n))
    arktest.assertEQ(new BigInt(0).operatorSubtract(new BigInt(-100)), (100n))
    arktest.assertEQ(new BigInt(0).operatorSubtract(new BigInt(100)), (-100n))
    arktest.assertEQ(new BigInt(65500).operatorSubtract(new BigInt(1)), (65499n))
    arktest.assertEQ(new BigInt(65500).operatorSubtract(new BigInt(-1)), (65501n))
    arktest.assertEQ(new BigInt(-65500).operatorSubtract(new BigInt(-1)), (-65499n))
    arktest.assertEQ(new BigInt(-65500).operatorSubtract(new BigInt(1)), (-65501n))
    arktest.assertEQ(new BigInt(65500).operatorSubtract(new BigInt(100000)), (-34500n))
    arktest.assertEQ(new BigInt(49798775).operatorSubtract(new BigInt(43621978)), (6176797n))
    arktest.assertEQ(new BigInt(10).operatorSubtract(new BigInt(20)), (-10n))
}

function test_subtraction(): void {
    const a = 18446744073709551616n;
    const b = 36893488147419103232n;
    const c = -10000000000000000000n;

    /* Subtraction testing (-) */
    arktest.assertEQ(999999999999999n - 1n, 999999999999998n)
    arktest.assertEQ(b - a, a)
    arktest.assertEQ(a - b, -18446744073709551616n)
    arktest.assertEQ(b - a - a, 0n)
}

function test_inc_dec(): void {
    const a = 340282366920938463463374607431768211456n
    const b = -2313455919991034304490n

    /* Increment testing */
    /* Decrement testing */
}


function main() {
    test_minus();
    test_subtraction()
    test_inc_dec()
}
