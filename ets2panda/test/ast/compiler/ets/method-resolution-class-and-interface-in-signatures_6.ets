/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

interface I {}
interface J {}

class A implements I, J {}

function foo(b: int, a: I): int {
    return b;
}
function foo(d: double, a: J): double {
    return d;
}
function foo(a: I, b: int): int {
    return b;
}
function foo(a: J, d: double): double {
    return d;
}
function foo(a: I, d: double): double {
    return d;
}

function main(): int {
    let asd1: int = foo(1, new A());
    let asd2: double = foo(2.1, new A());
    let asd3: double = foo(2.0, new A());
    let asd4: int = foo(new A(), 1);
    let asd5: double = /* @@ label */foo(new A(), 2.1);
    return 0;
}