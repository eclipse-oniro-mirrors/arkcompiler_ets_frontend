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
}

class B {
    a: A;
    data: number = 0;
    constructor(){
        this.a = new A();
    }
}

function foo(this: A, f: (this: A) => void) {
    this.f();
}

function goo(this: B, f: (this: A) => void) {
    this.a.f();
    arktest.assertEQ(this.a.data, 555);

    this.a.foo(){this.data = 55;}
    arktest.assertEQ(this.a.data, 55);

    f(this.a);
    arktest.assertEQ(this.a.data, 555);

    foo(this.a){this.data = 55;}
    arktest.assertEQ(this.a.data, 55);
}

let a = new A();
a.foo(){this.data = 666}
arktest.assertEQ(a.data, 666);
foo(a){this.data = 66}
arktest.assertEQ(a.data, 66);

let b = new B();
b.goo(){this.data = 555;}
