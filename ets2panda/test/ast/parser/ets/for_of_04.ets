/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

let v: double;

function main(): void {
  let res = 0;
  for (v: double of a) {
    res += v;
  }
}

for (i in 50) {
  a += 10
}
/* @@? 20:9 Error SyntaxError: Type annotation is not allowed when existing variable is used as loop iterator in 'for' statement. */
/* @@? 20:11 Error SyntaxError: Expected ';', got 'double'. */
/* @@? 20:11 Error SyntaxError: Unexpected token 'double'. */
/* @@? 20:18 Error SyntaxError: Expected ';', got 'identification literal'. */
/* @@? 20:18 Error TypeError: Unresolved reference of */
/* @@? 20:21 Error SyntaxError: Expected ')', got 'identification literal'. */
/* @@? 20:21 Error TypeError: Unresolved reference a */
/* @@? 20:22 Error SyntaxError: Unexpected token ')'. */
/* @@? 20:24 Error SyntaxError: Unexpected token '{'. */
/* @@? 25:11 Error TypeError: Object type doesn't have proper iterator method. */
/* @@? 25:11 Error TypeError: 'For-of' statement source expression is not of iterable type. */
