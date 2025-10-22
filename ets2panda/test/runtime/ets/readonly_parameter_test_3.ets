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

function foo1 (tuple: readonly [number, string, boolean], arr: Readonly<number[]>) {
    let x: readonly [number, string] = [1, "a"]
    let x2: readonly [string, double]
    let y: Readonly<int[]> = []
    let y2: Readonly<string[]>
}

function foo2 (px: Readonly<int[]>, py: readonly [string, float]) {
    let x2 : Readonly<int[]> = px
    let y2 : readonly [string, float] = py
}

function foo3 (x: Readonly<int[]>, x2: int[]) {
    x = [1, 2]
    x = new int[10]
    let y: int[] = []
    x = y
    x = x2
}

function foo4 (x: readonly [int, string], x2: [int, string]) {
    x = [1, "abc"]
    let y : [int, string] = [1, "abc"]
    x = y
    x = x2
}

function main() : void {
    foo1([1, "ab", true], [1, 2])

    let x2: Readonly<int[]> = []
    let y2: readonly [string, float] = ["a", Double.toFloat(0.1)]
    foo2(x2, y2)

    let x : Readonly<int[]> = []
    x = [1, 2]
    let y : readonly [int, string] = [1, "abc"]
    y = [10, "cde"]
    foo3([1, 2], [1, 2])
    foo4([1, "xx"], [1, "xx"])
}