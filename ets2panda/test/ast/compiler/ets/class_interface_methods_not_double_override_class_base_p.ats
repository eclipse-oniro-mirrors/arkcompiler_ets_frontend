/*
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

class A {}
class B {}
class C {}

interface I1 {
    foo(p: A | B | C): B {
        return new B()
    }
}

interface I2 extends I1 {
    foo(p: A | B): B | C {
        return new B()
    }
}

class C1 implements I1 {
    foo(p: A): C {
        return new C()
    }
}

class C2 extends C1 implements I2 {}

let result = new C2().foo(new A())
