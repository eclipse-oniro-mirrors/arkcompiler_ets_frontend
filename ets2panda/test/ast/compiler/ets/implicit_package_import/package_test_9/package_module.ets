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

package mypack;

// That is the main file, from which we will compile full package

/* @@? package_module_with_semantic_error.ets:18:14 Error TypeError: Type '"I am a number, I promise..."' cannot be assigned to type 'Int' */

/* @@? package_module_with_syntax_error.ets:18:1 Error SyntaxError: Unexpected token '='. */
/* @@? package_module_with_syntax_error.ets:18:1 Error SyntaxError: Invalid package toplevel statement */
/* @@? package_module_with_syntax_error.ets:18:2 Error SyntaxError: Unexpected token ')'. */
/* @@? package_module_with_syntax_error.ets:18:2 Error SyntaxError: Invalid package toplevel statement */

/* @@? package_with_both_errors.ets:21:16 Error TypeError: Cannot find type 'good'. */
/* @@? package_with_both_errors.ets:21:21 Error SyntaxError: Unexpected token 'Day'. */
/* @@? package_with_both_errors.ets:21:21 Error TypeError: No static $_invoke method and static $_instantiate method in Day. Day() is not allowed. */
/* @@? package_with_both_errors.ets:21:21 Error TypeError: Type 'Day' has no call signatures. */
