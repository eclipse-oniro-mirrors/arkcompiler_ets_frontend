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

interface Obj {}
const object1: Obj = {};

console.log(Objct.keys(object1));

/* @@? 19:13 Error TypeError: Class name can't be the argument of function or method. */
/* @@? 19:13 Error TypeError: Class or interface or enum 'Obj' cannot be used as object */
/* @@? 19:16 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 19:16 Error SyntaxError: Unexpected token 'ct'. */
/* @@? 19:16 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 19:16 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 19:16 Error TypeError: Unresolved reference ct */
/* @@? 19:33 Error SyntaxError: Unexpected token ')'. */