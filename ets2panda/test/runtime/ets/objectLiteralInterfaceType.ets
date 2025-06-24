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

interface Base {
    f: int
}

interface InnerValue {
    v: int
}

interface I extends Base {
    x: Int
    s: String
    iv: InnerValue
}

function returnI(): I {
    return {x: 99, f: 44, s: "sR", iv: {v: 77}} // return statement
}

function test(i: int, f: int, x: int, s: String, ivv: int) {} // should not prevent calling the next func

function test(i: I = {f: -1, x: -2, s: "default", iv: {v: -3}},
              f: int = -1,
              x: int = -2,
              s: String = "default",
              ivv: int = -3) {
    arktest.assertEQ(i.f, f, "\"f\" is not equal \"" + i.f + "\" != \"" + f + "\"")
    arktest.assertEQ(i.x, x, "\"x\" is not equal \"" + i.x + "\" != \"" + x + "\"")
    arktest.assertEQ(i.s, s, "\"s\" is not equal \"" + i.s + "\" != \"" + s + "\"")
    arktest.assertEQ(i.iv.v, ivv, "\"innervalue.v\" is not equal \"" + i.iv.v + "\" != \"" + ivv + "\"")
}

function main(): int {
    let i: I = { // variable definition
        f : 1,
        x: 2,
        s: "s1",
        iv: { v: 3}
    };
    test(i, 1, 2, "s1", 3)

    let i2 = { // as construction
        f: 4,
        x: 5,
        s: "s2",
        iv: { v: 6}
    } as I;
    test(i2, 4, 5, "s2", 6)

    i = {f: 7, x: 8, s: "s3", iv : { v: 9}} // assignment
    test(i, 7, 8, "s3", 9)

    test({  // function argument
        f: 10,
        x: 11,
        s: "s3",
        iv: {
            v: 12
        }}, 10, 11, "s3", 12)

    test(returnI(), 44, 99, "sR", 77)

    let ia: I[] = [{f: 20, x: 21, s: "first", iv: {v: 22}}, {f: 30, x: 31, s: "second", iv: {v: 32}}] // array elements
    test(ia[1], 30, 31, "second", 32)

    test() // optional parameter

    return 0
}
