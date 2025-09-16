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

class A {
    foo(): int {
        return 1;
    }

    final baz(): int {
        return this.foo(); // virtual call expected
    }
}

final class B extends A {
    override foo(): int {
        return 2;
    }

    bar(): int {
        return super.foo(); // static call expected
    }

    virt(): int {
        return super.baz();
    }
}

class F {
    public readonly f: int;

    constructor() {
        this.f = 42;
    }
}

final class G extends F {

}

final class H extends F {
    constructor() {

    }
}

final class I extends F {
    constructor() {
        super();
    }
}

function main(): void {
    let bClass: B = new B();

    let a: int = bClass.foo();
    arktest.assertEQ(a, 2)

    let b: int = bClass.bar();
    arktest.assertEQ(b, 1)

    let c: int = bClass.virt();
    arktest.assertEQ(c, 2)

    const f: F = new F();
    arktest.assertEQ(f.f, 42)

    const g: G = new G();
    arktest.assertEQ(g.f, 42)

    const h: H = new H();
    arktest.assertEQ(h.f, 42)

    const i: I = new I();
    arktest.assertEQ(i.f, 42)

    let d_as_A: F = i;
    arktest.assertEQ(d_as_A.f, 42)
}
