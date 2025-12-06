/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

function testBinaryOperationInIndex() {
    let arr: number[] = [1, 2, 3];
    let idx1 = 0;
    let idx2 = 1;
    arr[idx1 + idx2] *= 3;
    arktest.assertEQ(arr[1], 6); // 2 * 3 = 6
}

function testSubtractionInIndex() {
    let arr: number[] = [10, 20, 30, 40];
    let idx1 = 3;
    let idx2 = 1;
    arr[idx1 - idx2] += 5;
    arktest.assertEQ(arr[2], 35); // 30 + 5 = 35
}

function testMultiplicationInIndex() {
    let arr: number[] = [1, 2, 3, 4, 5, 6];
    let idx1 = 2;
    let idx2 = 1;
    arr[idx1 * idx2] -= 1;
    arktest.assertEQ(arr[2], 2); // 3 - 1 = 2
}

function main() {
    testBinaryOperationInIndex();
    testSubtractionInIndex();
    testMultiplicationInIndex();
}

