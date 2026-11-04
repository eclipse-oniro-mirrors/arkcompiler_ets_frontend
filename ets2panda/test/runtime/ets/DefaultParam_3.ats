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

class MyType {
    x: int = 10;
    constructor() { }
    constructor(a: int) {
        this.x = a;
    }
}

function main(): void {
    arktest.assertEQ(foo1(), 8)

    arktest.assertEQ(foo2(), 30)
    arktest.assertEQ(foo2(new MyType(5)), 25)

    arktest.assertEQ(foo3(new MyType(5)), 55)
    arktest.assertEQ(foo3(new MyType(5), new MyType(10) ), 45)

    arktest.assertEQ(foo4(), 0)

    arktest.assertEQ(foo5(), 0)
    arktest.assertEQ(foo5(new MyType(5)), -1)

    arktest.assertEQ(foo6(new MyType(5)), 0)
    arktest.assertEQ(foo6(new MyType(5), new MyType(10)), -1)

    arktest.assertEQ(foo7(), 0)
    arktest.assertEQ(foo8(), 0)

    arktest.assertEQ(foo9(new MyType(10)), 15)
    arktest.assertEQ(foo10(new MyType(10)), 25)

    arktest.assertEQ(foo11(new MyType(10), new MyType(5)), 20)
    arktest.assertEQ(foo12(new MyType(10), new MyType(5)), 30)
}

function foo1(a: MyType|null = new MyType(8)): int {
    if (a == null) {
        return -1;
    }
    return a.x;
}

function foo2(a: MyType = new MyType(10), b: MyType = new MyType(20)): int {
    return a.x + b.x;
}

function foo3(a: MyType = new MyType(10), b: MyType = new MyType(20), c: MyType = new MyType(30)): int {
    arktest.assertEQ(a.x, 5)

    return a.x + b.x + c.x;
}

function foo4(a?: MyType): int {
    if (a == null) {
        return 0;
    }
    return a.x;
}

function foo5(a?: MyType, b?: MyType): int {
    if (a == null && b == null) {
        return 0;
    }
    if (b == null) {
        return -1;
    }
    return a!.x + b.x;
}

function foo6(a?: MyType, b?: MyType, c?: MyType): int {
    arktest.assertEQ(a!.x, 5)
    if (b == null && c == null) {
        return 0;
    }
    if (c == null) {
        return -1;
    }

    return a!.x + b!.x + c.x;
}

function foo7(a: MyType = new MyType(5), b?: MyType): int {
    if (b == null) {
        return 0;
    }
    return a.x + b.x;
}

function foo8(a?: MyType, b: MyType = new MyType(5), c?: MyType): int {
    arktest.assertEQ(b.x, 5)

    if (a == null && c == null) {
        return 0;
    }

    return a!.x + b.x + c!.x;
}

function foo9(a: MyType, b: MyType = new MyType(5)): int {
    return a.x + b.x;
}

function foo10(a: MyType, b: MyType = new MyType(5), c: MyType = new MyType(10)): int {
    return a.x + b.x + c.x;
}

function foo11(a: MyType, b: MyType, c: MyType = new MyType(5)): int {
    return a.x + b.x + c.x;
}

function foo12(a: MyType, b: MyType, c: MyType = new MyType(10), d: MyType = new MyType(5)): int {
    return a.x + b.x + c.x + d.x;
}
