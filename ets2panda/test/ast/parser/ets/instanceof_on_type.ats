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

class X {}
type B = X

let a = (new X()) instanceof Object
let b = (new X()) instanceof X
let c = X instanceof Object
let d = X instanceof X
let f = B instanceof Object


/* @@? 19:9 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@? 20:9 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@? 21:9 Error Syntax error ESY0310: The left operand of 'instanceof' operator cannot be a type. */
/* @@? 21:9 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@? 22:9 Error Syntax error ESY0310: The left operand of 'instanceof' operator cannot be a type. */
/* @@? 22:9 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@? 23:9 Error Syntax error ESY0310: The left operand of 'instanceof' operator cannot be a type. */
/* @@? 23:9 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
