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

function foo1(f: () => void) {
    try {

    } catch (e) {
        arktest.assertTrue(false, "expected}
}

class A {}

function foo2(let f = ((x:A {}

function foo3() {
    foo2(() => {}});
    foo2(() => {});
}

/* @@? 20:35 Error SyntaxError: Newline is not allowed in strings */
/* @@? 20:45 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 22:1 Error SyntaxError: Illegal start of CLASS expression. */
/* @@? 24:1 Error SyntaxError: Unexpected token, expected ')'. */
/* @@? 24:1 Error SyntaxError: Unexpected token '{'. */
/* @@? 24:1 Error SyntaxError: Unexpected token, expected ')'. */
/* @@? 24:1 Error SyntaxError: Expected '{', got 'let'. */
/* @@? 24:1 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 24:1 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 24:1 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 24:1 Error SyntaxError: Nested functions are not allowed. */
/* @@? 24:1 Error TypeError: Variable 'f' has already been declared. */
/* @@? 24:1 Error TypeError: Unresolved reference x */
/* @@? 26:1 Error SyntaxError: Nested functions are not allowed. */
/* @@? 27:18 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 27:18 Error SyntaxError: Unexpected token ')'. */
/* @@? 28:21 Error TypeError: Unresolved reference foo2 */
