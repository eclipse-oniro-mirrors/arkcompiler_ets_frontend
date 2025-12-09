/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

function foo1(f: (a1: string, ...args: string[]) => string) {
    let fa = f("a")
    arktest.assertEQ(fa, "a-undefined-undefined")
    let fab = f("a", "b")
    arktest.assertEQ(fab, "a-b-undefined")
    let fabc = f("a", "b", "c")
    arktest.assertEQ(fabc, "a-b-c")
    let fabcd = f("a", "b", "c", "d")
    arktest.assertEQ(fabcd, "a-b-c-d")
    let fabcde = f("a", "b", "c", "d", "e")
    arktest.assertEQ(fabcde, "a-b-c-d")
}

function foo2(f: (a1: string, ...args: string[]) => string) {
    let fa = f("a")
    arktest.assertEQ(fa, "a-undefined")
    let fab = f("a", "b")
    arktest.assertEQ(fab, "a-b")
    let fabc = f("a", "b", "c")
    arktest.assertEQ(fabc, "a-b")
}

function foo3(f: (a1: string, ...args: string[]) => string) {
    let fa = f("a")
    arktest.assertEQ(fa, "a-undefined-undefined-undefined")
    let fab = f("a", "b")
    arktest.assertEQ(fab, "a-b-undefined-undefined")
    let fabc = f("a", "b", "c")
    arktest.assertEQ(fabc, "a-b-c-undefined")
    let fabcd = f("a", "b", "c", "d")
    arktest.assertEQ(fabcd, "a-b-c-d")
    let fabcde = f("a", "b", "c", "d", "e")
    arktest.assertEQ(fabcde, "a-b-c-d")
}

function main(): void {
    let f1 = ((a: string, b?: string, c?: string, ...arg1: string[]): string => {
        let res: string = a + "-" + b + "-" + c;
        if(arg1.length != 0) {
            res += "-" + arg1[0]
        }
        return res
    }) as Function
    foo1(f1 as (a1: string, ...args: string[]) => string)

    let f2 = ((a: string, b?: string): string => {
        return a + "-" + b;
    }) as Function
    foo2(f2 as (a1: string, ...args: string[]) => string)

    let f3 = ((a: string, b?: string, c?: string, d?: string): string => {
        return a + "-" + b + "-" + c + "-" + d;
    }) as Function
    foo3(f3 as (a1: string, ...args: string[]) => string)
}
