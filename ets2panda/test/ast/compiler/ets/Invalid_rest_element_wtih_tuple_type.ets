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

class TH<A, B, C> {
    static restMethod(...v: [number, B, C]) {
    }
}

function main() {
    let mc = new TH<number, string, boolean>
}

/* @@? 16:19 Error Semantic error ESE0127: No matching call signature for restMethod(...) */
/* @@? 17:23 Error Syntax error ESY0193: Rest parameter should be either array or tuple type. */
/* @@? 17:38 Error Semantic error ESE170021: Static members cannot reference class type parameters 'B' */
/* @@? 17:38 Error Semantic error ESE0371: Cannot find type 'B'. */
/* @@? 17:41 Error Semantic error ESE170021: Static members cannot reference class type parameters 'C' */
/* @@? 17:41 Error Semantic error ESE0371: Cannot find type 'C'. */
