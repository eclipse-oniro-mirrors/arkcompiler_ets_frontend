/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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


/* @@? 18:5 Error Semantic error ESE0130: Function proc is already declared. */
/* @@? 18:11 Error Syntax error ESY0227: Unexpected token '('. */
/* @@? 18:11 Error Syntax error ESY0228: Unexpected token, expected ','. */
/* @@? 18:11 Error Syntax error ESY0224: Identifier expected, got 'end of stream'. */
/* @@? 18:12 Error Syntax error ESY0122: Identifier expected. */
/* @@? 18:12 Error Syntax error ESY0228: Unexpected token, expected ','. */
/* @@? 18:12 Error Syntax error ESY0224: Identifier expected, got 'end of stream'. */
/* @@? 19:1 Error Syntax error ESY0122: Identifier expected. */
/* @@? 18:5 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '() => undefined' will never be selected for a call, as the preceding entity with the signature '() => undefined' prevails. */
