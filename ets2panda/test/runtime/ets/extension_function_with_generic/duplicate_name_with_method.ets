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

class A<T> {
    data1: T;
    data2: T;
    constructor(d1: T, d2: T) {
        this.data1 = d1;
        this.data2 = d2;
    }

    goo1(): T {
        return this.foo(this.data1);
    }

    goo2(): T {
        return foo(this, this.data1);
    }

    private foo(p: T): T {
        return this.data1;
    }
}

function foo<T>(this:A<T>, p: T): T {
    return this.data2;
}

function main() {
    let a1 = new A<int>(1, 2);
    arktest.assertEQ(a1.foo(1), 2)
    arktest.assertEQ(foo(a1, 1), 2)
    arktest.assertEQ(a1.goo1(), 1)
    arktest.assertEQ(a1.goo2(), 2)


    let s0 = new String("test");
    let s1 = new String("private method call");
    let s2 = new String("extension func call");
    let a2 = new A<String>(s1, s2);
    arktest.assertEQ(a2.foo(s0), s2)
    arktest.assertEQ(foo(a2, s0), s2)
    arktest.assertEQ(a2.goo1(), s1)
    arktest.assertEQ(a2.goo2(), s2)

    let a3 = new A<number>(0.1, 0.2);
    arktest.assertEQ(a3.foo(0.0), 0.2)
    arktest.assertEQ(foo(a3, 0.0), 0.2)
    arktest.assertEQ(a3.goo1(), 0.1)
    arktest.assertEQ(a3.goo2(), 0.2)
}