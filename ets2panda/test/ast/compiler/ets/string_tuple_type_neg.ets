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

type A = [n1:string, n2:string]


/* @@? 16:11 Error Semantic error ESE0371: Cannot find type 'n1'. */
/* @@? 16:13 Error Syntax error ESY0229: Unexpected token, expected ',' or ']'. */
/* @@? 16:13 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 16:14 Error Syntax error ESY0227: Unexpected token 'string'. */
/* @@? 16:14 Error Semantic error ESE0144: Type name 'string' used in the wrong context */
/* @@? 16:20 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 16:22 Error Syntax error ESY0227: Unexpected token 'n2'. */
/* @@? 16:25 Error Syntax error ESY0038: Label must be followed by a loop statement. */
/* @@? 16:25 Error Semantic error ESE0144: Type name 'string' used in the wrong context */
/* @@? 16:31 Error Syntax error ESY0227: Unexpected token ']'. */