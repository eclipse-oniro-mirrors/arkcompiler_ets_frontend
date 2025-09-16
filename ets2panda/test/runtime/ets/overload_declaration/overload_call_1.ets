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

class A{}
class B{}
class C{}

class Test1 {
    foo110(a: A): A {
        return a;
    }
    foo112(a: B): B {
        return a;
    }

    foo113(a: C): C {
        console.log(a)
        return a;
    }
    overload foo111{foo110,foo112,foo113}
}

function main() {
    let a: Test1 = new Test1();
    let b = new B();
    a.foo111(b);
}
