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

class A5 extends Object { }
class A4 extends A5 { }
class A3 extends A4 { }
class A2 extends A3 { }
class A1 extends A2 { }

function n(a: Object) {
    if (a instanceof A1) return 1;
    if (a instanceof A2) return 2;
    if (a instanceof A3) return 3;
    if (a instanceof A4) return 4;
    if (a instanceof A5) return 5;
    return new Error("Not found");
}

const a5 = new A5();
const a4 = new A4();
const a3 = new A3();
const a2 = new A2();
const a1 = new A1();

function expect(exp: int[], ...res: int[]) {
    for (let i = 0; i < exp.length; i++) {
        if (exp[i] != res[i]) {
            arktest.assertTrue(exp[i] == res[i]);
        }
    }
}

namespace Test1 {
    class C {
        f(a: A1) { return 1 }
        f(a: A2) { return 2 }
        f(a: A3) { return 3 }
    }
    const c = new C();
    const expected: int[] = [1, 2, 3]
    export function r() {
        expect(expected, c.f(a1), c.f(a2), c.f(a3))
    }
}

namespace Test2 {
    class C {
        f(a: A1) { return 1 }
        f(a: A3) { return 2 }
        f(a: A2) { return 3 }
    }
    const c = new C();
    const expected: int[] = [1, 2, 2]
    export function r() {
        expect(expected, c.f(a1), c.f(a2), c.f(a3))
    }
}

Test2.r()

namespace Test3 {
    class C1 {
        f(a: A2) { return 3 }
    }
    class C2 extends C1 {
        f(a: A1) { return 1 }
        f(a: A3) { return 2 }
    }
    const c = new C2();
    const expected: int[] = [1, 2, 2]
    export function r() {
        expect(expected, c.f(a1), c.f(a2), c.f(a3))
    }
}

Test1.r()
Test2.r()
Test3.r()