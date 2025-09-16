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
 
class B {
    foo(a: int, op?: int): int { return a }
}
class A extends B {
    override foo(a: int, op?: int): int { return a }
}

class C<T> {
  foo(a: T, op?: T): T { return a }
}
class D<T> extends C<T> {
  override foo(a: T, op?: T): T { return a }
}

function main(){
    let a = new A();
    let b = new B();
    let c = new C<string>();
    let d = new D<string>();
 
    arktest.assertEQ(b.foo(5), a.foo(5))
    arktest.assertNE(b.foo(6), a.foo(5))
    arktest.assertNE(b.foo(5), a.foo(6))
    arktest.assertEQ(c.foo("x"), d.foo("x"))
    arktest.assertNE(c.foo("x"), d.foo("y"))
    arktest.assertNE(c.foo("y"), d.foo("x"))
}
