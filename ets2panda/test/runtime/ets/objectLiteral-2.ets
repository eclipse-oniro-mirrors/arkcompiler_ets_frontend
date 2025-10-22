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

class InnerValue {
    private _v: int = 0
    get v(): int { return this._v; }
    set v(v: int){ this._v = v; }
}

class C {
    private _x: int = 0
    get x(): int { return this._x; }
    set x(x: int){ this._x = x; }

    private _iv: InnerValue = {v: 0}
    get iv(): InnerValue { return this._iv; }
    set iv(iv:InnerValue){ this._iv = iv; }
}

function returnC(): C {
    return {x: 99, iv: {v: 77}} // return statement
}

function test(c: C = {}, x: int = 0, ivv: int = 0) {
    arktest.assertEQ(c.x, x, "c.x != x")
    arktest.assertEQ(c.iv.v, ivv, "c.iv.v != ivv")
}

function main(): int {
    let c: C = {x: 7, iv: {v: 8}}; // variable definition
    test(c, 7, 8)

    test() // optional parameter

    let c2 = { // as construction
        x: 4,
        iv: {v: 5}
    } as C;
    test(c2, 4, 5)

    c = {x: 5, iv: {v: 6}} // assignment
    test(c, 5, 6)

    test({  // function argument
        x: 3,
        }, 3, 0)

    test(returnC(), 99, 77)

    let ca: C[] = [{x: 42, iv: {v: 1}}, {x: 128, iv: {v: 2}}] // array elements
    test(ca[1], 128, 2)
    return 0;
}
