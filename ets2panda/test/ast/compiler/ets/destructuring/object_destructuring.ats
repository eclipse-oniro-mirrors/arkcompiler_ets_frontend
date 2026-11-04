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
  let obj = {
    str: "abc",
    x: 1,
    y: 2
  };

  let {str, x, y} = obj;
}

/* @@? 17:7 Error Semantic error ESE0174: Cannot infer type for obj because class composite needs an explicit target type */
/* @@? 23:7 Error Syntax error ESY0224: Identifier expected, got '{'. */
/* @@? 23:8 Error Syntax error ESY0105: Variable must be initialized or it's type must be declared. */
/* @@? 23:8 Error Syntax error ESY0227: Unexpected token 'str'. */
/* @@? 23:8 Error Semantic error ESE0143: Unresolved reference str */
/* @@? 23:11 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 23:13 Error Syntax error ESY0227: Unexpected token 'x'. */
/* @@? 23:13 Error Semantic error ESE0143: Unresolved reference x */
/* @@? 23:14 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 23:16 Error Syntax error ESY0227: Unexpected token 'y'. */
/* @@? 23:16 Error Semantic error ESE0143: Unresolved reference y */
/* @@? 23:19 Error Syntax error ESY0227: Unexpected token '='. */
/* @@? 23:21 Error Syntax error ESY0227: Unexpected token 'obj'. */
/* @@? 23:21 Error Semantic error ESE0143: Unresolved reference obj */
/* @@? 24:1 Error Syntax error ESY0227: Unexpected token '}'. */
