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

function foo(a: "aa"|"bb") {
    return a
}

let x = ["aa", "bb", "aa"] // type of x is FixedArray<string>
let y = foo(x)


/* @@? 21:9 Error Semantic error ESE0127: No matching call signature for foo(Array<String>) */
/* @@? 21:13 Error Semantic error ESE0046: Type 'Array<String>' is not compatible with type '"aa"|"bb"' at index 1 */