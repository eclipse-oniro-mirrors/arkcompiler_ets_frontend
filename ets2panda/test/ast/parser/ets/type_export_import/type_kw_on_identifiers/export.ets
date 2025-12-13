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

export { A, B }
export type { C, D }
export { type E }
export { F, type G }
export { type H, type I }
export { J, K, type L }

export {type a as default};
export type { type /* @@ first */M, N }     // expect: CTE
export type { O, type /* @@ second */P }     // expect: CTE
export type { type /* @@ third */Q, type /* @@ fourth */R }     // expect: CTE

class A {}
class B {}
class C {}
class D {}
class E {}
class F {}
class G {}
class H {}
class I {}
class J {}
class K {}
class L {}
class M {}
class N {}
class O {}
class P {}
class Q {}
class R {}

let a = 0;



/* @@@ first Error Syntax error ESY135857: Type keyword set on both the specifier list and on the specifier itself. */
/* @@@ second Error Syntax error ESY135857: Type keyword set on both the specifier list and on the specifier itself. */
/* @@@ third Error Syntax error ESY135857: Type keyword set on both the specifier list and on the specifier itself. */
/* @@@ fourth Error Syntax error ESY135857: Type keyword set on both the specifier list and on the specifier itself. */