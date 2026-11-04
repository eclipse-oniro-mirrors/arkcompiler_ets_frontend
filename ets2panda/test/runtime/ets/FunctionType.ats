/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

let cb: (a: int) => int;

class A {
    a(a: int): int {
        return this.value;
    }

    value: int = 1;
}

class B {
    b(a: int): int {
        return this.value;
    }

    value: int = 2;
}

function main(): void {
    let a: A = new A();
    let b: B = new B();
    let c = 10;

    if (c < 10) {
        cb = a.a;
    } else {
        cb = b.b;
    }

    arktest.assertEQ(cb(21), 2)
}
