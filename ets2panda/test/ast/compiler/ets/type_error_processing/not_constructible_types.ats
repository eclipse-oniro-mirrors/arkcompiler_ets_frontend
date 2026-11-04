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

let a = new null()
new null()
new undefined(1, "123")
new never()
new []()

type A = undefined | null | int
new A()
type B = Object | string
new B
WeakSet [12] = new undefined()

// *NOTE* error has printed twice, that is known issue #20560



/* @@? 16:9 Error Semantic error ESE0290: Type 'null' is not constructible. */
/* @@? 17:1 Error Semantic error ESE0290: Type 'null' is not constructible. */
/* @@? 18:1 Error Semantic error ESE0290: Type 'undefined' is not constructible. */
/* @@? 19:1 Error Semantic error ESE0290: Type 'never' is not constructible. */
/* @@? 20:1 Error Semantic error ESE0292: This expression is not constructible. */
/* @@? 23:1 Error Semantic error ESE0300: The union type is not constructible. */
/* @@? 26:1 Error Syntax error ESY0318: Class cannot be used as object. */
/* @@? 26:1 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
/* @@? 26:16 Error Semantic error ESE0290: Type 'undefined' is not constructible. */