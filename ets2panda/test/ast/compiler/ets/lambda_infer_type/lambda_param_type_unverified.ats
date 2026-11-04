/**
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

function foo(f1: (f2: (p?: number) => void) => void) {
    let fn = (p?: number) => {
        console.log((p ?? 0) + 123)
    }
    f1(fn)
}

function test() {
    foo((f2: (p: string) => void) => {
        f2("a")
    })
}

/* @@? 24:5 Error Semantic error ESE0127: No matching call signature for foo(((p1: ((p1: String) => undefined)) => undefined)) */
/* @@? 24:9 Error Semantic error ESE0046: Type '((p1: ((p1: String) => undefined)) => undefined)' is not compatible with type '((p1: ((p1: Double|undefined) => undefined)) => undefined)' at index 1 */
