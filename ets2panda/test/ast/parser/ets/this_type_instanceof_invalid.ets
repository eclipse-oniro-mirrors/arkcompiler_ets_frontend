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

type F = () => number;

interface I {
  goo(){
    if("string" instanceof this){
      return 1
    }
    return 0
  }
}

class C<T> {
  a: T;
  b: F;
  constructor(a: T,  b: F) {
    this.a = a;
    this.b = b;
  }

  static hoo(){
    if (5 instanceof this) {
        return 0;
    }
    return 1;
  }

  static koo<F>(){
    if (F instanceof this) {
        return 0;
    }
    return 1;
  }

  foo() {
    if (this.a instanceof this.b) {
        return 0;
    }
    return 1;
  }

  joo<G>() {
    if (G instanceof this) {
        return 0;
    }
    return 1;
  }
}


/* @@? 20:28 Error Syntax error ESY0130: Invalid right-hand side in 'instanceof' expression. */
/* @@? 36:22 Error Syntax error ESY0130: Invalid right-hand side in 'instanceof' expression. */
/* @@? 43:22 Error Syntax error ESY0130: Invalid right-hand side in 'instanceof' expression. */
/* @@? 50:27 Error Syntax error ESY0130: Invalid right-hand side in 'instanceof' expression. */
/* @@? 50:31 Error Syntax error ESY0230: Expected ')', got '.'. */
/* @@? 50:31 Error Syntax error ESY0227: Unexpected token '.'. */
/* @@? 50:32 Error Syntax error ESY0227: Unexpected token 'b'. */
/* @@? 50:32 Error Semantic error ESE0145: Property 'b' must be accessed through 'this' */
/* @@? 50:33 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 50:35 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 57:22 Error Syntax error ESY0130: Invalid right-hand side in 'instanceof' expression. */