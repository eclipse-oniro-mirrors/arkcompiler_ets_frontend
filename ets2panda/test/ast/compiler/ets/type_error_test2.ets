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

let f:(c:string, ...abe])=>void = (c:be ...abe])=>{}

/* @@? 16:18 Error Syntax error ESY0193: Rest parameter should be either array or tuple type. */
/* @@? 16:24 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 16:24 Error Syntax error ESY0067: Rest parameter must be the last formal parameter. */
/* @@? 16:35 Error Semantic error ESE0318: Type '((p1: *ERROR_TYPE*) => void)' cannot be assigned to type '((p1: String, ...p2: *ERROR_TYPE*) => void)' */
/* @@? 16:38 Error Semantic error ESE0371: Cannot find type 'be'. */
/* @@? 16:41 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 16:41 Error Syntax error ESY0230: Expected '=>', got '...'. */
/* @@? 16:41 Error Syntax error ESY0227: Unexpected token '...'. */
/* @@? 16:44 Error Syntax error ESY0227: Unexpected token 'abe'. */
/* @@? 16:44 Error Semantic error ESE0143: Unresolved reference abe */
/* @@? 16:47 Error Syntax error ESY0227: Unexpected token ']'. */
/* @@? 16:48 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 16:49 Error Syntax error ESY0227: Unexpected token '=>'. */
