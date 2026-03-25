/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

class A<T> {
    public x: T;

    constructor(x: T) {
        this.x = x;
    }
}

function foo<T, K>(b: A<K>) {
    let a: T;
}

function main() {
    let a = new A<number>(10);
    foo(a);
}

/* @@? 30:5 Error Semantic error ESE0127: No matching call signature for foo(A<Double>) */
/* @@? 30:5 Error Semantic error ESE8745: Type parameter 'T' can not be inferred for the call to 'foo'. Specify the type argument explicitly. */
