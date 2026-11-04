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

/*---
desc: Cannot modify an array or tuple content that has the readonly parameter.
name: spread_readonly_rest_parameter_modification
tags:
- negative
- compile-only
---*/

function foo(...p: readonly int[]): number {
    /* @@ label */p[1] = 1
    return 0
}

function main() {
    foo([1, 2, 3])
}


/* @@@ label Error Semantic error ESE0058: Cannot modify an array or tuple content that has the readonly parameter */