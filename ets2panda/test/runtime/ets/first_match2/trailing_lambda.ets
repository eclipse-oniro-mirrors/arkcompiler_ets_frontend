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

class A { }
class B { }

class Test {
    foo(a: A, lambda1: () => void): string {
        lambda1()
        return "invoke1";
    }
    foo(a: A, b: B, lambda1: (a: number) => void): string {
        lambda1(1)
        return "invoke2";
    }
    foo(a: A, b: B, lambda1: () => void): string {
        lambda1()
        return "invoke3";
    }
    foo(a: B, b: A, lambda1: () => void): string {
        lambda1()
        return "invoke4";
    }
}

function main() {
    let a: Test = new Test();

    arktest.assertEQ(a.foo(new A(), () => { }), "invoke1")
    arktest.assertEQ(a.foo(new A()){ return; }, "invoke1")

    arktest.assertEQ(a.foo(new A(), new B(), (a: number) => { }), "invoke2")

    arktest.assertEQ(a.foo(new A(), new B(), () => { }), "invoke2")
    arktest.assertEQ(a.foo(new A(), new B()){ return; }, "invoke2")

    arktest.assertEQ(a.foo(new B(), new A(), () => { }), "invoke4")
    arktest.assertEQ(a.foo(new B(), new A()){ return; }, "invoke4")
}
