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

class A{}

let myAssertEqual = (actualValue: boolean, expectValue: boolean) => {
  type a = A;
}

function test() {
  type a = A;

  if (true) {
    type a = A;
  }
}

class C {
  type a = A;

  foo(){
    type a = A;
  }
}

interface I{
   type a = A;
}

/* @@? 19:3 Error SyntaxError: Illegal start of Type Alias expression. */
/* @@? 23:3 Error SyntaxError: Illegal start of Type Alias expression. */
/* @@? 26:5 Error SyntaxError: Illegal start of Type Alias expression. */
/* @@? 31:7 Error SyntaxError: Field type annotation expected. */
/* @@? 31:12 Error SyntaxError: Class cannot be used as object. */
/* @@? 34:5 Error SyntaxError: Illegal start of Type Alias expression. */
/* @@? 39:9 Error TypeError: Cannot find type 'a'. */
/* @@? 39:11 Error SyntaxError: Interface member initialization is prohibited. */
/* @@? 39:13 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 39:13 Error SyntaxError: Identifier expected, got 'identification literal'. */
/* @@? 39:14 Error SyntaxError: Identifier expected. */
