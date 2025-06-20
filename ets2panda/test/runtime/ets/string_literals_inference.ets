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

function foo(x: "a"|"b"|"c") {
    x = "c"
    return x
}

function id<T>(v: Object): T {
    return v as T
}

function getColor(colors: Array<"default"|"invisible"|number>, id: number) {
    return colors[id]
}

function f1(a: "xyz"): "xyz" {
    return a
}

function f2(a: "aa"|"bb"): "aa"|"bb" {
    return a
}

function f3(a: "aa"|"bb") {
    return a + "cc"
}

function f4(a: "b"): "b" {
    return a
}

class A {
    p: "aa"|"bb" = "bb"
}

function test_inference_from_return() {
    let arr = new Array<"default"|"invisible"|number>(3);
    arr[0] = "default"
    arr[1] = 42
    arr[2] = "invisible"
    let s = getColor(arr, 0)   // s is of type string|number
    assertEQ(s, "default")
    s = "dd"
}

function test_inference_from_conditional(cond: boolean) {
    let x1 = cond ? "aa" : "bb"  // type of x1 is string
    assertEQ(x1, "aa")
    x1 = "abc"
    assertEQ(x1, "abc")
    let x2: "yes"|"no" = cond ? "yes" : "no"
    assertEQ(x2, "yes")
    let x3 = cond ? "one" : new A  // type of x3 is string|A
    assertEQ(x3, "one")
    x3 = "bb"
    assertEQ(x3, "bb")

    const x4 = cond ? "aa" : "bb"  // type of x4 is "aa"|"bb"
    let y4 = f2(x4)  // type of y4 is "aa"|"bb"
    let z4 = f2(y4)
    assertEQ(z4, "aa")

    let x5 = ((p: boolean) => p ? "bb" : "aa")(cond)  // type of x5 is string
    assertEQ(x5, "bb")
    x5 = "xyz"
    assertEQ(x5, "xyz")
}

function main() {
    const x1 = "xyz"
    let y1 = f1(x1)
    assertEQ(y1, "xyz")

    let s = id<"a"|"b"|"c">("b")  // type of s is "a"|"b"|"c"
    assertEQ(s, "b")
    assertNE(s, "a")
    assertEQ(foo(s), "c")
    let y = s as "b"
    assertEQ(y, "b")
    assertEQ(f4(s as "b"), "b")

    test_inference_from_conditional(true)
    test_inference_from_return()

    assertEQ(f3("aa"), "aacc")

    let a = new A
    let x2 = a.p
    assertEQ(f2(x2), "bb")
    let z = x2
    assertEQ(f2(z), "bb")

    let x3: ("aa"|"bb")[] = ["aa", "bb", "aa"]
    let y3 = f2(x3[0])  // type of y3 is "aa"|"bb"
    assertEQ(y3, "aa")
    assertEQ(x3[1], "bb")

    let x4 = ["aa", "bb", "aa", 43]  // type of x4 is (string|int)[]
    x4[0] = "cc"
    assertEQ(x4[0], "cc")
    assertEQ(x4[1], "bb")
    x4[2] = 55
    x4[3] = "xyz"
    assertEQ(x4[2], 55)
    assertEQ(x4[3], "xyz")
}
