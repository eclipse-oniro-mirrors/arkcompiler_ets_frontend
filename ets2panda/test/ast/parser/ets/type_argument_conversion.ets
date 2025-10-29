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

class X {}

class Y {}

class A<T> {
    private t: T;

    public constructor(t: T) {
        this.t = t;
    }
}

function main(): int {
    let v: A<A<X>[]> = new A<A<X>[]>(new A<Y>[0]) // A<X>[] is not A<Y>[]
    return 0;
}


/* @@? 29:24 Error Semantic error ESE0127: No matching construct signature for type_argument_conversion.A(Array<A<Y>>) */
/* @@? 29:38 Error Semantic error ESE0270: No Matching Parameterless Constructor, parameter count 1 */
/* @@? 29:38 Error Semantic error ESE0125: No matching parameterless constructor */
/* @@? 29:38 Error Semantic error ESE0137: Signature is not available here. */
/* @@? 29:38 Error Semantic error ESE0046: Type 'Array<A<Y>>' is not compatible with type 'Array<A<X>>' at index 1 */