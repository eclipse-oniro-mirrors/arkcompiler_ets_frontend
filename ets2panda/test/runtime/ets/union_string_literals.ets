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

type UT1 = "abc"|"bcd"|"cde"
let a: UT1 = "bcd"

function foo<T extends "abc"|"ff">(a: T): string {
    let b: "abc"|"ff" = a
    arktest.assertEQ(b, "abc")
    b = "ff"
    return b
}

class A{}

function bar(x: "abc"|"bcd"|A|int) {
    return x
}

function baz(x: "ab1"|"bcd"|A|int): "ab1"|string|A|int {
    return x
}

let map: Record<"aa" | "bb" | int, number> = {
    "aa": 1,
    "bb" : 3,
    42 : 33,
}

function f1(x: number|string, y: boolean|"abc"): boolean {
    return x == y
}

function f2(x: number|string, y: string): boolean {
    return x == y
}

// function f3(a: "aa"|"bb"|"cc" = "bb") { // #22952
//     return a;
// }

function f4(a: (p: string) => "aa"|"bb",
              b: (p: "aa"|"bb") => string) {
    b = a
    b(a("aa"))
}

function f5(x: "aa"|"bbb") {
    return x.length
}

function main(): void {
    arktest.assertEQ(foo<"abc">("abc"), "ff")
    arktest.assertEQ(foo<"ff"|"abc">("abc"), "ff")

    arktest.assertEQ(a, "bcd")

    arktest.assertEQ(bar("abc"), "abc")
    arktest.assertEQ(bar(42), 42)

    let x = baz("bcd")
    x = "some string"
    arktest.assertEQ(x, "some string")
    arktest.assertEQ(baz("ab1"), "ab1")
    x = "abc"

    arktest.assertTrue(f1("abc", x))
    arktest.assertTrue(f2("abc", x))
    arktest.assertTrue(!f2("abc", "xyz"))

    // #22952
    // arktest.assertEQ(f3(), "bb")
    // arktest.assertEQ(f3("aa"), "aa")

    arktest.assertEQ(f5("aa"), 2)
    arktest.assertEQ(f5("bbb"), 3)
}
