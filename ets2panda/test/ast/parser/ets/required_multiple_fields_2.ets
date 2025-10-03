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
    static k: number = 10;
}

function main() {
    let x: Required<A> = {i: 1.9, j :"somestr"};
    let x2: Required<A> = {i: 1.9, j :"somestr", /* @@ label1 */K :100};
    let x3: Required<A> = /* @@ label2 */{j :"somestr", j :"somestr"};
}


/* @@@ label1 Error Semantic error ESE0065: type A has no property named K */
/* @@@ label2 Error Semantic error ESE0226: Class property 'i' needs to be initialized for Required<A>. */