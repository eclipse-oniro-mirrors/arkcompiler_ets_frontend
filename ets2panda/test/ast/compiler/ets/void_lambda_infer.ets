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

let voidTest1 = () => {
    try {
    } catch (e) {
        throw new Error()
    }
}

let voidTest2 = () => {
    try {
        throw new Error()
    } catch (e) {
    }
}

let voidTest3 = () => {
    for (let i = 0; i < 10; i++) {
        try {
            if (i === 5) throw new Error()
        } catch (e) {
            break
        }
    }
}

let voidTest4 = () => {
    try {
        throw new Error()
    } catch (e) {
        if (false) {
            throw new Error()
        }
    }
}

let voidTest5 = () => {
    try {
        throw new Error()
    } catch (e) {
        return
        throw new Error()
    }
}

let voidTest6 = () => {
    try {
        throw new Error()
    } catch (e) {
        if (true) {
            throw new Error()
        }
        return
    }
}

let voidTest7 = () => {
    try {
        if (false) {
            return
        }
        throw new Error()
    } catch (e) {
        throw new Error()
    }
}

// Test if a is ()=>void, then it will CTE
function foo(a: ()=>never) {

}

foo(voidTest1);
foo(voidTest2);
foo(voidTest3);
foo(voidTest4);
foo(voidTest5);
foo(voidTest6);
foo(voidTest7);

/* @@? 86:1 Error Semantic error ESE0127: No matching call signature for foo((() => void)) */
/* @@? 86:5 Error Semantic error ESE0046: Type '(() => void)' is not compatible with type '(() => never)' at index 1 */
/* @@? 87:1 Error Semantic error ESE0127: No matching call signature for foo((() => void)) */
/* @@? 87:5 Error Semantic error ESE0046: Type '(() => void)' is not compatible with type '(() => never)' at index 1 */
/* @@? 88:1 Error Semantic error ESE0127: No matching call signature for foo((() => void)) */
/* @@? 88:5 Error Semantic error ESE0046: Type '(() => void)' is not compatible with type '(() => never)' at index 1 */
/* @@? 89:1 Error Semantic error ESE0127: No matching call signature for foo((() => void)) */
/* @@? 89:5 Error Semantic error ESE0046: Type '(() => void)' is not compatible with type '(() => never)' at index 1 */
/* @@? 90:1 Error Semantic error ESE0127: No matching call signature for foo((() => void)) */
/* @@? 90:5 Error Semantic error ESE0046: Type '(() => void)' is not compatible with type '(() => never)' at index 1 */
/* @@? 91:1 Error Semantic error ESE0127: No matching call signature for foo((() => void)) */
/* @@? 91:5 Error Semantic error ESE0046: Type '(() => void)' is not compatible with type '(() => never)' at index 1 */
/* @@? 92:1 Error Semantic error ESE0127: No matching call signature for foo((() => void)) */
/* @@? 92:5 Error Semantic error ESE0046: Type '(() => void)' is not compatible with type '(() => never)' at index 1 */
