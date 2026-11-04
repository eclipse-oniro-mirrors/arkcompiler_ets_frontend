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

// Test logical expressions with short-circuit semantics
let callCount = 0;
let result: string = "";
let array1: int[] = [10, 20, 30];
let array2: int[] = [100, 200, 300];
let nullableArray: int[] | null = null;
let defaultArray: int[] = [1, 2, 3];

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

function getNullableArray(): int[] | null {
    callCount++;
    result = result + "getNullableArray ";
    return nullableArray;
}

function getDefaultArray(): int[] {
    callCount++;
    result = result + "getDefaultArray ";
    return defaultArray;
}

function testLogicalAND() {
    // Test: (getArray1() && getArray2())[idx] += value
    // getArray1() returns truthy array, so getArray2() should be called
    callCount = 0;
    result = "";
    array1 = [10, 20, 30];
    array2 = [100, 200, 300];
    let idx = 1;
    let value = 5;
    ((getArray1() && getArray2()))[idx] += value;
    arktest.assertEQ(callCount, 2); // Both should be called
    arktest.assertEQ(result, "getArray1 getArray2 ");
    arktest.assertEQ(array2[idx], 205); // 200 + 5 = 205 (getArray2() is the result)
    arktest.assertEQ(array1[idx], 20); // array1 should not be modified
}

function testLogicalOR() {
    // Test: (getArray1() || getArray2())[idx] += value
    // getArray1() returns truthy array, so getArray2() should not be called (short-circuit)
    callCount = 0;
    result = "";
    array1 = [10, 20, 30];
    array2 = [100, 200, 300];
    let idx = 1;
    let value = 5;
    ((getArray1() || getArray2()))[idx] += value;
    arktest.assertEQ(callCount, 1); // Only getArray1() should be called
    arktest.assertEQ(result, "getArray1 ");
    arktest.assertEQ(array1[idx], 25); // 20 + 5 = 25
    arktest.assertEQ(array2[idx], 200); // array2 should not be modified
}

function testLogicalORWithFunctionCalls() {
    // Test: (getArray1() || getArray2())[getIndex(0)] += getValue(10)
    // More complex: logical OR with function calls in index and value
    callCount = 0;
    result = "";
    array1 = [10, 20, 30];
    array2 = [100, 200, 300];
    ((getArray1() || getArray2()))[getIndex(0)] += getValue(10);
    arktest.assertEQ(callCount, 3); // getArray1, getIndex, getValue (getArray2 should not be called)
    arktest.assertEQ(result, "getArray1 getIndex getValue ");
    arktest.assertEQ(array1[0], 20); // 10 + 10 = 20
    arktest.assertEQ(array2[0], 100); // array2 should not be modified
}

function testNullishCoalescing() {
    // Test: (getNullableArray() ?? getDefaultArray())[idx] += value
    // getNullableArray() returns null, so getDefaultArray() should be called
    callCount = 0;
    result = "";
    nullableArray = null;
    defaultArray = [1, 2, 3];
    let idx = 1;
    let value = 10;
    ((getNullableArray() ?? getDefaultArray()))[idx] += value;
    arktest.assertEQ(callCount, 2); // Both should be called to check nullish
    arktest.assertEQ(result, "getNullableArray getDefaultArray ");
    arktest.assertEQ(defaultArray[idx], 12); // 2 + 10 = 12
}

function testNullishCoalescingWithFunctionCalls() {
    // Test: (getNullableArray() ?? getDefaultArray())[getIndex(0)] *= getValue(3)
    // More complex: nullish coalescing with function calls in index and value
    callCount = 0;
    result = "";
    nullableArray = [10, 20, 30];
    defaultArray = [1, 2, 3];
    ((getNullableArray() ?? getDefaultArray()))[getIndex(0)] *= getValue(3);
    // getNullableArray() returns non-null, so getDefaultArray() should not be called
    // getIndex() and getValue() should each be called once
    arktest.assertEQ(callCount, 3); // getNullableArray, getIndex, getValue
    arktest.assertEQ(result, "getNullableArray getIndex getValue ");
    arktest.assertEQ(nullableArray?.[0], 30); // 10 * 3 = 30
    arktest.assertEQ(defaultArray[0], 1); // defaultArray should not be modified
}

function main() {
    testLogicalAND();
    testLogicalOR();
    testLogicalORWithFunctionCalls();
    testNullishCoalescing();
    testNullishCoalescingWithFunctionCalls();
}

