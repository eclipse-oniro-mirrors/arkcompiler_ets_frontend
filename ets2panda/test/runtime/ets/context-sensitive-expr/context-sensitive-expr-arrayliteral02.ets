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

// arrayliteral as function params
function foo1(a: readonly [int, string] | [int, string] | never) {
    return a[0]
}

function foo2(a: readonly number[] | number[] | never): number {
    return a[1]
}

function foo3(a: ReadonlyArray<string> | string[] | never): string {
    return a[0]
}

function testMultipleNever(a: number[] | readonly number[] | never | never | never): number {
    return a[0]
}

function main() {
    arktest.assertEQ(foo1([1, "2"]), 1)
    arktest.assertEQ(foo2([1, 2] as number[]), 2)
    arktest.assertEQ(foo3(["hello", "world"]), "hello")

    let readonlyArr1: readonly number[] = [1, 2, 3]
    let mutableArr: number[] = [4, 5, 6]

    arktest.assertEQ(foo2(readonlyArr1), 2)
    arktest.assertEQ(foo2(mutableArr), 5)
    arktest.assertEQ(testMultipleNever(mutableArr), 4)
}