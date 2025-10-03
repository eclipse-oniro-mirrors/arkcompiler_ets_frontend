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
    i: number = 1;
    j: string = "somestr";
    k: number = 10;
}

interface I{
    i: number;
    j: string;
    k: number;
}

function main() {
    let x: Required<A> = /* @@ label1 */{i: 1.9,j :"somestr"};
    let x2: Required<A> = /* @@ label2 */{i: 1.9};
    let x3: Required<I> = /* @@ label3 */{i: 1.9,j :"somestr"};
    let x4: Required<I> = /* @@ label4 */{i: 1.9};
}


/* @@@ label1 Error Semantic error ESE0226: Class property 'k' needs to be initialized for Required<A>. */
/* @@@ label2 Error Semantic error ESE0226: Class property 'j' needs to be initialized for Required<A>. */
/* @@@ label2 Error Semantic error ESE0226: Class property 'k' needs to be initialized for Required<A>. */
/* @@@ label3 Error Semantic error ESE0400: Non-optional property 'k' in type 'Required<I>' is missing in object literal. */
/* @@@ label3 Error Semantic error ESE0226: Class property 'k' needs to be initialized for Required<I>. */
/* @@@ label4 Error Semantic error ESE0400: Non-optional property 'j' in type 'Required<I>' is missing in object literal. */
/* @@@ label4 Error Semantic error ESE0400: Non-optional property 'k' in type 'Required<I>' is missing in object literal. */
/* @@@ label4 Error Semantic error ESE0226: Class property 'j' needs to be initialized for Required<I>. */
/* @@@ label4 Error Semantic error ESE0226: Class property 'k' needs to be initialized for Required<I>. */