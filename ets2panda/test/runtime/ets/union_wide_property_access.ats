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

class A { memb: number = 2; }
class B { memb: number = 6; }
class C { memb: A|B = new A(); }

function bar(a0: A | B): void {
    a0.memb = 50;
}

function foo(arr: Int8Array | Int16Array): void {
    let len = arr.length
    arktest.assertEQ(len, 42)
}

function main() {
    let buffer: ArrayBuffer = new ArrayBuffer(42);
    let byteArr: Int8Array= new Int8Array(buffer)
    foo(byteArr);
    arktest.assertEQ(byteArr.length, 42)

    let ab: A|B = new A();
    bar(ab);
    arktest.assertEQ(ab.memb, 50)

    let cClass: C = new C();
    cClass.memb = new B();
    arktest.assertEQ(cClass.memb.memb, 6)
}
