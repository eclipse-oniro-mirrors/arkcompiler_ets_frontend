/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

let f: { <T>(): T, g<U>(): U };

const a1 = f<number>['g'];
const a2 = (f<number>)['g'];

const a3 = f<number><number>;
const a4 = (f<number>)<number>;

/* @@? 16:8 Error Syntax error ESY177354: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 16:10 Error Syntax error ESY0227: Unexpected token '<'. */
/* @@? 16:12 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 16:12 Error Syntax error ESY0227: Unexpected token '>'. */
/* @@? 16:13 Error Syntax error ESY103145: Call signatures in object types are not supported. Use '$_invoke' method instead. */
/* @@? 16:18 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 18:12 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
/* @@? 19:12 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
/* @@? 21:22 Error Semantic error ESE0144: Type name 'number' used in the wrong context */
/* @@? 21:29 Error Syntax error ESY0227: Unexpected token ';'. */
/* @@? 22:24 Error Semantic error ESE0144: Type name 'number' used in the wrong context */
/* @@? 22:31 Error Syntax error ESY0227: Unexpected token ';'. */
