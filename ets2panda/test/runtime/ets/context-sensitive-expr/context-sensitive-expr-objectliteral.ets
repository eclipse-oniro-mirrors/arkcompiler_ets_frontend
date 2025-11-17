/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an 'AS IS' BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Test case 1, class literal assignment test between class and Record
class C {
    v1: int = 0
    v2: string = ''
}
type U = C | Record<string, string>;
function test1() {
    let a: U = {'v1': 0, 'v2': 'qqq'} // deduce to C, but should throw warning here: key of class literal should be identifier.
    let b: U = {v1: 0, v2: 'qqq'} // deduce to C.
    let c: U = {'v2': 'zzz', 'v3': 'sss'} // deduce to Record, C has no property v3.

    arktest.assertTrue(a instanceof C)
    arktest.assertTrue(b instanceof C)
    arktest.assertTrue(c instanceof Record)
}

// Test case 2, class literal assignment test between base-derived class
class B {
    b: int
}

class D extends B {
    d: string | undefined
}

function test2() {
    let x: B | D = {b: 3, d: 'aa'} // deduce to D
    let y: B | D = {b: 4}  // deduce to B

    let catchMsg: string = 'no error'
    try {
        (y as D).d
    } catch(e) {
        catchMsg = e.message
    }

    arktest.assertEQ(catchMsg, 'context-sensitive-expr-objectliteral.B cannot be cast to context-sensitive-expr-objectliteral.D')
}

// Test case 3, class literal assignment test between class with the same name field.
class C3 {
    z: object = "1.2.string"
}

class D3 {
    z: number
}

class Z3 {
    foo(): object { return 1.1};
}
function test3() {
    let x: C3 | D3 = {z: new Z3().foo()} // should deduce to `C3`, return type of `foo` is object.
    arktest.assertEQ((x as C3).z, 1.1)
}

// Test case 4, objectliteral assign to Record union.
type UT = Record<number, number> | Record<string, number>
function test4() {
    let a: UT = { "1": 2 }
    let b: UT = { 1: 1 }
    let c: Record<number, number> | Record<string, number> = {"2": 2.1}
    let d: Record<number, number> | Record<string, number> = {2.1: 2.1}
    arktest.assertEQ(a["1"], 2)
    arktest.assertEQ(b[1], 1)
    arktest.assertEQ(c["2"], 2.1)
    arktest.assertEQ(d[2.1], 2.1)
}

test1();
test2();
test3();
test4();