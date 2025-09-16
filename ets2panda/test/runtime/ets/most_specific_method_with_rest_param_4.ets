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

class A {
    foo(...n: FixedArray<number>){
        return "A";
    }
}

class B extends A {
    foo(...n: FixedArray<number>){
        return "B";
    }
}

class C extends B {
    foo(...n: FixedArray<number>){
        return "C";
    }
}

function main() {
    let a: A = new A();
    let b: B = new B();
    let c: C = new C();
    arktest.assertEQ(a.foo(), "A")
    arktest.assertEQ(a.foo(1), "A")
    arktest.assertEQ(b.foo(), "B")
    arktest.assertEQ(b.foo(1,2), "B")
    arktest.assertEQ(c.foo(), "C")
    arktest.assertEQ(c.foo(1,2,3), "C")
}