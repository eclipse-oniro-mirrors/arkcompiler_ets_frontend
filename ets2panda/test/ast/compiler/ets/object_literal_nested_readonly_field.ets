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

class Inner {
    readonly value: int = 1;
}

class Outer {
    inner: Inner = { /* @@ label1 */value: 1 };
}

class Wrapper {
    outer: Outer = { inner: { /* @@ label2 */value: 2 } };
}

function buildOuter(): Outer {
    return { inner: { /* @@ label3 */value: 3 } };
}

function main() {
    let obj: Outer = { inner: { /* @@ label4 */value: 4 } };
    let nested: Wrapper = { outer: { inner: { /* @@ label5 */value: 5 } } };
    let ok: Outer = { inner: {} };
}

/* @@@ label1 Error Semantic error ESE4002: Cannot assign to a readonly field value */
/* @@@ label2 Error Semantic error ESE4002: Cannot assign to a readonly field value */
/* @@@ label3 Error Semantic error ESE4002: Cannot assign to a readonly field value */
/* @@@ label4 Error Semantic error ESE4002: Cannot assign to a readonly field value */
/* @@@ label5 Error Semantic error ESE4002: Cannot assign to a readonly field value */