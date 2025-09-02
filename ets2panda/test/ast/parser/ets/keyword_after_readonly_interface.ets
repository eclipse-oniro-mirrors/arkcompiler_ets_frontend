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
	readonly static a = 45;
}

/* @@? 17:2 Error SyntaxError: Identifier expected. */
/* @@? 17:11 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 17:11 Error SyntaxError: Identifier expected, got 'static'. */
/* @@? 17:20 Error SyntaxError: Interface fields must have type annotation. */
/* @@? 17:22 Error SyntaxError: Invalid Type. */
/* @@? 17:22 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 17:22 Error SyntaxError: Identifier expected, got 'number literal'. */
/* @@? 17:24 Error SyntaxError: Identifier expected. */
