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

type T = (int[] | (int | int[])[])[]

let topLevel = [
    [1, 2],
    [1, [1, 2]]
]

const topLevelConst = [
    [3, 4],
    [5, [6, 7]]
]

function buildInFunction(): T {
    let local = [
        [8, 9],
        [10, [11, 12]]
    ]
    return local
}

function assertNestedArrayShape(arr: T, first0: int, first1: int, second0: int, nested0: int, nested1: int): void {
    arktest.assertEQ(arr.length, 2)

    let first = arr[0] as int[]
    arktest.assertEQ(first.length, 2)
    arktest.assertEQ(first[0], first0)
    arktest.assertEQ(first[1], first1)

    let second = arr[1] as (int | int[])[]
    arktest.assertEQ(second.length, 2)
    arktest.assertEQ(second[0] as int, second0)
    arktest.assertTrue(second[1] instanceof Array)

    let nested = second[1] as int[]
    arktest.assertEQ(nested.length, 2)
    arktest.assertEQ(nested[0], nested0)
    arktest.assertEQ(nested[1], nested1)
}

function main() {
    assertNestedArrayShape(topLevel, 1, 2, 1, 1, 2)
    assertNestedArrayShape(topLevelConst, 3, 4, 5, 6, 7)

    let local = buildInFunction()
    assertNestedArrayShape(local, 8, 9, 10, 11, 12)
}
