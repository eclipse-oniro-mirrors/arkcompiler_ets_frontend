/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

function foo(x: number, y: number) {
  console.log(x, y)
}
let args_tuple: [number, number] = [0, 1]
/* @@ label1 */foo(...args_tuple)
let args_array: number[] = [2, 3]
/* @@ label2 */foo(...args_array)

function moo(x: number, y: number, z: number) {
  console.log(x, y, z)
}
args_tuple = [0, 1]
/* @@ label3 */moo(1, ...args_tuple)
args_array = [2, 3]
/* @@ label4 */moo(2, ...args_array)


/* @@@ label1 Error Semantic error ESE44192: The function or method being called needs a rest parameter to accept arguments passed via the spread operator. */
/* @@@ label1 Error Semantic error ESE0127: No matching call signature for foo([Double, Double]) */
/* @@@ label2 Error Semantic error ESE44192: The function or method being called needs a rest parameter to accept arguments passed via the spread operator. */
/* @@@ label2 Error Semantic error ESE0127: No matching call signature for foo(Array<Double>) */
/* @@@ label3 Error Semantic error ESE44192: The function or method being called needs a rest parameter to accept arguments passed via the spread operator. */
/* @@@ label3 Error Semantic error ESE0127: No matching call signature for moo(Int, [Double, Double]) */
/* @@@ label4 Error Semantic error ESE44192: The function or method being called needs a rest parameter to accept arguments passed via the spread operator. */
/* @@@ label4 Error Semantic error ESE0127: No matching call signature for moo(Int, Array<Double>) */
