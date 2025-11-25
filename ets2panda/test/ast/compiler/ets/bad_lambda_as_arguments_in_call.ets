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

interface Base10 {
    value: string | number,
    type: string
}

const numPrimitive = 123;
const numObject = new Number(123);
const strPrimitive = 'test';
const strObject = new String('test');
const set = new Set<number | string>([numPrimitive, numObject, strPrimitive, strObject]);
const visited: Base10[] = [];
/* @@ label1 */set.forEach((/* @@ label2 */value) : void => {
    visited.push({ value: value.value.Of(), type: typeof value});
});

/* @@ label3 */[0, 1, 2].forEach(()=>{/* @@ label4 */({})});

/* @@@ label1 Error Semantic error ESE0127: No matching call signature for forEach(((p1: *ERROR_TYPE*) => void)) */
/* @@@ label2 Error Semantic error ESE0132: The type of parameter 'value' cannot be inferred */
/* @@@ label3 Error Semantic error ESE0127: No matching call signature for forEach((() => void)) */
/* @@@ label4 Error Semantic error ESE0062: need to specify target type for class composite */
