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

let a = [1, ..., 2];

[a..., 55] = f();

[...x..., 78]

let f = 8 => 7;

let a = (x: int, y..., ): void => {};

if (x ||= 10) {}

let a = `The ${a+10)}`;

let a = new import(10);

function f(x: int): void {
    super(x);
}

a?.[1+2);

let a = [1, 2, 3);
/* @@? 1:1 Error TypeError: Invalid type reference. */
/* @@? 16:16 Error SyntaxError: Unexpected token ','. */
/* @@? 16:18 Error SyntaxError: Unexpected token, expected ',' or ']'. */
/* @@? 16:18 Error SyntaxError: Unexpected token '2'. */
/* @@? 16:19 Error SyntaxError: Unexpected token ']'. */
/* @@? 18:3 Error SyntaxError: Unexpected token, expected ',' or ']'. */
/* @@? 18:3 Error SyntaxError: Unexpected token '...'. */
/* @@? 18:6 Error SyntaxError: Unexpected token ','. */
/* @@? 18:8 Error SyntaxError: Unexpected token '55'. */
/* @@? 18:10 Error SyntaxError: Unexpected token ']'. */
/* @@? 20:5 Error TypeError: Unresolved reference x */
/* @@? 20:6 Error SyntaxError: Unexpected token, expected ',' or ']'. */
/* @@? 20:6 Error SyntaxError: Unexpected token '...'. */
/* @@? 20:9 Error SyntaxError: Unexpected token ','. */
/* @@? 20:11 Error SyntaxError: Unexpected token '78'. */
/* @@? 20:13 Error SyntaxError: Unexpected token ']'. */
/* @@? 22:11 Error SyntaxError: Unexpected token '=>'. */
/* @@? 22:11 Error SyntaxError: Unexpected token. */
/* @@? 24:5 Error TypeError: Variable 'a' has already been declared. */
/* @@? 24:18 Error TypeError: The type of parameter 'y' cannot be inferred */
/* @@? 24:19 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 24:19 Error SyntaxError: Expected '=>', got '...'. */
/* @@? 24:19 Error SyntaxError: Unexpected token '...'. */
/* @@? 24:24 Error SyntaxError: Identifier expected, got ')'. */
/* @@? 24:32 Error SyntaxError: Unexpected token '=>'. */
/* @@? 24:35 Error SyntaxError: Unexpected token '{'. */
/* @@? 26:7 Error SyntaxError: Unexpected token '||='. */
/* @@? 28:5 Error TypeError: Variable 'a' has already been declared. */
/* @@? 28:20 Error SyntaxError: Expected '}', got ')'. */
/* @@? 30:5 Error TypeError: Variable 'a' has already been declared. */
/* @@? 30:13 Error SyntaxError: Import declarations can only be used on the top level and before any other declaration, top level statement or directive. */
/* @@? 30:13 Error SyntaxError: Unexpected token 'import'. */
/* @@? 30:13 Error SyntaxError: Invalid Type. */
/* @@? 30:13 Error SyntaxError: Unexpected token 'import'. */
/* @@? 30:13 Error TypeError: Invalid type reference. */
/* @@? 32:10 Error TypeError: Variable 'f' has already been declared. */
/* @@? 33:5 Error TypeError: Call to 'super' must be first statement in constructor */
/* @@? 33:5 Error TypeError: Expected 0 arguments, got 1. */
/* @@? 33:5 Error TypeError: No matching call signature for std.core.Object(Int) */
/* @@? 33:10 Error SyntaxError: Unexpected super keyword. */
/* @@? 36:1 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 36:8 Error SyntaxError: Unexpected token, expected ']'. */
/* @@? 38:5 Error TypeError: Variable 'a' has already been declared. */
/* @@? 38:17 Error SyntaxError: Unexpected token ')'. */
/* @@? 38:17 Error SyntaxError: Unexpected token, expected ',' or ']'. */
