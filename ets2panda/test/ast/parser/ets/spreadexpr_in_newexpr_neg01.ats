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

class A {
    fld: number[];
    constructor(...p: number[], x: int) {
        this.fld = p;
    }
}

let nums: number[] = [1.0, 2.0, 3.0];

function main() {
    let a2 = new A(...nums);
}

/* @@? 18:16 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 18:31 Error Syntax error ESY0067: Rest parameter must be the last formal parameter. */
/* @@? 18:39 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 18:41 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 19:9 Error Syntax error ESY0227: Unexpected token 'this'. */
/* @@? 19:13 Error Syntax error ESY0227: Unexpected token '.'. */
/* @@? 19:14 Error Semantic error ESE0351: Variable 'fld' has already been declared. */
/* @@? 19:20 Error Semantic error ESE0143: Unresolved reference p */
/* @@? 21:1 Error Syntax error ESY0227: Unexpected token '}'. */