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
    a: int
    constructor(param: int) {
        this.a = param
    }
}

class Base {
    get foo() {
        return new A(1);
    }
}

class Derived extends Base {
    get foo() { 
        return new A(2);
    }
}

let x: Derived = new Derived();
let y = x.foo;
arktest.assertEQ(y.a, 2)

let v: Base = new Base();
let w = v.foo;
arktest.assertEQ(w.a, 1)
