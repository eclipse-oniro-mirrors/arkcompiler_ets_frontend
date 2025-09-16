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

function bar<T>(a: T): T {
    return a;
}

function foo<T, U, K>(a: A<A<K, U, T>, String, K>): void {
    arktest.assertEQ((typeof a.a), "object")
    arktest.assertEQ((typeof a.b), "string")
    arktest.assertEQ((typeof a.c), "boolean")
    arktest.assertEQ((typeof a.a.a), "boolean")
    arktest.assertEQ((typeof a.a.b), "string")
    arktest.assertEQ((typeof a.a.c), "number")
}

function main(): void {
    let a = new A<A<Boolean, String, Int>, string, boolean>(new A<Boolean, String, Int>(bar(true), "alma", 20), "korte", true);
    foo(a);
}
