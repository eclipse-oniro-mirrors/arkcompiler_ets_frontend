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

/* @@? 20:16 Error Semantic error ESE0127: No matching call signature for sum() */
/* @@? 21:16 Error Semantic error ESE0127: No matching call signature for sum(Int) */
/* @@? 22:16 Error Semantic error ESE0127: No matching call signature for sum(Int, Int, Int) */
/* @@? 23:16 Error Semantic error ESE0127: No matching call signature for sum(Int, Int, Int, Int, Int) */
