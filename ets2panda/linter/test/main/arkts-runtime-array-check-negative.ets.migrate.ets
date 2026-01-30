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
// Table 12: Literal Index - Boundary Mismatch (Negative)
// ============================================================================

// 12.1 arr.length > 10 access arr[11] - unsafe
let arr12_1: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
if (arr12_1.length > 10) {
    arr12_1[11]; // unsafe:
}

// 12.2 arr.length >= 10 access arr[10] - unsafe
let arr12_2: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
if (arr12_2.length >= 10) {
    arr12_2[10]; // unsafe:
}

// 12.3 arr.length === 10 access arr[10] - unsafe
let arr12_3: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
if (arr12_3.length === 10) {
    arr12_3[10]; // unsafe:
}

// 12.4 arr.length > 9 access arr[10] - unsafe
let arr12_4: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
if (arr12_4.length > 9) {
    arr12_4[10]; // unsafe:
}

// ============================================================================
// Table 13: Variable Index - Missing Checks (Negative)
// ============================================================================

// 13.1 index > 0 access arr[index] - unsafe (missing upper bound)
let arr14_1: number[] = [1, 2, 3, 4, 5];
let idx14_1: number = 10;
if (idx14_1 > 0) {
    arr14_1[idx14_1]; // unsafe:
}

// 13.2 index <= 10 access arr[index] - unsafe (no length comparison)
let arr14_2: number[] = [1, 2, 3];
let idx14_2: number = 5;
if (idx14_2 <= 10) {
    arr14_2[idx14_2]; // unsafe:
}

// 13.3 arr.length >= index access arr[index] - unsafe
let arr14_3: number[] = [1, 2, 3, 4, 5];
let idx14_3: number = 5;
if (arr14_3.length >= idx14_3) {
    arr14_3[idx14_3]; // unsafe:
}

// 13.4 arr.length === index access arr[index] - unsafe
let arr14_4: number[] = [1, 2, 3, 4, 5];
let idx14_4: number = 5;
if (arr14_4.length === idx14_4) {
    arr14_4[idx14_4]; // unsafe:
}

// 13.5 index <= arr.length access arr[index] - unsafe
let arr14_5: number[] = [1, 2, 3];
let idx14_5: number = 3;
if (idx14_5 <= arr14_5.length) {
    arr14_5[idx14_5]; // unsafe:
}

// 13.6 arr.length > index - 1 access arr[index] - unsafe
let arr14_6: number[] = [1, 2, 3];
let idx14_6: number = 2;
if (arr14_6.length > idx14_6 - 1) {
    arr14_6[idx14_6]; // unsafe:
}

// ============================================================================
// Table 14: Loops - Boundary Errors (Negative)
// ============================================================================

// 14.1 for (let i = 0; i <= arr.length; i++) { arr[i]; } - unsafe
let arr15_1: number[] = [1, 2, 3, 4, 5];
for (let i = 0; i <= arr15_1.length; i++) {
    arr15_1[i]; // unsafe:
}

// 14.2 for (let i = 1; i <= arr.length; i++) { arr[i]; } - unsafe
let arr15_2: number[] = [1, 2, 3, 4, 5];
for (let i = 1; i <= arr15_2.length; i++) {
    arr15_2[i]; // unsafe:
}

// 14.3 for (let i = 0; i < arr.length; i++) { arr[10]; } - unsafe
let arr15_3: number[] = [1, 2, 3];
for (let i = 0; i < arr15_3.length; i++) {
    arr15_3[10]; // unsafe:
}

// 14.4 while (i <= arr.length) { arr[i]; i++; } - unsafe
let arr15_4: number[] = [1, 2, 3, 4, 5];
let i15_4: number = 0;
while (i15_4 <= arr15_4.length) {
    arr15_4[i15_4]; // unsafe:
    i15_4++;
}

// 14.5 do { arr[i]; i++; } while (i < arr.length) - unsafe (access before check)
let arr15_5: number[] = [1, 2, 3, 4, 5];
let i15_5: number = 0;
do {
    arr15_5[i15_5]; // unsafe: access before boundary check
    i15_5++;
} while (i15_5 < arr15_5.length);

// ============================================================================
// Table 15: After Array Modification - Reduced Length (Negative)
// ============================================================================

// 15.1 if (arr.length > 10) { arr.pop(); arr[10]; } - unsafe
let arr16_1: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
if (arr16_1.length > 10) {
    arr16_1.pop();
    arr16_1[10]; // unsafe:
}

// 15.2 for (let i = 0; i < arr.length; i++) { arr.pop(); arr[i]; } - unsafe
let arr16_2: number[] = [1, 2, 3, 4, 5];
for (let i16_2 = 0; i16_2 < arr16_2.length; i16_2++) {
    arr16_2.pop();
    arr16_2[i16_2]; // unsafe:
}

