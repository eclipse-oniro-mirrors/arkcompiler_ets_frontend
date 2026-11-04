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
desc: Test if ambiguity check works for numeric union types
name: union_numeric_init_4
tags:
- negative
- compile-only
---*/

function main() {

    const a = 32767;
    let b: Short | Int = /* @@ label */ a;

}


/* @@? 27:41 Error Semantic error ESE101680: Ambiguous value 32767, fits into multiple types */
