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

class A {
    a: int;
    constructor(a: int) {
        this.a = a;
    }
}

class B extends A {
    constructor(a: int) {
        super(a);
    }
}

class C extends A {
    constructor(a: int) {
        super(a);
    }
}

function foo<T, K>(a: T, b: T | K, c: T | K): void {
    arktest.assertTrue(a instanceof B)
    arktest.assertTrue(b instanceof A)
    arktest.assertTrue(c instanceof C)
}

function main(): void {
    foo(new B(10), new A(20), new C(30));
}