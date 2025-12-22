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
    value: boolean = true;
}
type F1 = (this: A) => boolean;
type F2 = (a: A) => boolean;

function foo(this: A): boolean {
    return this.value;
}

let a = new A();
let f1: F2 = foo;
let f2: F2 = (this: A): boolean => { return this.value }
a.f1();
a.f2();


/* @@? 29:3 Error Semantic error ESE0087: Property 'f1' does not exist on type 'A' */
/* @@? 30:3 Error Semantic error ESE0087: Property 'f2' does not exist on type 'A' */