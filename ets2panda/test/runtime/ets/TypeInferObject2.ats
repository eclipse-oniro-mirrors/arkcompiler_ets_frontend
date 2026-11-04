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

class A<T, U, K> {
    public a: T;
    public b: U;
    public c: K;

    constructor(a: T, b: U, c: K) {
        this.a = a;
        this.b = b;
        this.c = c;
    }
}

function foo<T, U extends Number, K, L extends A<T, K, T>>(a: A<T, K, int>, b: U, c: K, d: U, e: L) {
    arktest.assertEQ(typeof a.a, "int")
    arktest.assertEQ(typeof a.b, "string")
    arktest.assertEQ(typeof a.c, "int")
    arktest.assertEQ(typeof b, "number")
    arktest.assertEQ(typeof c, "string")
    arktest.assertEQ(typeof d, "number")
    arktest.assertEQ(typeof e, "object")
    return new A<T, U, K>(a.a, b, a.b);
}

function bar<T, U, K>(a: A<T, K, T>, b : U) {
    arktest.assertEQ(typeof a.a, "int")
    arktest.assertEQ(typeof a.b, "string")
    arktest.assertEQ(typeof a.c, "int")
    arktest.assertEQ(typeof b, "number")
    return new A<T, U, K>(a.a, b, a.b);
}

function main() {
    let a = new A<int, String, int>(10, "Test", 40);
    let b: number = 20;
    let c = foo(a, b, "Test", 20, a);
    arktest.assertEQ(typeof c.a, "int")
    arktest.assertEQ(typeof c.b, "number")
    arktest.assertEQ(typeof c.c, "string")
    let d = bar(a, b);
    arktest.assertEQ(typeof d.a, "int")
    arktest.assertEQ(typeof d.b, "number")
    arktest.assertEQ(typeof d.c, "string")
}
