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

class A1 {}
class A2 extends A1 {}
class A3 extends A2 {}

class C1 {
    foo(v: A1): string
    {
        return "foo1"
    }
}

class C2 extends C1 {
    foo(v: A3): string
    {
        return "foo2"
    }
}

class C3 extends C2 {
    foo(v: A2): string
    {
        return "foo3"
    }
}

let c3 = new C3()
let result = c3.foo(new A3())
