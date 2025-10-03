/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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


function foo1 (x : int = 1) : readonly int[] {
    return [1, 2]
}

function foo2 (x : int = 1) : readonly [int, string] {
    return [1, "a"]
}

let arr = foo1()
/* @@ label */arr[0] = 1
let tuple = foo2()
/* @@ label1 */tuple[1] = "c"


/* @@@ label Error Semantic error ESE0250: Object type doesn't have proper index access method. */
/* @@@ label1 Error Semantic error ESE0058: Cannot modify an array or tuple content that has the readonly parameter */