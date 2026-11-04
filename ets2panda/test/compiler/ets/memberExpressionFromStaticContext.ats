/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
    foo_in_A() : void {}
    static static_foo_in_A() : void {}

    static bar_in_A(arg: A) : void {
        arg.foo_in_A()
        A.static_foo_in_A()
    }
}

class B {
    foo_in_B() : void {}
    static static_foo_in_B() : void {}

    static bar_in_B(arg: A) : void {
        arg.foo_in_A()
        A.static_foo_in_A()
        B.static_foo_in_B();

        let b_instance = new B()
        b_instance.foo_in_B()
    }
}

class C extends A {
    foo_in_C() : void {}
    static static_foo_in_C() : void {}

    static bar_in_C(arg: A) : void {
        arg.foo_in_A()
        A.static_foo_in_A()
        C.static_foo_in_C();

        let c_instance = new C()
        c_instance.foo_in_C()
        c_instance.foo_in_A()
    }
}
