/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

/* @@? 16:16 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 16:18 Error Syntax error ESY0227: Unexpected token '2'. */
/* @@? 16:18 Error Syntax error ESY0229: Unexpected token, expected ',' or ']'. */
/* @@? 16:19 Error Syntax error ESY0227: Unexpected token ']'. */
/* @@? 18:3 Error Syntax error ESY0227: Unexpected token '...'. */
/* @@? 18:3 Error Syntax error ESY0229: Unexpected token, expected ',' or ']'. */
/* @@? 18:6 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 18:8 Error Syntax error ESY0227: Unexpected token '55'. */
/* @@? 18:10 Error Syntax error ESY0227: Unexpected token ']'. */
/* @@? 20:5 Error Semantic error ESE0143: Unresolved reference x */
/* @@? 20:6 Error Syntax error ESY0227: Unexpected token '...'. */
/* @@? 20:6 Error Syntax error ESY0229: Unexpected token, expected ',' or ']'. */
/* @@? 20:9 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 20:11 Error Syntax error ESY0227: Unexpected token '78'. */
/* @@? 20:13 Error Syntax error ESY0227: Unexpected token ']'. */
/* @@? 22:11 Error Syntax error ESY0227: Unexpected token '=>'. */
/* @@? 22:11 Error Syntax error ESY0016: Unexpected token. */
/* @@? 24:5 Error Semantic error ESE0351: Variable 'a' has already been declared. */
/* @@? 24:18 Error Semantic error ESE0132: The type of parameter 'y' cannot be inferred */
/* @@? 24:19 Error Syntax error ESY0230: Expected '=>', got '...'. */
/* @@? 24:19 Error Syntax error ESY0227: Unexpected token '...'. */
/* @@? 24:19 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 24:24 Error Syntax error ESY0224: Identifier expected, got ')'. */
/* @@? 24:32 Error Syntax error ESY0227: Unexpected token '=>'. */
/* @@? 24:35 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 26:5 Error Semantic error ESE0143: Unresolved reference x */
/* @@? 26:7 Error Syntax error ESY0227: Unexpected token '||='. */
/* @@? 28:5 Error Semantic error ESE0351: Variable 'a' has already been declared. */
/* @@? 28:20 Error Syntax error ESY0230: Expected '}', got ')'. */
/* @@? 30:5 Error Semantic error ESE0351: Variable 'a' has already been declared. */
/* @@? 30:13 Error Syntax error ESY0226: Import declarations can only be used on the top level and before any other declaration, top level statement or directive. */
/* @@? 30:13 Error Syntax error ESY0138: Invalid Type. */
/* @@? 30:13 Error Syntax error ESY0227: Unexpected token 'import'. */
/* @@? 32:10 Error Semantic error ESE0351: Variable 'f' has already been declared. */
/* @@? 33:5 Error Semantic error ESE0201: Call to 'super' must be first statement in constructor */
/* @@? 33:10 Error Syntax error ESY0061: Unexpected super keyword. */
/* @@? 36:1 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
/* @@? 36:8 Error Syntax error ESY0228: Unexpected token, expected ']'. */
/* @@? 38:5 Error Semantic error ESE0351: Variable 'a' has already been declared. */
/* @@? 38:17 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 38:17 Error Syntax error ESY0229: Unexpected token, expected ',' or ']'. */
