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

class Test {
    foo() {}

    bar() {
        this.baz(this, "*1*");

        const t = new Test()
        this.baz(t, "*2*");
    }

    baz(a: Test, k: keyof Test) {}
}


/* @@? 20:9 Error Semantic error ESE0127: No matching call signature for baz(Test, "*1*") */
/* @@? 20:24 Error Semantic error ESE0046: Type '"*1*"' is not compatible with type '"foo"|"bar"|"baz"' at index 2 */
/* @@? 23:9 Error Semantic error ESE0127: No matching call signature for baz(Test, "*2*") */
/* @@? 23:21 Error Semantic error ESE0046: Type '"*2*"' is not compatible with type '"foo"|"bar"|"baz"' at index 2 */
/* @@? 26:31 Error Semantic error ESE0379: Circular type of reference */