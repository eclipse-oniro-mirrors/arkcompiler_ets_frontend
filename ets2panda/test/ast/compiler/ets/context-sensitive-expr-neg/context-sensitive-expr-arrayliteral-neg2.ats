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

type U = int[] | double[] | string[] | [int, string] | [double, string]

let x: U = [33, 44.0]; // choose double[]
let y: U = ['aa', 'bb']; // choose string[]
let z: U = /* @@ label1 */[33, 'aa']; // CTE, Ambiguity on `[int, string]` and `[double, string]`
let w: U = /* @@ label2 */[1] // CTE, Ambiguity on `int[]` and `double[]`
/* @@@ label1 Error Semantic error ESE146473: Cannot infer a specific type of the Array Expression, ambiguous between type `[Int, String]` and type `[Double, String]`. */
/* @@@ label1 Error Semantic error ESE0318: Type 'Array<Int|String>' cannot be assigned to type 'Array<Int>|Array<Double>|Array<String>|[Int, String]|[Double, String]' */
/* @@@ label2 Error Semantic error ESE146473: Cannot infer a specific type of the Array Expression, ambiguous between type `Array<Int>` and type `Array<Double>`. */
