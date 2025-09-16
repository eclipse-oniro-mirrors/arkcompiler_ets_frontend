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

function test_divide(): void {
    arktest.assertEQ(new BigInt(10).operatorDivide(new BigInt(3)), (3n))
    arktest.assertEQ(new BigInt(-10).operatorDivide(new BigInt(3)), (-3n))
    arktest.assertEQ(new BigInt(10).operatorDivide(new BigInt(-3)), (-3n))
    arktest.assertEQ(new BigInt(-10).operatorDivide(new BigInt(-3)), (3n))
    arktest.assertEQ(new BigInt(100).operatorDivide(new BigInt(50)), (2n))
    arktest.assertEQ(new BigInt(100).operatorDivide(new BigInt(-50)), (-2n))
    arktest.assertEQ(new BigInt(-100).operatorDivide(new BigInt(50)), (-2n))
    arktest.assertEQ(new BigInt(-100).operatorDivide(new BigInt(-50)), (2n))
    arktest.assertEQ(new BigInt(3124378143267041203423n).operatorDivide(new BigInt(43621978)), (71623944775430n))
    arktest.assertEQ(new BigInt(-3124378143267041203423n).operatorDivide(new BigInt(43621978)), (-71623944775430n))
    arktest.assertEQ(new BigInt(3124378143267041203423n).operatorDivide(new BigInt(-43621978)), (-71623944775430n))
    arktest.assertEQ(new BigInt(-3124378143267041203423n).operatorDivide(new BigInt(-43621978)), (71623944775430n))
    arktest.assertEQ(new BigInt(100).operatorDivide(new BigInt(250)), (0n))
    arktest.assertEQ(new BigInt(100).operatorDivide(new BigInt(-250)), (0n))
    arktest.assertEQ(new BigInt(-100).operatorDivide(new BigInt(250)), (0n))
    arktest.assertEQ(new BigInt(-100).operatorDivide(new BigInt(-250)), (0n))
    arktest.assertEQ(new BigInt(65000).operatorDivide(new BigInt(100)), (650n))
    arktest.assertEQ(new BigInt(65000).operatorDivide(new BigInt(-100)), (-650n))
    arktest.assertEQ(new BigInt(-65000).operatorDivide(new BigInt(100)), (-650n))
    arktest.assertEQ(new BigInt(-65000).operatorDivide(new BigInt(-100)), (650n))
}

function main() : void {
    test_divide()
}
