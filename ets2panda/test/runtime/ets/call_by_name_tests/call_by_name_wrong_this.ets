/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
    foo() { return 1 }
}

class B {
    foo() { return 2 }
}

class C {
    foo() { return 3 }
}

class D {}

function foo(a: A|B) { return a.foo() }

let f: Function = foo

arktest.assertEQ(f.unsafeCall(new A), 1)
arktest.assertEQ(f.unsafeCall(new B), 2)
arktest.expectThrow(() => { f.unsafeCall(new C) }, (e: Error) => { return e instanceof ClassCastError })
arktest.expectThrow(() => { f.unsafeCall(new D) }, (e: Error) => { return e instanceof ClassCastError })
