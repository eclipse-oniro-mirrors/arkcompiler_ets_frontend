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

class Base {
    readonly id: int = 1;
}

class Mid extends Base {
    readonly code: int = 10;
}

class Derived extends Mid {
    name: string = 'derived';
    level: int = 0;
}

class Holder {
    item: Derived = { /* @@ label1 */id: 2, /* @@ label2 */code: 20, name: 'holder', level: 1 };
}

function buildDerived(): Derived {
    return { /* @@ label3 */id: 3, /* @@ label4 */code: 30, name: 'factory', level: 2 };
}

function main() {
    let obj: Derived = { /* @@ label5 */id: 4, /* @@ label6 */code: 40, name: 'test', level: 3 };
    let nested: Holder = { item: { /* @@ label7 */id: 5, /* @@ label8 */code: 50, name: 'nested', level: 4 } };
    let ok: Derived = { name: 'ok', level: 5 };
}

/* @@@ label1 Error Semantic error ESE4002: Cannot assign to a readonly field id */
/* @@@ label2 Error Semantic error ESE4002: Cannot assign to a readonly field code */
/* @@@ label3 Error Semantic error ESE4002: Cannot assign to a readonly field id */
/* @@@ label4 Error Semantic error ESE4002: Cannot assign to a readonly field code */
/* @@@ label5 Error Semantic error ESE4002: Cannot assign to a readonly field id */
/* @@@ label6 Error Semantic error ESE4002: Cannot assign to a readonly field code */
/* @@@ label7 Error Semantic error ESE4002: Cannot assign to a readonly field id */
/* @@@ label8 Error Semantic error ESE4002: Cannot assign to a readonly field code */