// 15.3 if (arr.length > 10) { arr.shift(); arr[10]; } - unsafe
let arr16_3: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
if (arr16_3.length > 10) {
    arr16_3.shift();
    arr16_3[10]; // unsafe:
}

// ============================================================================
// Table 16: Index Increment - Increasing Index (Negative)
// ============================================================================

// 16.1 if (idx < arr.length) { idx++; arr[idx]; } - unsafe
let arr17_1: number[] = [1, 2, 3, 4, 5, 6];
let idx17_1: number = 5;
if (idx17_1 < arr17_1.length) {
    idx17_1++;
    arr17_1[idx17_1]; // unsafe:
}

// 16.2 if (idx < arr.length) { idx += 1; arr[idx]; } - unsafe
let arr17_2: number[] = [1, 2, 3, 4, 5, 6];
let idx17_2: number = 5;
if (idx17_2 < arr17_2.length) {
    idx17_2 += 1;
    arr17_2[idx17_2]; // unsafe:
}

// 16.3 if (idx < arr.length) { idx = 10; arr[idx]; } - unsafe
let arr17_3: number[] = [1, 2, 3, 4, 5];
let idx17_3: number = 2;
if (idx17_3 < arr17_3.length) {
    idx17_3 = 10;
    arr17_3[idx17_3]; // unsafe:
}

// 16.4 if (idx < arr.length) { idx += 5; arr[idx]; } - unsafe
let arr17_4: number[] = [1, 2, 3, 4, 5];
let idx17_4: number = 2;
if (idx17_4 < arr17_4.length) {
    idx17_4 += 5;
    arr17_4[idx17_4]; // unsafe:
}

// ============================================================================
// Table 17: Increment/Decrement in Expression (Negative)
// ============================================================================

// 17.1 if (idx < arr.length) { arr[++idx]; } - unsafe
let arr17_1: number[] = [1, 2, 3, 4, 5];
let idx17_1: number = 4;
if (idx17_1 < arr17_1.length) {
    arr17_1[++idx17_1 as int]; // unsafe:
}

// 17.2 if (idx >= 0) { arr[idx--]; } - unsafe (missing upper bound check)
let arr17_2: number[] = [1, 2, 3];
let idx17_2: number = 0;
if (idx17_2 >= 0) {
    arr17_2[idx17_2-- as int]; // unsafe:
}

// 17.3 if (idx >= 1) { arr[--idx]; } - unsafe (missing upper bound check)
let arr17_3: number[] = [1, 2, 3, 4, 5];
let idx17_3: number = 3;
if (idx17_3 >= 1) {
    arr17_3[--idx17_3 as int]; // unsafe:
}

// ============================================================================
// Table 18: Direct Access Without Check (Negative)
// ============================================================================

// 18.1 arr[10] - unsafe (no length check)
let arr19_1: number[] = [1, 2, 3];
arr19_1[10]; // unsafe:

// 18.2 arr[index] - unsafe
let arr19_2: number[] = [1, 2, 3];
let idx19_2: number = 10;
arr19_2[idx19_2]; // unsafe:

// 18.3 arr[3 * 5] - unsafe
let arr19_3: number[] = [1, 2, 3];
arr19_3[3 * 5]; // unsafe:

// 18.4 arr[getIndex()] - unsafe
let arr19_4: number[] = [1, 2, 3];
function getIndex(): number {
    return 10;
}
arr19_4[getIndex() as int]; // unsafe:

// ============================================================================
// Table 19: Access Outside Loop (Negative)
// ============================================================================

// 19.1 for (let i = 0; i < arr.length; i++) {} arr[i] - unsafe
let arr20_1: number[] = [1, 2, 3];
for (let i = 0; i < arr20_1.length; i++) {
    // loop body
}
arr20_1[5]; // unsafe:

// 19.2 while (i < arr.length) { i++; } arr[i] - unsafe
let arr20_2: number[] = [1, 2, 3];
let i20_2: number = 0;
while (i20_2 < arr20_2.length) {
    i20_2++;
}
arr20_2[i20_2]; // unsafe:

// 19.3 if (idx < arr.length) {} arr[idx] - unsafe
let arr20_3: number[] = [1, 2, 3];
let idx20_3: number = 1;
if (idx20_3 < arr20_3.length) {
    // if body
}
arr20_3[idx20_3]; // unsafe:

// ============================================================================
// Table 20: Cross-Array Misuse (Negative)
// ============================================================================

// 20.1 for (let i = 0; i < arrA.length; i++) { arrB[i] } - unsafe
let arr21_A: number[] = [1, 2, 3, 4, 5];
let arr21_B: number[] = [1, 2];
for (let i = 0; i < arr21_A.length; i++) {
    arr21_B[i]; // unsafe:
}

// 20.2 if (arrA.length > 10) { arrB[10] } - unsafe
let arr21_C: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
let arr21_D: number[] = [1, 2, 3];
if (arr21_C.length > 10) {
    arr21_D[10]; // unsafe:
}

