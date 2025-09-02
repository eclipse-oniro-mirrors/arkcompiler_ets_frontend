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

declare interface A {
    [index : string]: string
}

/* @@? 17:14 Error SyntaxError: Index type must be number in index signature. */
/* @@? 17:14 Error SyntaxError: ] expected in index signature. */
/* @@? 17:20 Error SyntaxError: An index signature must have a type annotation. */
/* @@? 17:20 Error SyntaxError: Expected ':', got ']'. */
/* @@? 17:20 Error SyntaxError: Invalid Type. */
/* @@? 17:20 Error SyntaxError: Return type of index signature from exported class or interface need to be identifier. */
/* @@? 17:20 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 17:20 Error SyntaxError: Identifier expected, got 'end of stream'. */
/* @@? 17:21 Error SyntaxError: Identifier expected. */
/* @@? 17:21 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 17:21 Error SyntaxError: Identifier expected, got 'end of stream'. */
/* @@? 18:1 Error SyntaxError: Invalid Type. */
