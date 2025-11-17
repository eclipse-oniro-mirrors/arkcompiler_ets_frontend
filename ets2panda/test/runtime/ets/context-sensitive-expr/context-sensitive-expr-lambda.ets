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

// union infer in variable assign.
type U = ((x: int) => int) | ((x: string, y: int) => int) | ((x: int) => string)
let f1: U = (x) => {return x + 1};
let f2: U = (x) => {return x.length}
let f3: U = (x) => {return "aaa"}
let f4: U = (x) => {return 33}
let f5: U = (x, y) => {return x.length + y}

arktest.assertEQ((f1 as ((x: int) => int))(1), 2);
arktest.assertEQ((f2 as ((x: string, y: int) => int))("aaa", 0), 3);
arktest.assertEQ((f3 as ((x: int) => string))(0), "aaa");
arktest.assertEQ((f4 as ((x: int) => int))(0), 33) // type first match
arktest.assertEQ((f5 as ((x: string, y: int) => int))("aaa", 3), 6)

// union infer in function call.
class A {
    prop:string = "a-prop"
}
type U2 = (() => string) | ((x: string) => string) | ((x: string, y: int) => string) | (((x: string, y: int, z: A) => string))
function test(cb: U2) {
    let callRes = (cb as ((x: string, y: int, z: A) => string))("args1", 0, new A());
    if (cb instanceof Function0) {
        arktest.assertEQ(callRes, "empty args")
    } else if (cb instanceof Function1) {
        arktest.assertEQ(callRes, "args1")
    } else if (cb instanceof Function2) {
        arktest.assertEQ(callRes, "two args: args1, 0")
    } else if (cb instanceof Function3) {
        arktest.assertEQ(callRes, "three args: args1, 0, a-prop")
    }
}

test(() => {return "empty args"});
test((x) => {return x})
test((x, y) => {return "two args: " + `${x}, ` + `${y}`})
test((x, y, z) => {return "three args: " + `${x}, ` + `${y}, ` + `${z.prop}`})
