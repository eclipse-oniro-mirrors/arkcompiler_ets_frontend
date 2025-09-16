/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable low or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

function sum(a: int, ...numbers: [number, number, number]): number {
    return numbers[0] + numbers[1] + numbers[2]
}

/* @@ label1 */sum()
/* @@ label2 */sum(11)
/* @@ label3 */sum(11,12,13)
/* @@ label4 */sum(11,12,13,15,16)

/* @@@ label1 Error TypeError: Expected 1 arguments, got 0. */
/* @@@ label1 Error TypeError: Expected 4 arguments, got 0. */
/* @@@ label1 Error TypeError: No matching call signature */
/* @@@ label2 Error TypeError: Expected 4 arguments, got 1. */
/* @@@ label2 Error TypeError: No matching call signature for sum(Int) */
/* @@@ label3 Error TypeError: Expected 4 arguments, got 3. */
/* @@@ label3 Error TypeError: No matching call signature for sum(Int, Int, Int) */
/* @@@ label4 Error TypeError: Expected 4 arguments, got 5. */
/* @@@ label4 Error TypeError: No matching call signature for sum(Int, Int, Int, Int, Int) */
