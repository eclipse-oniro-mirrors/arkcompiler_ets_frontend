/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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


// ============================================================================
// Part 1: Positive Examples (Safe Cases - No Errors)
// ============================================================================

// ============================================================================
// Table 1: Literal Index - Boundary Condition Match (Positive)
// ============================================================================

// 1.1 arr.length > 10 access arr[10] - safe
let arr1_1: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
if (arr1_1.length > 10) {
    arr1_1[10]; // safe:
}

// 1.2 arr.length >= 11 access arr[10] - safe
let arr1_2: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
if (arr1_2.length >= 11) {
    arr1_2[10]; // safe:
}

// 1.3 arr.length === 11 access arr[10] - safe
let arr1_3: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
if (arr1_3.length === 11) {
    arr1_3[10]; // safe:
}

// 1.4 10 < arr.length access arr[10] - safe
let arr1_4: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
if (10 < arr1_4.length) {
    arr1_4[10]; // safe:
}

// 1.6 arr.length > 10 access arr[9] - safe (smaller index)
let arr1_6: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
if (arr1_6.length > 10) {
    arr1_6[9]; // safe:
}

// 1.7 arr.length >= 10 access arr[9] - safe
let arr1_7: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
if (arr1_7.length >= 10) {
    arr1_7[9]; // safe:
}

// 1.8 arr.length > 0 access arr[0] - safe
let arr1_8: number[] = [1, 2, 3];
if (arr1_8.length > 0) {
    arr1_8[0]; // safe:
}

// ============================================================================
// Table 2: Variable Index - Boundary Condition Match (Positive)
// ============================================================================

// 2.1 index < arr.length access arr[index] - safe
let arr2_1: number[] = [1, 2, 3, 4, 5];
let idx2_1: number = 3;
if (idx2_1 < arr2_1.length) {
    arr2_1[idx2_1]; // safe:
}

// 2.2 arr.length > index access arr[index] - safe
let arr2_2: number[] = [1, 2, 3, 4, 5];
let idx2_2: number = 3;
if (arr2_2.length > idx2_2) {
    arr2_2[idx2_2]; // safe:
}

// 2.3 index >= 0 && index < arr.length - safe
let arr2_3: number[] = [1, 2, 3, 4, 5];
let idx2_3: number = 3;
if (idx2_3 >= 0 && idx2_3 < arr2_3.length) {
    arr2_3[idx2_3]; // safe:
}

// 2.4 index > 0 && index < arr.length - safe
let arr2_4: number[] = [1, 2, 3, 4, 5];
let idx2_4: number = 3;
if (idx2_4 > 0 && idx2_4 < arr2_4.length) {
    arr2_4[idx2_4]; // safe:
}

// 2.5 arr.length > index && index >= 0 - safe
let arr2_5: number[] = [1, 2, 3, 4, 5];
let idx2_5: number = 3;
if (arr2_5.length > idx2_5 && idx2_5 >= 0) {
    arr2_5[idx2_5]; // safe:
}

// 2.6 0 <= index && index < arr.length - safe
let arr2_6: number[] = [1, 2, 3, 4, 5];
let idx2_6: number = 3;
if (0 <= idx2_6 && idx2_6 < arr2_6.length) {
    arr2_6[idx2_6]; // safe:
}

// 2.7 arr.length > index && index > 0 - safe
let arr2_7: number[] = [1, 2, 3, 4, 5];
let idx2_7: number = 3;
if (arr2_7.length > idx2_7 && idx2_7 > 0) {
    arr2_7[idx2_7]; // safe:
}

// ============================================================================
// Table 3: For Loop - Standard Form (Positive)
// ============================================================================

// 3.1 for (let i = 0; i < arr.length; i++) { arr[i]; } - safe
let arr3_1: number[] = [1, 2, 3, 4, 5];
for (let i = 0; i < arr3_1.length; i++) {
    arr3_1[i]; // safe:
}

// 3.2 for (let i = 0; i <= arr.length - 1; i++) { arr[i]; } - safe
let arr3_2: number[] = [1, 2, 3, 4, 5];
for (let i = 0; i <= arr3_2.length - 1; i++) {
    arr3_2[i]; // safe:
}

