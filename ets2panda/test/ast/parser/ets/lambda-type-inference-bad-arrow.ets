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

function foo(callback: () => void): void {
}

function main(): void {
    foo((x(() => {
}

/* @@? 20:5 Error TypeError: No matching call signature for foo(...) */
/* @@? 20:10 Error TypeError: Unresolved reference x */
/* @@? 20:10 Error TypeError: This expression is not callable. */
/* @@? 29:70 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 29:70 Error SyntaxError: Unexpected token, expected ')'. */
/* @@? 29:70 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 29:70 Error SyntaxError: Expected '}', got 'end of stream'. */