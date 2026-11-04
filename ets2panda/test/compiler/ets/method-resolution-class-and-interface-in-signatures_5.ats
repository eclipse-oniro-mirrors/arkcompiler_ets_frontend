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

class A {}
class B extends A implements I, J {}
class C extends B {}

function foo(x: I): string {
    return "I";
}
function foo(x: J): string {
    return "J";
}
function foo(x: A): string {
    return "A";
}
function foo(x: B): string {
    return "B";
}

function main(): int {
    let b : string = foo(new B());
    return 0;
}

