/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS' BASIS,",
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.",
 * See the License for the specific language governing permissions and",
 * limitations under the License."
 */

// 1.2


export class A {data: Object = 0}


import { A } from './fixtures/homecheck-interop-assign/Field/1.1import1.2type/primitive_type_and_boxed_type/file1'

function goo() {
    let a = new A()
    a.data = 123
}


let a = new A()
function foo() {
    let y: Y = { s: "Hi" }
    a.data = new Number(123)
}

