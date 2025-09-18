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

import {dbl as c} from /* @@ label1 */"./imported_module_1.ets";
import * as c from "./imported_module_2.ets";


function main() {
    /* @@ label2 */console.log(/* @@ label3 */c);
}
/* @@@ label1 Error TypeError: Variable 'c' is already defined with different type.  */
/* @@@ label2 Error TypeError: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Boolean): void`  */
/* @@@ label2 Error TypeError: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Byte): void`  */
/* @@@ label2 Error TypeError: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Short): void`  */
/* @@@ label2 Error TypeError: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Char): void`  */
/* @@@ label2 Error TypeError: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Int): void`  */
/* @@@ label2 Error TypeError: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Long): void`  */
/* @@@ label2 Error TypeError: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Float): void`  */
/* @@@ label2 Error TypeError: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Double): void`  */
/* @@@ label3 Error TypeError: Namespace 'c' cannot be used as object  */