// 3.3 for (let i = 1; i < arr.length; i++) { arr[i]; } - safe
let arr3_3: number[] = [1, 2, 3, 4, 5];
for (let i = 1; i < arr3_3.length; i++) {
    arr3_3[i]; // safe:
}

// 3.4 for (let i = arr.length - 1; i >= 0; i--) { arr[i]; } - safe
let arr3_4: number[] = [1, 2, 3, 4, 5];
for (let i = arr3_4.length - 1; i >= 0; i--) {
    arr3_4[i]; // safe:
}

// 3.5 for (const item of arr) - safe
let arr3_5: number[] = [1, 2, 3, 4, 5];
for (const item of arr3_5) {
    let x = item; // safe:
}

// ============================================================================
// Table 4: While Loop (Positive)
// ============================================================================

// 4.1 while (i < arr.length) { arr[i]; i++; } - safe
let arr4_1: number[] = [1, 2, 3, 4, 5];
let i4_1: number = 0;
while (i4_1 < arr4_1.length) {
    arr4_1[i4_1]; // safe:
    i4_1++;
}

// 4.2 while (i >= 0 && i < arr.length) { arr[i]; i++; } - safe
let arr4_2: number[] = [1, 2, 3, 4, 5];
let i4_2: number = 0;
while (i4_2 >= 0 && i4_2 < arr4_2.length) {
    arr4_2[i4_2]; // safe:
    i4_2++;
}

// ============================================================================
// Table 5: After Array Modification - Increased Length (Positive)
// ============================================================================

// 5.1 if (arr.length > 10) { arr.push(x); arr[10]; } - safe
let arr5_1: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
if (arr5_1.length > 10) {
    arr5_1.push(12);
    arr5_1[10]; // safe:
}

// 5.2 if (arr.length > 10) { arr.unshift(x); arr[10]; } - safe
let arr5_2: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
if (arr5_2.length > 10) {
    arr5_2.unshift(0);
    arr5_2[10]; // safe:
}

// 5.3 for (let i = 0; i < arr.length; i++) { arr.push(i); arr[i]; } - safe
let arr5_3: number[] = [1, 2, 3];
for (let i5_3 = 0; i5_3 < arr5_3.length; i5_3++) {
    arr5_3.push(i5_3);
    arr5_3[i5_3]; // safe:
}

// ============================================================================
// Table 6: Index Decrement - Decreasing Index (Positive)
// ============================================================================

// 6.1 if (idx < arr.length) { idx--; arr[idx]; } - safe
let arr6_1: number[] = [1, 2, 3, 4, 5, 6];
let idx6_1: number = 5;
if (idx6_1 < arr6_1.length) {
    idx6_1--;
    arr6_1[idx6_1]; // safe:
}

// 6.2 if (idx < arr.length) { idx -= 1; arr[idx]; } - safe
let arr6_2: number[] = [1, 2, 3, 4, 5, 6];
let idx6_2: number = 5;
if (idx6_2 < arr6_2.length) {
    idx6_2 -= 1;
    arr6_2[idx6_2]; // safe:
}

// 6.3 if (idx < arr.length) { arr[idx--]; } - safe (uses original value)
let arr6_3: number[] = [1, 2, 3, 4, 5, 6];
let idx6_3: number = 5;
if (idx6_3 < arr6_3.length) {
    arr6_3[idx6_3--]; // safe:
}

// 6.4 if (idx < arr.length) { arr[--idx]; } - safe (uses decreased value)
let arr6_4: number[] = [1, 2, 3, 4, 5, 6];
let idx6_4: number = 5;
if (idx6_4 < arr6_4.length) {
    arr6_4[--idx6_4]; // safe:
}

// ============================================================================
// Table 7: Nested Conditions (Positive)
// ============================================================================

// 7.1 if (len > 10) { if (len > 5) { arr[5]; } } - safe
let arr8_1: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
if (arr8_1.length > 10) {
    if (arr8_1.length > 5) {
        arr8_1[5]; // safe:
    }
}

// 7.2 if (idx >= 0) { if (idx < arr.length) { arr[idx]; } } - safe
let arr7_2: number[] = [1, 2, 3, 4, 5];
let idx7_2: number = 3;
if (idx7_2 >= 0) {
    if (idx7_2 < arr7_2.length) {
        arr7_2[idx7_2]; // safe:
    }
}

