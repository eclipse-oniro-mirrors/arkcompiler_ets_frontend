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

const matrix1: number[] = [1, 2];
const matrix2: string[] = ['a', 'b'];
const arr = new Array<number[] | string[]>(matrix1, matrix2);

console.log(arr.length)
console.log(arr[0][1]+"")
console.log(arr[1][0]+"")

/* @@? 21:1 Error Semantic error ESE0127: No matching call signature for log("") */
/* @@? 21:13 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
/* @@? 22:1 Error Semantic error ESE0127: No matching call signature for log("") */
/* @@? 22:13 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
