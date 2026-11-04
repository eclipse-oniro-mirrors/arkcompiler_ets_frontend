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

function fooNonNullGen<T extends NonNullable<Object | null | undefined>>(x: T): T {
    return x;
}

function fooNonNullAnyGen<T extends NonNullable<Any>>(x: T): T {
    return x;
}

fooNonNullGen(undefined);
fooNonNullGen(null);
fooNonNullGen(new Object());
let v: Object | undefined = undefined
fooNonNullGen(v);
let l = fooNonNullGen<Object | undefined>(new Object());
let anyValue: Object | undefined = undefined
fooNonNullAnyGen(anyValue);
let anyResult = fooNonNullAnyGen<Object | undefined>(new Object());

function main(): void {}

/* @@? 24:1 Error Semantic error ESE0127: No matching call signature for fooNonNullGen(undefined) */
/* @@? 24:1 Error Semantic error ESE8745: Type parameter 'T' can not be inferred for the call to 'fooNonNullGen'. Specify the type argument explicitly. */
/* @@? 24:15 Error Semantic error ESE0046: Type 'undefined' is not compatible with type 'T' at index 1 */
/* @@? 25:1 Error Semantic error ESE0127: No matching call signature for fooNonNullGen(null) */
/* @@? 25:1 Error Semantic error ESE8745: Type parameter 'T' can not be inferred for the call to 'fooNonNullGen'. Specify the type argument explicitly. */
/* @@? 25:15 Error Semantic error ESE0046: Type 'null' is not compatible with type 'T' at index 1 */
/* @@? 28:1 Error Semantic error ESE0127: No matching call signature for fooNonNullGen(Object|undefined) */
/* @@? 28:1 Error Semantic error ESE8745: Type parameter 'T' can not be inferred for the call to 'fooNonNullGen'. Specify the type argument explicitly. */
/* @@? 28:15 Error Semantic error ESE0046: Type 'Object|undefined' is not compatible with type 'T' at index 1 */
/* @@? 29:9 Error Semantic error ESE0127: No matching call signature for fooNonNullGen(Object) */
/* @@? 29:9 Error Semantic error ESE0228: Type argument 'Object|undefined' should be a subtype of 'Object' constraint */
/* @@? 31:1 Error Semantic error ESE0127: No matching call signature for fooNonNullAnyGen(Object|undefined) */
/* @@? 31:1 Error Semantic error ESE8745: Type parameter 'T' can not be inferred for the call to 'fooNonNullAnyGen'. Specify the type argument explicitly. */
/* @@? 31:18 Error Semantic error ESE0046: Type 'Object|undefined' is not compatible with type 'T' at index 1 */
/* @@? 32:17 Error Semantic error ESE0127: No matching call signature for fooNonNullAnyGen(Object) */
/* @@? 32:17 Error Semantic error ESE0228: Type argument 'Object|undefined' should be a subtype of 'NonNullable<Any>' constraint */
