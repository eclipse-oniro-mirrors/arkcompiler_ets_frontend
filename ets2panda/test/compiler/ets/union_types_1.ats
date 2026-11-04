/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

class A {
    num: int = 41;
}

class B {
    num: int = 42;
}

class C {
    num: int = 43;
    constructor(num: int) {
        this.num = num;
    }
    num_to_return() : int {
        return this.num;
    }
}

function foo(x : A | B | C) {
    if (x instanceof C) {
        let xx = x as C;
        arktest.assertEQ(xx.num_to_return(), 777, "Error! The num field of class `C` must be 777");
    }
    if (x instanceof A) {
        arktest.assertTrue(false, "Error! x is instaceof C but not A");
    }
    arktest.assertEQ(x.num, 777, "Error! The num field of union must be 777");
    let a: int = x.num + 3;
    arktest.assertEQ(a, 780, "Error! Variable 'a' must be 780");
    x.num += 223;
    arktest.assertEQ(x.num, 1000, "Error! The num field of union must be 1000");
}

function main() {
    let x : A | B | C;
    x = new C(777);
    foo(x);
    arktest.assertEQ(x.num, 1000, "Error! The num field of union must be 1000");
    if (x instanceof C) {
        let xx = x as C;
        arktest.assertEQ(xx.num_to_return(), 1000, "Error! The num field of class `C` must be 1000");
    }
    if (x instanceof B) {
        arktest.assertTrue(false, "Error! x is instaceof C but not B");
    }
    x = new A();
    arktest.assertEQ(x.num, 41, "Error! The num field of union must be 41");
    x = new B();
    arktest.assertEQ(x.num, 42, "Error! The num field of union must be 42");
}

