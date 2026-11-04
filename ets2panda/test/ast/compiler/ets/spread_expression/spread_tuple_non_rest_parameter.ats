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
desc: If a tuple is spread while calling a function, then an appropriate parameter
  must be of the rest kind.
name: spread_tuple_non_rest_parameter
tags:
- negative
- compile-only
---*/

function foo2(n1: number, n2: string): number {
    return n1
}

function main() {
    let a_tuple: [number, string] = [1, "2"]
    /* @@ label */foo2(...a_tuple)
}


/* @@@ label Error Semantic error ESE0127: No matching call signature for foo2([Double, String]) */