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
    X: number = 0;
    data: number = 0;
    setX(n: number): this {
        this.X = n;
        return this;
    }
}

class D extends B {
    Y: number = 0;
    setY(n: number): this {
        this.Y = n;
        return this;
    }
}

class A {
    d:D;
    data: number = 0;
    constructor() {
        this.d = new D();
    }
}

function foo(this: A, f: (this:B) => this) {
    this.d.setX(1).f().setY(2);
    arktest.assertEQ(this.d.X, 1);
    arktest.assertEQ(this.d.Y, 2);
}

function foo2(this: A, f: (this:B) => this) {
    f(this.d.setX(1)).setY(2);
    arktest.assertEQ(this.d.X, 1);
    arktest.assertEQ(this.d.Y, 2);
}

function goo(n:number, f: (this:B) => this) {
    let d = new D();
    d.setX(1).f().setY(2);
    arktest.assertEQ(d.data, n);
    arktest.assertEQ(d.X, 1);
    arktest.assertEQ(d.Y, 2);
}

function goo2(n:number, f: (this:B) => this) {
    let d = new D();
    f(d.setX(1)).setY(2);
    arktest.assertEQ(d.data, n);
    arktest.assertEQ(d.X, 1);
    arktest.assertEQ(d.Y, 2);
}

let a = new A();
// function with receiver test;
a.foo() { this.data = 666; return this; } 
arktest.assertEQ(a.d.data, 666);
arktest.assertEQ(a.data, 0);
foo(a) { this.data = 66; return this; }
arktest.assertEQ(a.d.data, 66);
arktest.assertEQ(a.data, 0);

a.foo2() { this.data = 66; return this; } 
arktest.assertEQ(a.d.data, 66);
arktest.assertEQ(a.data, 0);
foo2(a) { this.data = 666; return this; }
arktest.assertEQ(a.d.data, 666);
arktest.assertEQ(a.data, 0);

// normal function test;
let n: number = 66;
goo(n) { this.data = n; return this; }
goo2(n) { this.data = n; return this; }
