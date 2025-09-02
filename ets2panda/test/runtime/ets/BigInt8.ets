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

function test_multiply(): void {
    arktest.assertEQ(new BigInt(10).operatorMultiply(new BigInt(10)), 100n)
    arktest.assertEQ(new BigInt(0).operatorMultiply(new BigInt(50)), 0n)
    arktest.assertEQ(new BigInt(1).operatorMultiply(new BigInt(50)), 50n)
    arktest.assertEQ(new BigInt(50).operatorMultiply(new BigInt(5)), 250n)
    arktest.assertEQ(new BigInt(50).operatorMultiply(new BigInt(-5)), -250n)
    arktest.assertEQ(new BigInt(-1).operatorMultiply(new BigInt(-5)), 5n)
    arktest.assertEQ(new BigInt(0).operatorMultiply(new BigInt(0)), 0n)
    arktest.assertEQ(new BigInt(123).operatorMultiply(new BigInt(1)), 123n)
    arktest.assertEQ(new BigInt(1234).operatorMultiply(new BigInt(987)), 1217958n)
    arktest.assertEQ(new BigInt(3241847031247230147213740214703214721047312n).operatorMultiply(new BigInt(412343124123421347812304712431421204731024n)), 1336753332794721625246945391107220242430725631478413717131017736872102322242538207488n)
    arktest.assertEQ(new BigInt(-3241847031247230147213740214703214721047312n).operatorMultiply(new BigInt(-412343124123421347812304712431421204731024n)), 1336753332794721625246945391107220242430725631478413717131017736872102322242538207488n)
    arktest.assertEQ(new BigInt(-3241847031247230147213740214703214721047312n).operatorMultiply(new BigInt(412343124123421347812304712431421204731024n)), -1336753332794721625246945391107220242430725631478413717131017736872102322242538207488n)
    arktest.assertEQ(new BigInt(3241847031247230147213740214703214721047312n).operatorMultiply(new BigInt(-412343124123421347812304712431421204731024n)), -1336753332794721625246945391107220242430725631478413717131017736872102322242538207488n)
    arktest.assertEQ(new BigInt(256).operatorMultiply(new BigInt(256)), 65536n)
}

function test_multiplication(): void {
    const a = 23443495146314363289895841n
    const b = 245000234343499329134n
    const c = -245000234343499329134n

    /* Multiplication testing (*) */
    arktest.assertEQ(978667632325344545n * 4534000101n, 4437279143808543031889799045n)
    arktest.assertEQ(a * b, 5743661804677708098900659843374372544236731694n)
    arktest.assertEQ(a * c, -5743661804677708098900659843374372544236731694n)
    arktest.assertEQ(a * 0n, 0n)
    arktest.assertEQ(c * 0n, 0n)

    /* Division testing (/) */
    arktest.assertEQ(39735235034886462n / 89221422n, 445355321n)
    arktest.assertEQ(a / b, 95687n)
    arktest.assertEQ(a / c, -95687n)
    arktest.assertEQ(0n / a, 0n)

    let err = false;
    try {
        a / 0n
    } catch (e) {
        if (e instanceof Error) {
            err = true
        }
    }
    arktest.assertTrue(err)

    /* Remainder of the division (%) */
    arktest.assertEQ(493433405047004109n % 111114444n, 18100749n)
    arktest.assertEQ(a % b, a % c)
    arktest.assertEQ(0n % a, 0n)

    err = false;
    try {
        a % 0n
    } catch (e) {
        if (e instanceof Error) {
            err = true
        }
    }
    arktest.assertTrue(err)
}

function main() {
    test_multiply()
    test_multiplication()
}

