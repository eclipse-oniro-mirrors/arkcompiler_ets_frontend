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

function foo1(x : readonly int[], y : readonly [int, string]) {
    /* @@ label */x[1] = 0
    /* @@ label1 */y[0] = 10
    /* @@ label2 */y[1] = 'a'
}

function foo2(x : Readonly<int[]>, y : readonly [int, string]) {
    /* @@ label3 */x[1] = 0
    /* @@ label4 */y[0] = 10
    /* @@ label5 */y[1] = 'a'
}

let x1 : readonly int[] = [1, 2]
let y1 : readonly [int, string] = [1, "a"]
/* @@ label6 */x1[0] = 2
/* @@ label7 */y1[0] = 1

let x2 : Readonly<int[]> = [0]
/* @@ label8 */x2[0] = 1
let y2 : readonly [int, string]
/* @@ label9 */y2[1] = "b"

/* @@@ label Error Semantic error ESE0058: Cannot modify an array or tuple content that has the readonly parameter */
/* @@@ label1 Error Semantic error ESE0058: Cannot modify an array or tuple content that has the readonly parameter */
/* @@@ label2 Error Semantic error ESE0058: Cannot modify an array or tuple content that has the readonly parameter */
/* @@@ label3 Error Semantic error ESE0058: Cannot modify an array or tuple content that has the readonly parameter */
/* @@@ label4 Error Semantic error ESE0058: Cannot modify an array or tuple content that has the readonly parameter */
/* @@@ label5 Error Semantic error ESE0058: Cannot modify an array or tuple content that has the readonly parameter */
/* @@@ label6 Error Semantic error ESE0058: Cannot modify an array or tuple content that has the readonly parameter */
/* @@@ label7 Error Semantic error ESE0058: Cannot modify an array or tuple content that has the readonly parameter */
/* @@@ label8 Error Semantic error ESE0058: Cannot modify an array or tuple content that has the readonly parameter */
/* @@@ label9 Error Semantic error ESE0058: Cannot modify an array or tuple content that has the readonly parameter */
