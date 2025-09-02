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
    field: string = "A"
}
class B {
    field: string = "B"
}

class Test {
    foo1(a: string): string {
        return "invoke1"
    }
    foo2(a: A, b: B, ...args: string[]): string {
        return "invoke2"
    }
    foo3(a: B, ...args: string[]): string {
        return "invoke3"
    }

    overload foo{ foo1, foo2, foo3 }
}

function main() {
    let a: Test = new Test();
    arktest.assertEQ(a.foo("abc"), "invoke1");

    let restparam1: string[] = ["abc", "abc"]
    arktest.assertEQ(a.foo(new A(), new B()), "invoke2");
    arktest.assertEQ(a.foo(new A(), new B(), "abc"), "invoke2");
    arktest.assertEQ(a.foo(new A(), new B(), "abc", "abc", "abc"), "invoke2");
    arktest.assertEQ(a.foo(new A(), new B(), ...restparam1), "invoke2");

    let restparam2: string[] = ["abc", "abc"]
    arktest.assertEQ(a.foo(new B()), "invoke3");
    arktest.assertEQ(a.foo(new B(), "abc"), "invoke3");
    arktest.assertEQ(a.foo(new B(), "abc", "abc", "abc"), "invoke3");
    arktest.assertEQ(a.foo(new B(), ...restparam1), "invoke3");
}
