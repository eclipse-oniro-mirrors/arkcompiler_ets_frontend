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

class A {
    data: number = 0;
    foo(n: number, f: (this: A) => void) {
        this.f();
    }

    foo(n: number, f: () => void) {
        f();
    }
}

function foo(this: A, f: (this: A) => void) {
    this.f();
}

function foo(this: A, n: number, n2: number, f: () => void) {
    f();
}

function foo(this: A, f: (this: A) => void, n: number) {
    this.f();
}

function foo(a: A, n: number, f: (this: A) => void) {
    a.f();
}

function foo(a: A, n: number, f: () => void) {
    f();
}

let a = new A();
// function with receiver append trailing lambda with receiver.
a.foo(){ this.data = 1; } 
arktest.assertEQ(a.data, 1); 
foo(a) {this.data = 2; }
arktest.assertEQ(a.data, 2);

// function with receiver append trailing lambda.
a.foo(1, 2){ a.data = -1; } 
arktest.assertEQ(a.data, -1); 
foo(a, 1, 2) {a.data = -2; }
arktest.assertEQ(a.data, -2);

// function with receiver contain param as lambda with receiver.
let n: number = 3;
a.foo((this: A):void =>{ this.data = n}, n);
arktest.assertEQ(a.data, 3);
foo(a, (this: A):void =>{ this.data = n + 1}, n);
arktest.assertEQ(a.data, 4);

// normal function append trailing lambda with receiver.
foo(a, n + 2){ this.data = n + 2 };
arktest.assertEQ(a.data, 5);

// method append trailing lambda with receiver.
a.foo(n + 3){ this.data = n + 3 };
arktest.assertEQ(a.data, 6);

// method append normal trailing lambda.
a.foo(n + 4){a.data = n + 4}
arktest.assertEQ(a.data, 7);

// normal function append normal trailing lambda.
foo(a, n + 5){a.data = n + 5}
arktest.assertEQ(a.data, 8);
