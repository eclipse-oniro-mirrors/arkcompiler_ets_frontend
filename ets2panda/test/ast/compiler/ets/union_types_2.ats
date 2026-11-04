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

class A {}

class B {
    num: int = 42;
}

class C {
    num: int = 43;
    constructor(num: int) {
        this.num = num;
    }
    num_to_return() : int {
        return this.num;
    }
}

function foo(x : A | B | C) {
    x.num;  // CTE - `A` doesn't have field `num`
}

function main() {
    let x : A | B | C;
    x = new C(777);
    foo(x);
}


/* @@? 33:7 Error Semantic error ESE0087: Property 'num' does not exist on type 'A' */