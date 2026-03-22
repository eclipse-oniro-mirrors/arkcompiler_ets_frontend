/*
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



function testPrimitiveArrays() {
    let intArr: ValueArray<int> = [1, 2, 3, 4, 5];
    arktest.assertEQ(intArr.length, 5);
    arktest.assertEQ(intArr[0], 1);
    arktest.assertEQ(intArr[4], 5);
    intArr[2] = 99;
    arktest.assertEQ(intArr[2], 99);
    arktest.assertTrue(intArr instanceof ValueArray<int>);


    let doubleArr: ValueArray<double> = [1.5, 2.7, 3.14];
    arktest.assertEQ(doubleArr.length, 3);
    arktest.assertEQ(doubleArr[1], 2.7);
    arktest.assertTrue(doubleArr instanceof ValueArray<double>);

    let boolArr: ValueArray<boolean> = [true, false, true];
    arktest.assertEQ(boolArr.length, 3);
    arktest.assertEQ(boolArr[0], true);
    arktest.assertTrue(boolArr instanceof ValueArray<boolean>);

    let byteArr: ValueArray<byte> = [127, -128, 0];
    arktest.assertEQ(byteArr.length, 3);
    arktest.assertEQ(byteArr[0], 127);
    arktest.assertTrue(byteArr instanceof ValueArray<byte>);

    let shortArr: ValueArray<short> = [32000, -32000, 0];
    arktest.assertEQ(shortArr.length, 3);
    arktest.assertEQ(shortArr[0], 32000);
    arktest.assertTrue(shortArr instanceof ValueArray<short>);

    let longArr: ValueArray<long> = [10000000000, -10000000000];
    arktest.assertEQ(longArr.length, 2);
    arktest.assertEQ(longArr[0], 10000000000);
    arktest.assertTrue(longArr instanceof ValueArray<long>);

    let floatArr: ValueArray<float> = [1.5f, -2.5f, 0.0f];
    arktest.assertEQ(floatArr.length, 3);
    arktest.assertEQ(floatArr[0], 1.5f);
    arktest.assertTrue(floatArr instanceof ValueArray<float>);
}

function testNewConstructor() {
    let arr1 = new ValueArray<int>(10);
    arktest.assertEQ(arr1.length, 10);
    arktest.assertEQ(arr1[0], 0); // default initialized to 0
    arr1[9] = 42;
    arktest.assertEQ(arr1[9], 42);

    let arr2 = new ValueArray<double>(5, 3.14);
    arktest.assertEQ(arr2.length, 5);
    arktest.assertEQ(arr2[2], 3.14);

    let arr3 = new ValueArray<boolean>(3, true);
    arktest.assertEQ(arr3.length, 3);
    arktest.assertEQ(arr3[1], true);
}

function testAssignmentAndSubtyping() {
    let valueArr: ValueArray<int> = [1, 2, 3];
    let fixedArr: FixedArray<int> = [1, 2, 3];
    arktest.assertTrue(valueArr instanceof ValueArray<int>);
    arktest.assertTrue(fixedArr instanceof FixedArray<int>);


}



function testIteration() {
    let arr: ValueArray<int> = [10, 20, 30];
    let sum = 0;
    for (let i = 0; i < arr.length; i++) {
        sum += arr[i];
    }
    arktest.assertEQ(sum, 60);

    let count = 0;
    for (let _ of arr) {
        count++;
    }
    arktest.assertEQ(count, 3);
}

function testGenericFunctions() {
    let sum = (arr: ValueArray<int>): int => {
        let total = 0;
        for (let i = 0; i < arr.length; i++) {
            total += arr[i];
        }
        return total;
    };
    
    let arr: ValueArray<int> = [1, 2, 3, 4, 5];
    arktest.assertEQ(sum(arr), 15);
    
    let intFirst = ([10, 20, 30] as ValueArray<int>)[0];
    arktest.assertEQ(intFirst, 10);
    
    let doubleFirst = ([1.5, 2.5] as ValueArray<double>)[0];
    arktest.assertEQ(doubleFirst, 1.5);
    
    let boolFirst = ([true, false] as ValueArray<boolean>)[0];
    arktest.assertEQ(boolFirst, true);
}

function testBoundsChecking() {
    let arr: ValueArray<int> = new ValueArray<int>(5);
    arr[0] = 100;
    arr[4] = 200;
    arktest.assertEQ(arr[0], 100);
    arktest.assertEQ(arr[4], 200);
    
    try {
        let x = arr[5];
        arktest.assertFalse(true);
    } catch (e) {
        arktest.assertTrue(e instanceof RangeError);
    }
    
    try {
        let idx = -1;
        arr[idx] = 0;
        arktest.assertFalse(true);
    } catch (e) {
        arktest.assertTrue(e instanceof RangeError);
    }
}

function testSpreadOperator() {
    let arr1: ValueArray<int> = [1, 2, 3];
    let arr2: ValueArray<int> = [4, 5, 6];
    let combined: ValueArray<int> = [...arr1, ...arr2];
    arktest.assertEQ(combined.length, 6);
    arktest.assertEQ(combined[0], 1);
    arktest.assertEQ(combined[5], 6);
    
}

function testConstAndReadonly() {
    const constArr: ValueArray<int> = [7, 8, 9];
    arktest.assertEQ(constArr.length, 3);
    arktest.assertEQ(constArr[1], 8);
    constArr[0] = 77;
    arktest.assertEQ(constArr[0], 77);
    
    let readonlyArr: ValueArray<int> = [10, 20, 30];
    arktest.assertEQ(readonlyArr.length, 3);
    arktest.assertEQ(readonlyArr[1], 20);
}

function testMultiDimensional() {
    let matrix: FixedArray<ValueArray<int>> = [
        [1, 2, 3] as ValueArray<int>,
        [4, 5, 6] as ValueArray<int>
    ];
    arktest.assertEQ(matrix.length, 2);
    arktest.assertEQ(matrix[0].length, 3);
    arktest.assertEQ(matrix[1][2], 6);
    arktest.assertTrue(matrix[0] instanceof ValueArray<int>);
    
}

function testTypeAssertions() {
    let arr: ValueArray<int> = [1, 2, 3];
    let fixedAttempt = arr as Object as FixedArray<int>;
    arktest.assertTrue(fixedAttempt instanceof FixedArray<int>);
    arktest.assertTrue(fixedAttempt instanceof ValueArray<int>);
    
    let obj: Object = arr as Object;
    arktest.assertTrue(obj instanceof Object);
    arktest.assertTrue(obj instanceof ValueArray<int>);
}

function testArrayMethods() {
    let arr: ValueArray<int> = [5, 10, 15, 20];
    arktest.assertEQ(arr.length, 4);
    
    let sum = 0;
    for (let val of arr) {
        sum += val;
    }
    arktest.assertEQ(sum, 50);
    
    let str = arr.toString();
    arktest.assertTrue(str.includes("5"));
    
}

function testEdgeCases() {
    let empty: ValueArray<int> = [];
    arktest.assertEQ(empty.length, 0);
    
    let single: ValueArray<double> = [3.14];
    arktest.assertEQ(single.length, 1);
    arktest.assertEQ(single[0], 3.14);
    
    let large = new ValueArray<int>(1000);
    arktest.assertEQ(large.length, 1000);
    large[999] = 9999;
    arktest.assertEQ(large[999], 9999);
    
    let defaultInts = new ValueArray<int>(5);
    arktest.assertEQ(defaultInts[0], 0);
    arktest.assertEQ(defaultInts[4], 0);
    
    let defaultBools = new ValueArray<boolean>(3);
    arktest.assertEQ(defaultBools[0], false);
    
    let defaultDoubles = new ValueArray<double>(2);
    arktest.assertEQ(defaultDoubles[0], 0.0);
}

function testPerformancePatterns() {
    let values: ValueArray<double> = new ValueArray<double>(100);
    for (let i = 0; i < values.length; i++) {
        values[i] = i * 1.5;
    }
    
    let total = 0.0;
    for (let i = 0; i < values.length; i++) {
        total += values[i];
    }
    arktest.assertTrue(total > 0);
    
    let swapArr: ValueArray<int> = [1, 2];
    let temp = swapArr[0];
    swapArr[0] = swapArr[1];
    swapArr[1] = temp;
    arktest.assertEQ(swapArr[0], 2);
    arktest.assertEQ(swapArr[1], 1);
}

function main() {
    testPrimitiveArrays();
    testNewConstructor();
    testAssignmentAndSubtyping();
    testIteration();
    testGenericFunctions();
    testBoundsChecking();
    testSpreadOperator();
    testConstAndReadonly();
    testMultiDimensional();
    testTypeAssertions();
    testArrayMethods();
    testEdgeCases();
    testPerformancePatterns();
    console.log("All ValueArray tests passed");
}
