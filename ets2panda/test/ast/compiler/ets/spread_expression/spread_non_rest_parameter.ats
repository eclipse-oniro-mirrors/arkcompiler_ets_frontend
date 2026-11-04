/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
desc: If an array is spread while calling a function, then an appropriate parameter
  must be of the rest kind.
name: spread_non_rest_parameter
tags:
- negative
- compile-only
---*/

function foo1(n1: number, n2: number): number {
    return n1 + n2
}

function main() {
    let an_array = [1, 2]
    /* @@ label */foo1(...an_array)
}


/* @@@ label Error Semantic error ESE0127: No matching call signature for foo1(Array<Int>) */