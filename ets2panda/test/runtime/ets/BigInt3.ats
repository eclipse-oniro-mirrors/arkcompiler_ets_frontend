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

function test_module(): void {
    arktest.assertEQ(new BigInt(10).operatorModule(new BigInt(3)), (1n))
    arktest.assertEQ(new BigInt(10).operatorModule(new BigInt(-3)), (1n))
    arktest.assertEQ(new BigInt(-10).operatorModule(new BigInt(3)), (-1n))
    arktest.assertEQ(new BigInt(-10).operatorModule(new BigInt(-3)), (-1n))
    arktest.assertEQ(new BigInt(100).operatorModule(new BigInt(50)), (0n))
    arktest.assertEQ(new BigInt(100).operatorModule(new BigInt(-50)), (0n))
    arktest.assertEQ(new BigInt(-100).operatorModule(new BigInt(50)), (0n))
    arktest.assertEQ(new BigInt(-100).operatorModule(new BigInt(-50)), (0n))
    arktest.assertEQ(new BigInt(3124378143267041203423n).operatorModule(new BigInt(43621978)), (18802883n))
    arktest.assertEQ(new BigInt(-3124378143267041203423n).operatorModule(new BigInt(43621978)), (-18802883n))
    arktest.assertEQ(new BigInt(3124378143267041203423n).operatorModule(new BigInt(-43621978)), (18802883n))
    arktest.assertEQ(new BigInt(-3124378143267041203423n).operatorModule(new BigInt(-43621978)), (-18802883n))
    arktest.assertEQ(new BigInt(100).operatorModule(new BigInt(250)), (100n))
    arktest.assertEQ(new BigInt(-100).operatorModule(new BigInt(250)), (-100n))
    arktest.assertEQ(new BigInt(100).operatorModule(new BigInt(-250)), (100n))
    arktest.assertEQ(new BigInt(-100).operatorModule(new BigInt(-250)), (-100n))
    arktest.assertEQ(new BigInt(0).operatorModule(new BigInt(8)), (0n))
}

function main() : void {
    test_module()
}

