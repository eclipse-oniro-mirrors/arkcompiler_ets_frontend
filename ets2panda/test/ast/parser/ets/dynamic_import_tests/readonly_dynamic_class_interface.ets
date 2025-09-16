/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

/*---
flags: [dynamic-ast]
---*/

import { C, I2 } from "dynamic_import_tests/modules/module"

function main() {
    let c: Readonly<C> = { s: "123" }
    c.s = "234"
    let i: Readonly<I2> = {
        name: "abc",
        age: 123
    }
    i.age = 124
}

/* @@? 24:5 Error TypeError: The 'Readonly' property cannot be reassigned. */
/* @@? 24:5 Error TypeError: Cannot assign to a readonly variable s */
/* @@? 29:5 Error TypeError: The 'Readonly' property cannot be reassigned. */
