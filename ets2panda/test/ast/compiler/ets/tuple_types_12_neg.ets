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


const index1: Int = 0;
const index2: Int = 1;

function main(): void {
    let tuple: [number, number] = [0, 0];
    tuple[index1] = 5;
    tuple[index2] = 3;
}


/* @@? 22:11 Error Semantic error ESE0200: Only integer type allowed for element access on tuples. */
/* @@? 23:11 Error Semantic error ESE0200: Only integer type allowed for element access on tuples. */
