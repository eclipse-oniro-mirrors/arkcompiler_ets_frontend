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

let g = 0

function bar() {
    let arr = [1, 2]
    return arr[g ? 1 : ""]
}

function baz() {
    g++
}

baz()
bar()


/* @@? 20:12 Error Semantic error ESE0127: No matching indexing signature for $_get(Int|"") */
/* @@? 20:16 Error Semantic error ESE0251: Cannot find index access method with the required signature. */
