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

function main(): int {
    let tuple: [number, number] = [0,5];
    tuple[/* @@ label */MathPartialSums.g_flag = 0] = 12; 
    return 0;
}

/* @@@ label Error TypeError: Unresolved reference MathPartialSums */
/* @@@ label Error TypeError: Only constant expression allowed for element access on tuples. */
