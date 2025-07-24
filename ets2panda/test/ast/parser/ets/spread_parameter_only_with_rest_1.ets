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

function foo(x: number) {
  console.log(x)
}
let args_tuple: [number, number] = [0, 1]
/* @@ label1 */foo(1, ...args_tuple)
let args_array: number[] = [2, 3]
/* @@ label2 */foo(4, ...args_array)

/* @@@ label1 Error TypeError: The function or method being called needs a rest parameter to accept arguments passed via the spread operator. */
/* @@@ label1 Error TypeError: No matching call signature for foo(Int, [Double, Double]) */
/* @@@ label2 Error TypeError: The function or method being called needs a rest parameter to accept arguments passed via the spread operator. */
/* @@@ label2 Error TypeError: No matching call signature for foo(Int, Double) */
