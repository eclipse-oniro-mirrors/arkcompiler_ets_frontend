/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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


/* @@? 17:3 Error Syntax error ESY0040: Illegal start of ENUM expression. */
/* @@? 21:3 Error Syntax error ESY0040: Illegal start of ENUM expression. */
/* @@? 24:5 Error Syntax error ESY0040: Illegal start of ENUM expression. */
/* @@? 29:3 Error Syntax error ESY0233: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@? 32:5 Error Syntax error ESY0040: Illegal start of ENUM expression. */
/* @@? 37:4 Error Syntax error ESY0224: Identifier expected, got 'enum'. */
/* @@? 37:9 Error Semantic error ESE0371: Cannot find type 'E1'. */
/* @@? 37:11 Error Syntax error ESY0119: Interface fields must have type annotation. */
/* @@? 38:1 Error Syntax error ESY0227: Unexpected token '}'. */
