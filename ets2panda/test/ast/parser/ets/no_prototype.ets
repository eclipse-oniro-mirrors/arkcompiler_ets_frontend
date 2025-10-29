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

class C {}

C.prototype = Object.prototype

C.prototype = {
  m() {
    console.log("C.m()")
  }
}


/* @@? 18:3 Error Semantic error ESE0087: Property 'prototype' does not exist on type 'C' */
/* @@? 18:13 Error Syntax error ESY30818: Runtime prototype assignment is not supported because of static typing */
/* @@? 18:22 Error Semantic error ESE0087: Property 'prototype' does not exist on type 'Object' */
/* @@? 20:1 Error Syntax error ESY30818: Runtime prototype assignment is not supported because of static typing */
/* @@? 20:3 Error Semantic error ESE0087: Property 'prototype' does not exist on type 'C' */
/* @@? 20:13 Error Syntax error ESY30818: Runtime prototype assignment is not supported because of static typing */
/* @@? 21:4 Error Syntax error ESY0228: Unexpected token, expected ':'. */
/* @@? 21:5 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 21:7 Error Syntax error ESY0016: Unexpected token. */
/* @@? 22:12 Error Syntax error ESY0228: Unexpected token, expected ':'. */
/* @@? 24:1 Error Syntax error ESY0227: Unexpected token '}'. */