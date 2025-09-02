/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

class Base {
    f: int
}

class InnerValue {
    v: int
}

class C extends Base {
    x: Int = 5
    s: String = ""
    iv: InnerValue = {}
}

function returnC(): C {
    return {x: 99, f: 44, s: "qq", iv: {v: 77}} // return statement
}

function test(c: int, f: int, x: int, s: String, ivv: int) {} // should not prevent calling the next fun
function test(c: C, f: int, x: int, s: String, ivv: int) {
    arktest.assertEQ(c.f, f)
    arktest.assertEQ(c.x, x)
    arktest.assertEQ(c.s, s)
    arktest.assertEQ(c.iv.v, ivv)
}

function main(): int {
    let c: C = { // variable definition
        "x": 7,
        s: "sss",
    };
    test(c, 0, 7, "sss", 0)

    let c2 = { // as construction
        f: 4,
        s: "qq"
    } as C;
    test(c2, 4, 5, "qq", 0)

    c = {f: 5, s: "zzz"} // assignment
    test(c, 5, 5, "zzz", 0)

    test({  // function argument
        f: 3,
        x: 8,
        s: "uhuh",
        iv: {   // object literal field
            v: 55
        }}, 3, 8, "uhuh", 55)

    test(returnC(), 44, 99, "qq", 77)

    let ca: C[] = [{f: 42, s: "first"}, {f: 128, s: "second"}] // array elements
    test(ca[1], 128, 5, "second", 0)

    return 0
}
