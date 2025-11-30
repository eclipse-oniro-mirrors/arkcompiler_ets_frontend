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

export function foo<T eStends keyof A,P extends keyof B[T]>

/* @@? 16:17 Error TypeError: Only abstract or native methods can't have body. */
/* @@? 16:23 Error SyntaxError: Unexpected token, expected '>'. */
/* @@? 16:31 Error SyntaxError: Unexpected token, expected '('. */
/* @@? 16:38 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 16:41 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 16:41 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 16:41 Error SyntaxError: Unexpected token 'extends'. */
/* @@? 16:49 Error SyntaxError: Unexpected token 'keyof'. */
/* @@? 16:49 Error TypeError: Unresolved reference keyof */
/* @@? 16:55 Error SyntaxError: Unexpected token 'B'. */
/* @@? 16:55 Error TypeError: Unresolved reference B */
/* @@? 16:55 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 16:57 Error SyntaxError: Unexpected token, expected ']'. */
/* @@? 31:69 Error SyntaxError: Unexpected token 'end of stream'. */