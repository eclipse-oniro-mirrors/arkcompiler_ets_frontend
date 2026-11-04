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
    tags: [compile-only, negative]
    expected_out: |
        Semantic error ESE0318: Type 'Array<Int>' cannot be assigned to type 'ReadonlyArray<Int>'
        Semantic error ESE0318: Type 'Array<String>' cannot be assigned to type 'ReadonlyArray<String>'
        Semantic error ESE0318: Type '[Int, String]' cannot be assigned to type 'readonly [Int, String]'
---*/

function main(): void {
    let src: int[] = [1, 2, 3];
    let strSrc: string[] = ["a", "b"];

    let dest: readonly int[] = [0, 0, 0];
    let strDest: readonly string[] = ["", ""];

    // Non-readonly arrays must not be assigned to readonly arrays via a simple assignment.
    dest = src;
    strDest = strSrc;
    dest = [4, 5, 6];

    // Non-readonly tuples must not be assigned to readonly tuples via a simple assignment.
    let tupleSrc: [int, string] = [1, "a"];
    let tupleDest: readonly [int, string] = [0, ""];
    tupleDest = tupleSrc;
    tupleDest = [7, "b"];
}
