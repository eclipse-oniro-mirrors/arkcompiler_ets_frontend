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

let a: number;
let b: string;
let c: boolean;
let d: void;
let e: null;
let f: undefined;
let func3: (f: (a: number, b: string) => FixedArray<number>): FixedArray<number> => (a: number, b: boolean) => true;
// let func3: (f: (a: number, b: string) => FixedArray<number>): FixedArray<number>) => (a: number, b: boolean) => true;


/* @@? 19:8 Error Semantic error ESE0232: 'void' used as type annotation. */
/* @@? 22:61 Error Syntax error ESY0228: Unexpected token, expected '=>'. */
/* @@? 22:61 Error Syntax error ESY0227: Unexpected token '=>'. */
/* @@? 22:63 Error Syntax error ESY0227: Unexpected token 'FixedArray'. */
/* @@? 22:82 Error Syntax error ESY0227: Unexpected token '=>'. */
/* @@? 22:82 Error Syntax error ESY0016: Unexpected token. */