// ============================================================================
// Table 8: Constant Index (Positive)
// ============================================================================

// 8.1 const idx = 5; if (arr.length > 5) { arr[idx]; } - safe
let arr8_1: number[] = [1, 2, 3, 4, 5, 6];
const idx8_1: number = 5;
if (arr8_1.length > idx8_1) {
    arr8_1[idx8_1]; // safe:
}

// 8.2 if (arr.length > 10) { arr[10]; } - safe
let arr8_2: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
if (arr8_2.length > 10) {
    arr8_2[10]; // safe:
}

// ============================================================================
// Table 9: Type Related (Positive)
// ============================================================================

// 9.1 number type index - safe
let arr9_1: number[] = [1, 2, 3];
let numIdx9_1: number = 1;
if (numIdx9_1 < arr9_1.length) {
    arr9_1[numIdx9_1]; // safe:
}

// 9.2 enum value within range - safe
enum E1 { A = 0, B = 1, C = 2 }
let arr9_2: number[] = [1, 2, 3];
let enumIdx9_2: E1 = E1.B;
if (enumIdx9_2 < arr9_2.length) {
    arr9_2[enumIdx9_2]; // safe:
}

// 9.3 postfix increment in bound check - safe (uses original value then increments)
let arr9_3: object[] = [1, 2, 3];
let idx9_3 = 1;
if (idx9_3 < arr9_3.length) {
    const value = arr9_3[idx9_3++]; // safe: postfix increment uses original value
}

// 9.4 postfix decrement in bound check - safe (uses original value then decrements)
let arr9_4: object[] = [1, 2, 3];
let idx9_4 = 1;
if (idx9_4 < arr9_4.length) {
    const value = arr9_4[idx9_4--]; // safe: postfix decrement uses original value
}

// ============================================================================
// Table 10: Safe Ternary Expression (Positive)
// ============================================================================

// 10.1 condition ? arr[0] : default - safe
let arr10_1: number[] = [1, 2, 3];
if (arr10_1.length > 0) {
    let val = arr10_1.length > 0 ? arr10_1[0] : 0; // safe:
}

// ============================================================================
// Table 11: Safe Array Method Usage (Positive)
// ============================================================================

// 11.1 forEach safe usage
let arr11_1: number[] = [1, 2, 3];
arr11_1.forEach((item, index) => {
    let x = item; // safe:
});

// 11.2 map safe usage
let arr11_2: number[] = [1, 2, 3];
let mapped11_2 = arr11_2.map((item, index) => item * 2); // safe:

// 11.3 filter safe usage
let arr11_3: number[] = [1, 2, 3];
let filtered11_3 = arr11_3.filter(item => item > 1); // safe:

// ============================================================================

const arr: Map<string, string> = new Map();
let keys: string[] = Object.keys(arr);
let values: string[] = Object.keys(arr);
for (let i = 0; i < keys.length; i++) {
    values[i];
}

let a: string[] = [];
let b: Array = new Array(a.length);
for (let i = 0; i < a.length; i++) {
    b[i]; // safe:
}

for (let i = 0; i < a.length; i++) {
    b.push("abc");
    b[i]; // safe:
}


//Supplement Scenarios 
//1.Exist both correct judgment of Lower Bound and Upper Bound
let arr14_1: number[] = [1, 2, 3, 4, 5];
let idx14_1: int = 10;
if (idx14_1 > 0 && idx14_1 < arr14_1.length) {
  arr14_1[idx14_1];
}
//corresponding negative case: if (idx14_1 > 0) 

//2. correct judgment: index less than or equal with length - 1
let arr14_2: number[] = [1, 2, 3];
let idx14_2: int = 2;
if (idx14_2 <= arr14_2.length - 1) {
  arr14_2[idx14_2]; 
}
//corresponding negative case: if (idx14_2 <= arr14_2.length)

//3. correct judgment: index equal with length - 1
let arr14_4: number[] = [1, 2, 3, 4, 5];
let idx14_4: int = 5;
if (arr14_4.length - 1 === idx14_4) { 
  arr14_4[idx14_4]; 
}
//corresponding negative case: if (arr14_4.length === idx14_4)