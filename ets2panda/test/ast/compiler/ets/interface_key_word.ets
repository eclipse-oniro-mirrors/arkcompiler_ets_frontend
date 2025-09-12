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

class CCC {}

interface I {
    foo() : boolean;
    new() : CCC;
    enum() : int;
    await() : boolean;
    async() : number;
}

/* @@? 20:5 Error SyntaxError: Constructor signatures are not supported in interfaces, use methods instead! */
/* @@? 20:5 Error SyntaxError: Identifier expected, got 'new'. */
/* @@? 20:9 Error SyntaxError: Interface fields must have type annotation. */
/* @@? 20:11 Error SyntaxError: Invalid Type. */
/* @@? 20:11 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 20:11 Error SyntaxError: Identifier expected, got 'end of stream'. */
/* @@? 20:16 Error SyntaxError: Interface fields must have type annotation. */
/* @@? 21:5 Error SyntaxError: Invalid Type. */
/* @@? 21:5 Error SyntaxError: Identifier expected, got 'enum'. */
/* @@? 21:10 Error SyntaxError: Interface fields must have type annotation. */
/* @@? 21:12 Error SyntaxError: Invalid Type. */
/* @@? 21:12 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 21:12 Error SyntaxError: Identifier expected, got 'end of stream'. */
/* @@? 21:14 Error SyntaxError: Identifier expected. */
/* @@? 21:14 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 21:14 Error SyntaxError: Identifier expected, got 'int'. */
/* @@? 21:17 Error SyntaxError: Identifier expected. */
/* @@? 22:5 Error SyntaxError: Identifier expected, got 'await'. */
/* @@? 22:11 Error SyntaxError: Interface fields must have type annotation. */
/* @@? 22:13 Error SyntaxError: Invalid Type. */
/* @@? 22:13 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 22:13 Error SyntaxError: Identifier expected, got 'end of stream'. */
/* @@? 22:15 Error SyntaxError: Identifier expected. */
/* @@? 22:15 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 22:15 Error SyntaxError: Identifier expected, got 'boolean'. */
/* @@? 22:22 Error SyntaxError: Identifier expected. */
/* @@? 23:5 Error SyntaxError: Identifier expected, got 'async'. */
/* @@? 23:11 Error SyntaxError: Interface fields must have type annotation. */
/* @@? 23:13 Error SyntaxError: Invalid Type. */
/* @@? 23:13 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 23:13 Error SyntaxError: Identifier expected, got 'end of stream'. */
/* @@? 23:21 Error SyntaxError: Interface fields must have type annotation. */
/* @@? 24:1 Error SyntaxError: Invalid Type. */
