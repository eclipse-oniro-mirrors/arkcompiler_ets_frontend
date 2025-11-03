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

function foo(...number: int[],): int {
    return number[0]
}

function foo2(number: int,): int {
    return number
}

function foo3(, number: int): int {
    return number
}

let a = [1,, 2,]
foo(...a,)
foo(a[0],)
foo(a[0], a[1], ...a,)
foo(a[0],,a[0])
foo(a[0],,)
foo(,a[0])
foo(a[0] a[1])
foo(,)

/* @@? 16:30 Error SyntaxError: Rest parameter must be the last formal parameter. */
/* @@? 24:15 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 24:15 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 28:12 Error SyntaxError: Unexpected token ','. */
/* @@? 28:14 Error SyntaxError: Unexpected token, expected ',' or ']'. */
/* @@? 28:14 Error SyntaxError: Unexpected token '2'. */
/* @@? 28:15 Error SyntaxError: Unexpected token ','. */
/* @@? 28:16 Error SyntaxError: Unexpected token ']'. */
/* @@? 30:1 Error TypeError: No matching call signature for foo(a[0]) */
/* @@? 30:5 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 31:1 Error TypeError: No matching call signature for foo(a[0], a[1], ...a) */
/* @@? 31:5 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 31:11 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 31:17 Error TypeError: Spread argument for the rest parameter can be only one. */
/* @@? 32:1 Error TypeError: No matching call signature for foo(a[0], ...) */
/* @@? 32:5 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 32:10 Error SyntaxError: Unexpected token ','. */
/* @@? 32:11 Error SyntaxError: Unexpected token 'a'. */
/* @@? 32:11 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 32:11 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 32:15 Error SyntaxError: Unexpected token ')'. */
/* @@? 33:1 Error TypeError: No matching call signature for foo(a[0], ...) */
/* @@? 33:5 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 33:10 Error SyntaxError: Unexpected token ','. */
/* @@? 34:5 Error SyntaxError: Unexpected token ','. */
/* @@? 34:6 Error SyntaxError: Unexpected token 'a'. */
/* @@? 34:6 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 34:6 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 34:10 Error SyntaxError: Unexpected token ')'. */
/* @@? 35:1 Error TypeError: No matching call signature for foo(a[0]) */
/* @@? 35:5 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 35:10 Error SyntaxError: Unexpected token 'a'. */
/* @@? 35:10 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 35:10 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 35:14 Error SyntaxError: Unexpected token ')'. */
/* @@? 36:5 Error SyntaxError: Unexpected token ','. */