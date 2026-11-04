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

let a: int = 1;

let lambda1: () => int = (): int => {
    a++;
    return a;
}

function foo(a: (a: int, b: int) => int): int {
    let value = a(1, 2);
    value++;
    return value;
}

class A {
    bar(): int {
        return foo(this.a);
    }

    a: (a: int, b: int) => int = (a: int, b: int): int => {
        return a + b;
    }
}

function main(): void {
    let a = new A();
    let value = a.bar();
    value++;
}
