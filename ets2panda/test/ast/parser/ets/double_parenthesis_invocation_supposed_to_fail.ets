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

interface Test {
    proc()  // passes
    proc()()  // throws SyntaxError
}

/* @@? 18:5 Error TypeError: Function proc is already declared. */
/* @@? 18:11 Error SyntaxError: Unexpected token '('. */
/* @@? 18:11 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 18:11 Error SyntaxError: Unexpected token, expected 'private' or identifier. */
/* @@? 18:12 Error SyntaxError: Identifier expected. */
/* @@? 18:12 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 18:12 Error SyntaxError: Unexpected token, expected 'private' or identifier. */
/* @@? 19:1 Error SyntaxError: Identifier expected. */
