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

function main(): void {
  let length = 10;
  for (let i = 0; i < ( let f = ((x: A | null | undefined) => x as A); f(new A());
); i += 3) {
  }
}


/* @@? 18:19 Error Semantic error ESE0114: Bad operand type, the types of the operands must be numeric, same enumeration, or boolean type. */
/* @@? 18:25 Error Syntax error ESY0227: Unexpected token 'let'. */
/* @@? 18:29 Error Syntax error ESY0228: Unexpected token, expected ')'. */
/* @@? 18:38 Error Semantic error ESE0371: Cannot find type 'A'. */
/* @@? 18:68 Error Semantic error ESE0371: Cannot find type 'A'. */
/* @@? 18:72 Error Semantic error ESE0143: Unresolved reference f */
/* @@? 18:78 Error Semantic error ESE0371: Cannot find type 'A'. */
/* @@? 18:82 Error Syntax error ESY0230: Expected ')', got ';'. */
/* @@? 19:4 Error Semantic error ESE0143: Unresolved reference i */
/* @@? 19:10 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 19:12 Error Syntax error ESY0227: Unexpected token '{'. */