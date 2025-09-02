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

function test_bitwise_and(): void {
    arktest.assertEQ(new BigInt(10).operatorBitwiseAnd(new BigInt(2)), (2n))
    arktest.assertEQ(new BigInt(256).operatorBitwiseAnd(new BigInt(1)), (0n))
    arktest.assertEQ(new BigInt(3124378143267041203423n).operatorBitwiseAnd(new BigInt(43621978)), (41948250n))
    arktest.assertEQ(new BigInt(256).operatorBitwiseAnd(new BigInt(256)), (256n))
    arktest.assertEQ(new BigInt(12345678).operatorBitwiseAnd(new BigInt(1234)), (66n))
}

function test_bitwise_or(): void {
    arktest.assertEQ(new BigInt(10).operatorBitwiseOr(new BigInt(2)), (10n))
    arktest.assertEQ(new BigInt(256).operatorBitwiseOr(new BigInt(1)), (257n))
    arktest.assertEQ(new BigInt(256).operatorBitwiseOr(new BigInt(256)), (256n))
    arktest.assertEQ(new BigInt(3124378143267041203423n).operatorBitwiseOr(new BigInt(43621978)), (3124378143267042877151n))
    arktest.assertEQ(new BigInt(12345678).operatorBitwiseOr(new BigInt(1234)), (12346846n))
}

function test_bitwise_xor(): void {
    arktest.assertEQ(new BigInt(10).operatorBitwiseXor(new BigInt(2)), (8n))
    arktest.assertEQ(new BigInt(256).operatorBitwiseXor(new BigInt(1)), (257n))
    arktest.assertEQ(new BigInt(256).operatorBitwiseXor(new BigInt(256)), (0n))
    arktest.assertEQ(new BigInt(3124378143267041203423n).operatorBitwiseXor(new BigInt(43621978)), (3124378143267000928901n))
    arktest.assertEQ(new BigInt(12345678).operatorBitwiseXor(new BigInt(1234)), (12346780n))
}

function test_bitwise(): void {
    const a = 123456789123456789123456789123456789123456789123456789n
    let b = 123456790n
    const zero = 0n

    arktest.assertEQ(~zero, -1n)
    arktest.assertEQ(~a, -123456789123456789123456789123456789123456789123456790n)
}

function main(): void {
    test_bitwise_and()
    test_bitwise_or()
    test_bitwise_xor()
    test_bitwise()
}
