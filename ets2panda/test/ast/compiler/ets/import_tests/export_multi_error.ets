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

export { foo };

function bar(): int {
    return 1;
}

/*-----------------*/
function foo2(): void {}
class TestClass {}

export {foo2}
export type {TestClass as foo2}

/*-----------------*/
export default function foo3(): void {}

export {
  foo3 as bar
}

/*-----------------*/
export class A {}

export type {A as AA};

/*-----------------*/
interface I {}

export type {I}
export type MyI = I
export type {MyI}
/*-----------------*/

function f(){
    return 1;
}

export type {f}
/*-----------------*/

export default function TestFunc(): void {}
export default function TestFuncToo(): void {}

/*-----------------*/

/* @@? 46:13 Warning Warning W0000: Duplicated export aliases for 'MyI'. */
/* @@? 16:10 Error Syntax error ESY0279: Cannot find name 'foo' to export. */
/* @@? 27:14 Error Syntax error ESY0344: Cannot export two different names with the same export alias name 'foo2'. */
/* @@? 56:16 Error Semantic error ESE0374: Only one default export is allowed in a module */
/* @@? 57:16 Error Semantic error ESE0374: Only one default export is allowed in a module */