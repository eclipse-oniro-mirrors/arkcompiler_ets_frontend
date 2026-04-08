/*
* Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

function sum(...numbers: [number, number, number]): number {
    return /* @@ label1 */numbers[0] + /* @@ label2 */numbers[1] + /* @@ label3 */numbers[2]
}

/* @@ label */sum(10, 20, 30, 40)

/* @@@ label1 Warning Warning W670214: This cast can be excessive because it always succeeds */
/* @@@ label2 Warning Warning W670214: This cast can be excessive because it always succeeds */
/* @@@ label3 Warning Warning W670214: This cast can be excessive because it always succeeds */
/* @@@ label Error Semantic error ESE0127: No matching call signature for sum(Int, Int, Int, Int) */
