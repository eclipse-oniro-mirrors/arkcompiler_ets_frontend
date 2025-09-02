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

interface A {
    y : string = ""
    struct B {
        x : string = ""
    }
}

/* @@? 17:16 Error SyntaxError: Interface member initialization is prohibited. */
/* @@? 17:18 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 17:18 Error SyntaxError: Identifier expected, got 'string literal'. */
/* @@? 18:12 Error TypeError: Cannot find type 'B'. */
/* @@? 18:14 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 18:14 Error SyntaxError: Identifier expected, got 'end of stream'. */
/* @@? 19:20 Error SyntaxError: Interface member initialization is prohibited. */
/* @@? 19:22 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 19:22 Error SyntaxError: Identifier expected, got 'string literal'. */
/* @@? 20:5 Error SyntaxError: Identifier expected. */
/* @@? 21:1 Error SyntaxError: Unexpected token '}'. */
