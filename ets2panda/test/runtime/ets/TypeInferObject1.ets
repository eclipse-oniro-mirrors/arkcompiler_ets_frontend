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

class A <U, T> {
    public value1: U;
    public value2: T;

    constructor(a: U, b: T) {
        this.value1 = a;
        this.value2 = b;
    }
}

class B <U, T, K> {
    public value1: U;
    public value2: T;
    public value3: K;

    constructor(a: U, b: T, c: K) {
        this.value1 = a;
        this.value2 = b;
        this.value3 = c;
    }
}

function func1<U, T, K>(arg: A<T, U>, arg1: K){
    arktest.assertEQ(typeof arg1, "number")
    return new A<U, T>(arg.value2, arg.value1);
}

function func2<U, T>(arg: B<T, U, int>){
    return new B<U, T, int>(arg.value2, arg.value1, arg.value3);
}

function func3<U, T>(arg: A<T, U>){
    return new A<U, T>(arg.value2, arg.value1);
}

function main(): void {
    let a = new A<int, String>(10, "Test");
    arktest.assertEQ(typeof a, "object")
    arktest.assertEQ(typeof a.value1, "number")
    arktest.assertEQ(typeof a.value2, "string")

    let funcValue1 = func1(a, 12);
    arktest.assertEQ(typeof funcValue1.value1, "string")
    arktest.assertEQ(typeof funcValue1.value2, "number",  `got ${typeof funcValue1.value2}`)

    let b = new B<int, String, int>(10, "Test", 40);
    let funcValue2 = func2(b);
    arktest.assertEQ(typeof funcValue2.value1, "string")
    arktest.assertEQ(typeof funcValue2.value2, "number")
    arktest.assertEQ(typeof funcValue2.value3, "number")

    let c = new A<int, String>(10, "Test");
    let funcValue3 = func3(c);
    arktest.assertEQ(typeof funcValue3.value1, "string")
    arktest.assertEQ(typeof funcValue3.value2, "number")
}
