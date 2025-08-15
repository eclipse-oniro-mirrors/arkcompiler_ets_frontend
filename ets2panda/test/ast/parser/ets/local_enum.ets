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

let myAssertEqual = (actualValue: boolean, expectValue: boolean) => {
  enum E {E1}
}

function test() {
  enum E {E1}

  if (true) {
    enum E {E1}
  }
}

class C {
  enum E {E1}

  foo(){
    enum E {E1}
  }
}

interface I{
   enum E {E1}
}

/* @@? 17:3 Error SyntaxError: Illegal start of ENUM expression. */
/* @@? 21:3 Error SyntaxError: Illegal start of ENUM expression. */
/* @@? 24:5 Error SyntaxError: Illegal start of ENUM expression. */
/* @@? 29:3 Error SyntaxError: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@? 32:5 Error SyntaxError: Illegal start of ENUM expression. */
/* @@? 37:4 Error SyntaxError: Identifier expected, got 'enum'. */
/* @@? 37:11 Error SyntaxError: Interface fields must have type annotation. */
/* @@? 37:12 Error TypeError: Cannot find type 'E1'. */
/* @@? 38:1 Error SyntaxError: Unexpected token '}'. */
