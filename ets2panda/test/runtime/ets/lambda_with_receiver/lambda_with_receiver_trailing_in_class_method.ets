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
    data:number = 0;
    constructor() {
        this.b = new B();
    }
    
    foo(f: (this:B) => void) {
        this.b.f();
    }

    foo2(f: (this:B) => void) {
        f(this.b);
    }

}

let a = new A();
a.foo() { this.data = 666 } 
arktest.assertEQ(a.b.data, 666);
arktest.assertEQ(a.data, 0);

a.foo2() { this.data = 66 } 
arktest.assertEQ(a.b.data, 66);
arktest.assertEQ(a.data, 0);
