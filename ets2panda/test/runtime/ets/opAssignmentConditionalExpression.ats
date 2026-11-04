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

let callCount = 0;
let result: string = "";
let array1: int[] = [10, 20, 30];
let array2: int[] = [100, 200, 300];

function getArray1(): int[] {
    callCount++;
    result = result + "getArray1 ";
    return array1;
}

function getArray2(): int[] {
    callCount++;
    result = result + "getArray2 ";
    return array2;
}

function getIndex(ind: int): int {
    callCount++;
    result = result + "getIndex ";
    return ind;
}

function getValue(val: int): int {
    callCount++;
    result = result + "getValue ";
    return val;
}

function testConditionalExpressionTrue() {
    callCount = 0;
    result = "";
    let condition = true;
    let idx = 1;
    let value = 5;
    ((condition ? getArray1() : getArray2()))[idx] += value;
    // getArray1() should be called once (condition is true), getArray2() should not be called
    arktest.assertEQ(callCount, 1);
    arktest.assertEQ(result, "getArray1 ");
    // The array1[1] should be modified: 20 + 5 = 25
    arktest.assertEQ(array1[1], 25);
    arktest.assertEQ(array2[1], 200); // array2 should not be modified
}

function testConditionalExpressionFalse() {
    callCount = 0;
    result = "";
    array1 = [10, 20, 30];
    array2 = [100, 200, 300];
    let condition = false;
    let idx = 0;
    let value = 10;
    ((condition ? getArray1() : getArray2()))[idx] += value;
    // getArray2() should be called once (condition is false), getArray1() should not be called
    arktest.assertEQ(callCount, 1);
    arktest.assertEQ(result, "getArray2 ");
    // array2[0] should be modified: 100 + 10 = 110
    arktest.assertEQ(array2[0], 110);
    arktest.assertEQ(array1[0], 10); // array1 should not be modified
}

function testConditionalExpressionWithFunctionCalls() {
    callCount = 0;
    result = "";
    array1 = [10, 20, 30];
    array2 = [100, 200, 300];
    let condition = true;
    ((condition ? getArray1() : getArray2()))[getIndex(2)] *= getValue(3);
    // getArray1(), getIndex(), getValue() should each be called once
    arktest.assertEQ(callCount, 3);
    arktest.assertEQ(result, "getArray1 getIndex getValue ");
    // array1[2] should be modified: 30 * 3 = 90
    arktest.assertEQ(array1[2], 90);
    arktest.assertEQ(array2[2], 300); // array2 should not be modified
}

function main() {
    testConditionalExpressionTrue();
    testConditionalExpressionFalse();
    testConditionalExpressionWithFunctionCalls();
}

