/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Test for BuiltinNewArrayArgChecker branches

// Array constructor with callback
function testArrayConstructorCallback(): void {
  const size: int = 5; // arkts-builtin-api-num2int
  const arr1 = new Array<number>(size);
  
  // Array literal (should not trigger)
  const arr3 = [1, 2, 3];
  console.log(arr3.length);
}

// Array with size and initializer
function testArrayWithInitializer(): void {
  const len: int = 3; // arkts-builtin-api-num2int
  
  // Array constructor with length
  const arr1: string[] = [];
  
  console.log(arr1.join(', '));
}

// Array with non-number size
function testArrayNonNumberSize(): void {
  // This should trigger warning for size
  const arr = new Array<number>(10); // arkts-builtin-api-num2int
  console.log(arr.length);
}

// Array with function initializer
function testArrayFunctionInitializer(): void {
  const size: number = 4; // arkts-builtin-api-num2int
  console.log(size);
}

// Typed array with number index
function testTypedArrayIndex(): void {
  const idx: int = 2; // arkts-builtin-api-num2int
  const uint8 = new Uint8Array(10);
  console.log(uint8[idx]);
}

// Array index assignment with number
function testArrayIndexAssignment(): void {
  const arr: string[] = ['a', 'b', 'c'];
  const index: int = 1; // arkts-builtin-api-num2int
  console.log(arr[index]);
}

// Array with callback returning number
function testArrayCallbackReturn(): void {
  const arr = [1, 2, 3, 4, 5];
  const multiplier: int = 10; // arkts-builtin-api-num2int
  
  const mapped = arr.map((value: number) => value * multiplier); // arkts-builtin-api-num2int
  console.log(mapped[0]);
}// Test for NumericLocalUsageAnalyzer: Array operation scenarios

function arrayCreationTest(): void {
  const size: int = 10 // arkts-numeric-semantic - array size
  const arr: number[] = new Array(size)
}

function arrayWithInitializer(): void {
  const count: int = 20 // arkts-numeric-semantic
  const arr: number[] = new Array(count).fill(0)
}

function arrayFromTest(): void {
  const length: int = 30 // arkts-numeric-semantic
  const arr: number[] = Array.from({ length }, (_, i) => i)
}

function arrayConcatTest(): void {
  const size1: int = 40 // arkts-numeric-semantic
  const size2: int = 50 // arkts-numeric-semantic
  const arr1: number[] = new Array(size1)
  const arr2: number[] = new Array(size2)
  const combined = arr1.concat(arr2)
}

function arraySliceTest(): void {
  const start: number = 60 // arkts-numeric-semantic
  const end: number = 70 // arkts-numeric-semantic
  const arr: number[] = [1, 2, 3, 4, 5]
  const sliced = arr.slice(start, end)
}

function arrayIndexOfTest(): void {
  const value: number = 80 // arkts-numeric-semantic
  const arr: number[] = [1, 2, 3, 4, 5]
  const index: number = arr.indexOf(value)
  const resultArr: number[] = new Array(index + 1)
}

function arrayMapTest(): void {
  const multiplier: int = 90 // arkts-numeric-semantic
  const arr: number[] = [1, 2, 3]
  const mapped = arr.map(x => x * multiplier)
}

function arrayFilterTest(): void {
  const threshold: int = 100 // arkts-numeric-semantic
  const arr: number[] = [10, 20, 30, 40, 50]
  const filtered = arr.filter(x => x > threshold)
}

function arrayReduceTest(): void {
  const initial: number = 110 // arkts-numeric-semantic
  const arr: number[] = [1, 2, 3, 4, 5]
  const sum = arr.reduce((acc, val) => acc + val, initial)
}

function arrayPushPopTest(): void {
  const pushValue: number = 120 // arkts-numeric-semantic
  const arr: number[] = [1, 2, 3]
  arr.push(pushValue)
  const popped: number = arr.pop() ?? 0 // arkts-numeric-semantic
  const newArr: number[] = new Array(popped)
}

function arraySpliceTest(): void {
  const start: number = 130 // arkts-numeric-semantic
  const deleteCount: number = 140 // arkts-numeric-semantic
  const arr: number[] = [1, 2, 3, 4, 5]
  arr.splice(start, deleteCount)
}

function nestedArrayTest(): void {
  const outerSize: int = 150 // arkts-numeric-semantic
  const innerSize: int = 160 // arkts-numeric-semantic
  const matrix: number[][] = new Array(outerSize).fill(new Array(innerSize))
}
