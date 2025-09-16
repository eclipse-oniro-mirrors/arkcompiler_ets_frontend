/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

let results : int[] = [0,0,0];
let index : int = 0;

class A {
    constructor() {
        results[index++] = 1;
    }
}

class B extends A {
    constructor() {
        super();
        results[index++] = 2;
    }
}

class C extends B {
    constructor() {
        results[index++] = 3;
    }
}

final class D extends C {}

function main() : void {
    results = [0, 0, 0];
    index = 0;

    let a = new A();
    arktest.assertTrue(results[0] == 1 && results[1] == 0 && results[2] == 0 && index == 1)

    results = [0,0,0,0];
    index = 0;
    let b = new B();
    arktest.assertTrue(results[0] == 1 && results[1] == 2 && results[2] == 0 && index == 2)

    results = [0,0,0,0];
    index = 0;
    let c = new C();
    arktest.assertTrue(results[0] == 1 && results[1] == 2 && results[2] == 3 && index == 3)

    results = [0,0,0,0];
    index = 0;
    let d = new D();
    arktest.assertTrue(results[0] == 1 && results[1] == 2 && results[2] == 3 && index == 3)
}
