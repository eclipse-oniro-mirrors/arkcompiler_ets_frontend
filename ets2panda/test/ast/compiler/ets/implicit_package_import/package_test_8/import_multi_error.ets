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

package subpackage_a;

import {myvar} from "./package_module"
import {myvar2} from "./package_module"

export let a: String = "SubpackageA";


/* @@? 18:1 Error Syntax error ESY0003: Package module cannot import from a file in it's own package. */
/* @@? 18:21 Error Semantic error ESE0360: Cannot find import: ./package_module */
/* @@? 19:1 Error Syntax error ESY0003: Package module cannot import from a file in it's own package. */
/* @@? 19:22 Error Semantic error ESE0360: Cannot find import: ./package_module */