// ============================================================================
// Table 21: Expression Index (Negative)
// ============================================================================

// 21.1 arr[idx + 1] - unsafe
let arr22_1: number[] = [1, 2, 3];
let idx22_1: number = 2;
if (idx22_1 < arr22_1.length) {
    arr22_1[(idx22_1 + 1) as int]; // unsafe:
}

// 21.2 arr[idx << 1] - unsafe
let arr22_2: number[] = [1, 2, 3];
let idx22_2: number = 2;
if (idx22_2 < arr22_2.length) {
    arr22_2[(idx22_2 << 1) as int]; // unsafe:
}

// 21.3 arr[idx & 0xFF] - unsafe
let arr22_3: number[] = [1, 2, 3];
let idx22_3: number = 2;
if (idx22_3 < arr22_3.length) {
    arr22_3[(idx22_3 & 0xFF) as int]; // unsafe:
}

// 21.4 arr[idx || 0] - unsafe
let arr22_4: number[] = [1, 2, 3];
let idx22_4: number = 2;
arr22_4[(idx22_4 || 0) as int]; // unsafe:

// 21.5 arr[idx ?? 0] - unsafe
let arr22_5: number[] = [1, 2, 3];
let idx22_5: number = 2;
arr22_5[(idx22_5 ?? 0) as int]; // unsafe:

// ============================================================================
// Table 22: Complex Scenarios (Negative)
// ============================================================================

// 22.1 Two-dimensional array arr[m][n] - unsafe
let arr23_2D: number[][] = [[1, 2], [3, 4]];
let m23_1: number = 0;
let n23_1: number = 0;
arr23_2D[m23_1][n23_1]; // unsafe:

// 22.2 Nested arr[arr[idx]] - unsafe
let arr23_nested: number[] = [1, 2, 3];
let idx23_nested: number = 0;
arr23_nested[arr23_nested[idx23_nested] as int]; // unsafe:

// 22.3 Class method this.arr[10] - unsafe
class Class23_3 {
    arr: number[] = [1, 2, 3];
    method() {
        this.arr[10]; // unsafe:
    }
}

// 22.4 Callback function { arr[10] } - unsafe
let arr23_4: number[] = [1, 2, 3];
function process23_4(callback: () => void) {
    callback();
}
process23_4(() => {
    arr23_4[10]; // unsafe:
});

// ============================================================================
// Table 23: Special Boundary Values (Negative)
// ============================================================================

// 23.1 arr[-1] - unsafe
let arr24_1: number[] = [1, 2, 3];
arr24_1[-1]; // unsafe:

// 23.2 arr[Number.MAX_VALUE] - unsafe
let arr24_2: number[] = [1, 2, 3];
arr24_2[Number.MAX_VALUE as int]; // unsafe:

// 23.3 arr[NaN] - unsafe
let arr24_3: number[] = [1, 2, 3];
arr24_3[NaN as int]; // unsafe:

// ============================================================================
// Table 24: Empty Array (Negative)
// ============================================================================

// 24.1 Empty array arr[0] - unsafe
let arr25_1: number[] = [];
arr25_1[0]; // unsafe:

// 24.2 if (arr.length > 0) { } arr[0] - unsafe (access outside if block)
let arr25_2: number[] = [];
if (arr25_2.length > 0) {
    // empty block
}
arr25_2[0]; // unsafe:

// ============================================================================
// Table 25: Variable Modified After Condition (Negative)
// ============================================================================

// 25.1 if (idx < arr.length) { idx = 100; arr[idx]; } - unsafe
let arr26_1: number[] = [1, 2, 3, 4, 5];
let idx26_1: number = 2;
if (idx26_1 < arr26_1.length) {
    idx26_1 = 100;
    arr26_1[idx26_1]; // unsafe:
}

// 25.2 for (let i = 0; i < arr.length; i++) { i = 10; arr[i]; } - unsafe
let arr26_2: number[] = [1, 2, 3];
for (let i = 0; i < arr26_2.length; i++) {
    i = 10;
    arr26_2[i]; // unsafe:
}

// ============================================================================
// Table 26: Multi-Array Condition Confusion (Negative)
// ============================================================================

// 26.1 if (arrA.length > 10 && arrB.length > 5) { arrB[10] } - unsafe
let arr27_A: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
let arr27_B: number[] = [1, 2, 3];
if (arr27_A.length > 10 && arr27_B.length > 5) {
    arr27_B[10]; // unsafe:
}

// 26.2 if (arrA.length > 10 || arrB.length > 10) { arrA[10] } - unsafe (OR gives no guarantee)
let arr27_C: number[] = [1, 2, 3];
let arr27_D: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11];
if (arr27_C.length > 10 || arr27_D.length > 10) {
    arr27_C[10]; // unsafe:
}