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


func: readonly  (Y : object | long [] )  => [  ]

/* @@? 17:7 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 17:7 Error TypeError: Unresolved reference readonly */
/* @@? 17:20 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 17:20 Error SyntaxError: Unexpected token ':'. */
/* @@? 17:22 Error SyntaxError: Unexpected token 'object'. */
/* @@? 17:22 Error TypeError: Type name 'object' used in the wrong context */
/* @@? 17:31 Error SyntaxError: Unexpected token 'long'. */
/* @@? 17:39 Error SyntaxError: Unexpected token ')'. */
/* @@? 17:42 Error SyntaxError: Unexpected token '=>'. */