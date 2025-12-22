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

function main() {
    let foo = <T>(p: T): NonNullable<<<<<<<<<<<<<<<<<<<<<<<<<<<T> =>p;
}


/* @@? 17:15 Error Syntax error ESY178094: Type cast syntax '<type>' is not supported, please use 'as' keyword instead */
/* @@? 17:20 Error Syntax error ESY0228: Unexpected token, expected ')'. */
/* @@? 17:22 Error Syntax error ESY0227: Unexpected token 'T'. */
/* @@? 17:22 Error Semantic error ESE0143: Unresolved reference T */
/* @@? 17:23 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 17:24 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 17:26 Error Syntax error ESY0227: Unexpected token 'NonNullable'. */
/* @@? 17:26 Error Semantic error ESE0143: Unresolved reference NonNullable */
/* @@? 17:37 Error Syntax error ESY0230: Expected '>', got '<<'. */
/* @@? 17:37 Error Syntax error ESY0230: Expected '<', got '<<'. */
/* @@? 17:37 Error Syntax error ESY0230: Expected '>', got '<<'. */
/* @@? 17:37 Error Syntax error ESY0230: Expected '<', got '<<'. */
/* @@? 17:39 Error Syntax error ESY0227: Unexpected token '<<'. */
/* @@? 17:43 Error Syntax error ESY0227: Unexpected token '<<'. */
/* @@? 17:47 Error Syntax error ESY0227: Unexpected token '<<'. */
/* @@? 17:51 Error Syntax error ESY0227: Unexpected token '<<'. */
/* @@? 17:55 Error Syntax error ESY0227: Unexpected token '<<'. */
/* @@? 17:59 Error Syntax error ESY0227: Unexpected token '<<'. */
/* @@? 17:63 Error Syntax error ESY178094: Type cast syntax '<type>' is not supported, please use 'as' keyword instead */
/* @@? 17:67 Error Syntax error ESY0227: Unexpected token '=>'. */
/* @@? 17:69 Error Syntax error ESY0227: Unexpected token 'p'. */
/* @@? 17:69 Error Semantic error ESE0143: Unresolved reference p */