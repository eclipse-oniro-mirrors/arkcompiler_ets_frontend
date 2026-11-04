/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

function foo1 (tuple: readonly [number, string, boolean], arr: ReadonlyArray<int>) {
    foo2(tuple, arr)
    let x: readonly [number, string, boolean] = tuple
    let y: readonly int[] = arr
}

function foo2 (tuple: readonly [number, string, boolean], arr: readonly int[]) {
    foo3(tuple, arr)
    let x: readonly [number, string, boolean] = tuple
    let y: ReadonlyArray<int> = arr
}

function foo3 (tuple: readonly [number, string, boolean], arr: ReadonlyArray<int>) {
    let x: readonly [number, string, boolean] = tuple
    let y: readonly int[] = arr
}

function main() : void {
    foo1([1, "ab", true], [1, 2])
    foo2([1, "ab", true], [1, 2])

    let tuple1: readonly [int, boolean] = [1, true]
    let arr1: ReadonlyArray<int> = []
    let tuple2: readonly [int, boolean] = tuple1
    let arr2: readonly int[] = arr1

    arr1 = arr2
    tuple1 = tuple2
}
