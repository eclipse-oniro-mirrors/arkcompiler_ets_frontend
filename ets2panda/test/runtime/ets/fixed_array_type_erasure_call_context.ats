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

// The type erasure check for FixedArray element types should NOT fire when
// the array literal appears inside a function call argument chain, because
// the preferred type is cascaded from the parameter type rather than being
// an explicit user annotation.

type AnyType = Object | (() => void)

function expectType(a?: AnyType): AnyType {
    return a ?? new Object
}

function main(): void {
    // Case 1: array literal as base of indexing chain inside a call.
    // The preferred type is cascaded from the expectType parameter.
    let r1 = expectType([["a"]][0][0])
    arktest.assertTrue(r1 instanceof String)

    // Case 2: simple 1D array as indexing base inside a call.
    let r2 = expectType(["x"][0])
    arktest.assertTrue(r2 instanceof String)

    // Case 3: direct FixedArray creation with preserved types still works.
    let fa: FixedArray<int> = [1, 2, 3]
    arktest.assertEQ(fa[0], 1)

    // Case 4: 2D FixedArray with preserved element type still works.
    let fa2: FixedArray<FixedArray<string>> = [["a", "b"], ["c"]]
    arktest.assertEQ(fa2[0][0], "a")

    // Case 5: array literal as direct argument to a generic function call
    // (e.g. Promise.race<number>([...])).  The FixedArray preferred type
    // comes from the generic parameter which was validated at its declaration
    // site — the type erasure check should NOT fire here.
    let p = Promise.race<number>([
        new Promise<number>((resolve): void => { resolve(100) }), 7.0
    ])
    arktest.assertTrue(p instanceof Promise)

    // Case 6: Readonly<T> and Awaited<T> are simplified by the compiler to
    // their inner types before the type erasure check runs, so they should
    // compile without errors.
    let r: FixedArray<Readonly<string>> = ["x"]
    arktest.assertEQ(r[0], "x")

    let a: FixedArray<Awaited<number>> = [1.0]
    arktest.assertEQ(a[0], 1.0)
}
