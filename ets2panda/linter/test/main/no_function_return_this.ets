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

let t1 = Function('return this')(); // error

let t2 = Function("return this")(); // error

let t3 = Function(`return this`)(); // error

let t4 = new Function('return this')(); // error

let fn = Function('return this'); // error
let t5 = fn();

let t6 = (Function)('return this')(); // error

let t7 = (globalThis.Function)('return this')(); // error

let t8 = (0, Function)('return this')(); // error

let t9 = (Function.bind(null))('return this')(); // error

const body = "return this";
let t10 = Function(body)(); // error

let a1 = Function('return 123')(); // legal

let a2 = Function('return window')(); // legal

let a3 = Function('x', 'return this')(); // legal

const a4 = Function; // legal

let a5 = Function("return somethingElse")(); // legal

let a7 = SomeOtherConstructor('return this'); // legal