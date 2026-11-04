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

class A {
    readonly a: int = 1;
    readonly b: string = 'default';
    c: int = 0;
}

class Holder {
    item: A = { /* @@ label1 */a: 10, /* @@ label2 */b: 'holder', c: 1 };
}

function buildA(): A {
    return { /* @@ label3 */a: 20, /* @@ label4 */b: 'factory', c: 2 };
}

function main() {
    let first: A = { /* @@ label5 */a: 30, /* @@ label6 */b: 'override', c: 3 };
    let second: Holder = { item: { /* @@ label7 */a: 40, /* @@ label8 */b: 'nested', c: 4 } };
    let third: A = { c: 5 };
    let fourth: Holder = { item: { c: 6 } };
}

/* @@@ label1 Error Semantic error ESE4002: Cannot assign to a readonly field a */
/* @@@ label2 Error Semantic error ESE4002: Cannot assign to a readonly field b */
/* @@@ label3 Error Semantic error ESE4002: Cannot assign to a readonly field a */
/* @@@ label4 Error Semantic error ESE4002: Cannot assign to a readonly field b */
/* @@@ label5 Error Semantic error ESE4002: Cannot assign to a readonly field a */
/* @@@ label6 Error Semantic error ESE4002: Cannot assign to a readonly field b */
/* @@@ label7 Error Semantic error ESE4002: Cannot assign to a readonly field a */
/* @@@ label8 Error Semantic error ESE4002: Cannot assign to a readonly field b */
