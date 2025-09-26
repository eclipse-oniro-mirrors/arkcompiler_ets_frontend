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
interface K {}
class A {}
class B extends A implements I, J {}
class C extends B {}
class D extends C {}

function foo(x: I): string {
    return "I";
}
function foo(x: J): string {
    return "J";
}

function main(): int {
    let d : string = /* @@ label */foo(new D());
    return 0;
}
