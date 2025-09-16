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
 
class X {}
class Y extends X {}

abstract class A {
    foo(x: Y): string { return "A.foo(Y)" }
    abstract foo(x: Double): String
    foo2(x: Y): Object { return "A.foo(Y)" }
}

class B extends A {
    foo(x: X): string { return "B.foo(X)" }
    foo(x: Floating): String { return "B.foo(Floating)" }
}

class C extends A {
    foo(x: Double): String { return "C.foo(Double)" }
    override foo2(x: Y): String { return "C.foo2(Y)" }
}

function main() {
    arktest.assertEQ(new B().foo(new Y()), "B.foo(X)")
    arktest.assertEQ(new B().foo(new Double()), "B.foo(Floating)")
    arktest.assertEQ(new C().foo2(new Y()), "C.foo2(Y)")
}