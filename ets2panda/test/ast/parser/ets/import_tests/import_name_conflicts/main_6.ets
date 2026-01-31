/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

import {dbl as /* @@ label1 */c} from "./imported_module_1.ets";
import * as c from "./imported_module_2.ets";


function main() {
  /* @@ label2 */console.log(/* @@ label3 */c);
}

/* @@@ label1 Error Semantic error ESE0350: Variable 'c' is already defined with different type. */
/* @@@ label2 Error Semantic error ESE0127: No matching call signature for log(c) */
/* @@@ label3 Error Semantic error ESE43831: Namespace 'c' cannot be used as object */
