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

/**
 * Test array literal context matching for Array supertypes.
 * Spec: Array literals should support context matching for:
 * - Any, Object
 * - Array<T>
 * - Array<T> supertypes: ConcatArray<T>, ReadonlyArray<T>, ArrayLike<T>
 * - FixedArray<T>
 * - Tuples
 * - Union types containing the above
 */

function main() {
    // Test 1: Array<int>
    let a: Array<int> = [1, 2, 3] as Array<int>
    arktest.assertEQ(a.length, 3)
    arktest.assertEQ(a[2], 3)
    
    // Test 2: Array<double> (number)
    let b: Array<double> = [1, 2, 3] as Array<double>
    arktest.assertEQ(b.length, 3)
    arktest.assertEQ(b[2], 3)
    
    // Test 3: ReadonlyArray<int>
    let c: ReadonlyArray<int> = [1, 2, 3] as ReadonlyArray<int>
    arktest.assertEQ(c.length, 3)
    arktest.assertEQ(c[2], 3)
    
    // Test 4: ConcatArray<int>
    let d: ConcatArray<int> = [1, 2, 3] as ConcatArray<int>
    arktest.assertEQ(d.length, 3)
    arktest.assertEQ(d[2], 3)
    
    // Test 5: ConcatArray<double> (number)
    let e: ConcatArray<number> = [1, 2, 3] as ConcatArray<number>
    arktest.assertEQ(e.length, 3)
    arktest.assertEQ(e[2], 3)
    
    // Test 6: ArrayLike<int>
    let f: ArrayLike<int> = [1, 2, 3] as ArrayLike<int>
    arktest.assertEQ(f.length, 3)
    arktest.assertEQ(f[2], 3)
    
    // Test 7: Array in variable declaration with type annotation
    const arr1: Array<number> = [1, 2, 3]
    arktest.assertEQ(arr1.length, 3)
    arktest.assertEQ(arr1[2], 3)
    
    // Test 8: ConcatArray in variable declaration
    const arr2: ConcatArray<number> = [2, 4]
    arktest.assertEQ(arr2.length, 2)
    arktest.assertEQ(arr2[1], 4)
    
    // Test 9: concat method with ConcatArray argument
    const base: Array<number> = [1, 2, 3]
    const toConcat: ConcatArray<number> = [4, 5]
    const result = base.concat(toConcat)
    arktest.assertEQ(result.length, 5)
    arktest.assertEQ(result[4], 5)
    
    // Test 10: concat with inline ConcatArray
    const result2 = base.concat([6, 7] as ConcatArray<number>)
    arktest.assertEQ(result2.length, 5)
    arktest.assertEQ(result2[4], 7)
}
