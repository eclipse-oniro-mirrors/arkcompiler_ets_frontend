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


/* @@? 16:9 Error TypeError: Type 'null' is not constructible. */
/* @@? 17:1 Error TypeError: Type 'null' is not constructible. */
/* @@? 18:1 Error TypeError: Type 'undefined' is not constructible. */
/* @@? 19:1 Error TypeError: Type 'never' is not constructible. */
/* @@? 20:1 Error TypeError: This expression is not constructible. */
/* @@? 23:1 Error TypeError: The union type is not constructible. */
/* @@? 26:1 Error TypeError: Object type doesn't have proper index access method. */
/* @@? 26:1 Error SyntaxError: Class cannot be used as object. */
/* @@? 26:16 Error TypeError: Type 'undefined' is not constructible. */
