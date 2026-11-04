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
    public readonly a: int;

    constructor() {
        this.a = 42;
    }
}

final class B extends A {

}

final class C extends A {
    constructor() {

    }
}

final class D extends A {
    constructor() {
        super();
    }
}


function main(): void {
    const a: A = new A();
    arktest.assertEQ(a.a, 42)

    const b: B = new B();
    arktest.assertEQ(b.a, 42)

    const c: C = new C();
    arktest.assertEQ(c.a, 42)

    const d: D = new D();
    arktest.assertEQ(d.a, 42)

    let d_as_A: A = d;
    arktest.assertEQ(d_as_A.a, 42)
}
