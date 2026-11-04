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

type T = (int[] | (int | int[])[])[]

let topLevel: T = [
    [1, 2],
    [1, [1, 2]]
]

const topLevelConst: T = [
    [1, 2],
    [1, [1, 2]]
]

function buildInFunction() {
    let local: T = [
        [1, 2],
        [1, [1, 2]]
    ]
}

/* @@? 19:5 Error Semantic error ESE146473: Cannot infer a specific type of the Array Expression, ambiguous between type `Array<Int>` and type `Array<Int|Array<Int>>`. */
/* @@? 24:5 Error Semantic error ESE146473: Cannot infer a specific type of the Array Expression, ambiguous between type `Array<Int>` and type `Array<Int|Array<Int>>`. */
/* @@? 30:9 Error Semantic error ESE146473: Cannot infer a specific type of the Array Expression, ambiguous between type `Array<Int>` and type `Array<Int|Array<Int>>`. */
