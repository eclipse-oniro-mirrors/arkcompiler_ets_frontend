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

type Loop<T, U extends Loop<T, U>> = {
    [P in keyof T]: U[P] extends boolean ? number : string;
};

/* @@? 16:24 Error TypeError: Circular type alias reference */
/* @@? 16:38 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 17:6 Error TypeError: Indexed signatures are not allowed. Use arrays instead! */
/* @@? 17:10 Error SyntaxError: Field type annotation expected. */
/* @@? 17:16 Error SyntaxError: Field type annotation expected. */
/* @@? 17:18 Error SyntaxError: Unexpected token ']'. */
/* @@? 17:18 Error SyntaxError: Field type annotation expected. */
/* @@? 17:19 Error SyntaxError: Unexpected token ':'. */
/* @@? 17:22 Error SyntaxError: Field type annotation expected. */
/* @@? 17:23 Error TypeError: Indexed signatures are not allowed. Use arrays instead! */
/* @@? 17:24 Error SyntaxError: Unexpected token ']'. */
/* @@? 17:26 Error SyntaxError: Unexpected token 'extends'. */
/* @@? 17:34 Error SyntaxError: boolean is a predefined type, cannot be used as an identifier */
/* @@? 17:42 Error SyntaxError: Unexpected token '?'. */
/* @@? 17:44 Error SyntaxError: number is a predefined type, cannot be used as an identifier */
/* @@? 17:51 Error SyntaxError: Unexpected token ':'. */
/* @@? 17:53 Error SyntaxError: string is a predefined type, cannot be used as an identifier */
