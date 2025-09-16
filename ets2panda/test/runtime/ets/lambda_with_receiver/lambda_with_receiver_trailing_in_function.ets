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

class B {
    data: number = 0;
}

class A {
    b:B;
    data: number = 0;
    constructor() {
        this.b = new B();
    }
}

function foo(this: A, f: (this:B) => void) {
    this.b.f();
}

function foo2(this: A, f: (this:B) => void) {
    f(this.b);
}

function goo(n:number, f: (this:B) => void) {
    let b = new B();
    b.f();
    arktest.assertEQ(b.data, n);
}

function goo2(n:number, f: (this:B) => void) {
    let b = new B();
    f(b);
    arktest.assertEQ(b.data, n);
}

let a = new A();
// function with receiver test;
a.foo() { this.data = 666; } 
arktest.assertEQ(a.b.data, 666);
arktest.assertEQ(a.data, 0);
foo(a) { this.data = 66; }
arktest.assertEQ(a.b.data, 66);
arktest.assertEQ(a.data, 0);

a.foo2() { this.data = 66; } 
arktest.assertEQ(a.b.data, 66);
arktest.assertEQ(a.data, 0);
foo2(a) { this.data = 666; }
arktest.assertEQ(a.b.data, 666);
arktest.assertEQ(a.data, 0);

// normal function test;
let n: number = 66;
goo(n) { this.data = n; }
goo2(n) { this.data = n; }
