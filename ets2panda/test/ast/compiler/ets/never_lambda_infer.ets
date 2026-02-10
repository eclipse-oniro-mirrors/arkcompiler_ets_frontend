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

// Deliberately put `a:string` here because we expect error messages like: 
// No matching call signature for foo(((p1: String) => never)) instead of `foo(((p1: String) => void))`
let neverTest1 = (a: string) => {
    try {
        throw new Error()
    } catch (e) {
        throw new Error()
    }
}

// Deliberately put `a:string` here because we expect error messages like: 
// No matching call signature for foo(((p1: String) => never)) instead of `foo(((p1: String) => void))`
let neverTest2 = (a: string) => {
    try {
        throw new Error()
    } finally {
    }
}

// Deliberately put `a:string` here because we expect error messages like: 
// No matching call signature for foo(((p1: String) => never)) instead of `foo(((p1: String) => void))`
let neverTest3 = (a: string) => {
    try {
        if (true) throw new Error()
        throw new Error()
    } catch (e) {
        throw new Error()
    }
}

// Deliberately put `a:string` here because we expect error messages like: 
// No matching call signature for foo(((p1: String) => never)) instead of `foo(((p1: String) => void))`
let neverTest4 = (a: string) => {
    try {
        try {
            throw new Error()
        } catch (e) {
        }
        throw new Error()
    } catch (e) {
        throw new Error()
    }
}

function foo(a : ()=>void) {}
foo(neverTest1)
foo(neverTest2)
foo(neverTest3)
foo(neverTest4)

/* @@? 61:1 Error Semantic error ESE0127: No matching call signature for foo(((p1: String) => never)) */
/* @@? 61:5 Error Semantic error ESE0046: Type '((p1: String) => never)' is not compatible with type '(() => void)' at index 1 */
/* @@? 62:1 Error Semantic error ESE0127: No matching call signature for foo(((p1: String) => never)) */
/* @@? 62:5 Error Semantic error ESE0046: Type '((p1: String) => never)' is not compatible with type '(() => void)' at index 1 */
/* @@? 63:1 Error Semantic error ESE0127: No matching call signature for foo(((p1: String) => never)) */
/* @@? 63:5 Error Semantic error ESE0046: Type '((p1: String) => never)' is not compatible with type '(() => void)' at index 1 */
/* @@? 64:1 Error Semantic error ESE0127: No matching call signature for foo(((p1: String) => never)) */
/* @@? 64:5 Error Semantic error ESE0046: Type '((p1: String) => never)' is not compatible with type '(() => void)' at index 1 */
