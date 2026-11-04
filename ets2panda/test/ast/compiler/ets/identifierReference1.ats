/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

let a:(a: int, b: int) => int = max;

let b: String = "foo";

function bar (a: int): void {

}

class B {
  b: int = 2;
  static c: (a: int) => void;
  foo(a: int): void {

  }
}

class A extends B {
  foo(a: int, b: int): void {
    b = 3;
  }

  static foo(): void {
    A.c = bar;
  }

  baz(): void {
    this.foo(1);
  }

  /* @@ label */class C {
    foo(): void {
      this.b = 4;
    }
  }
}


/* @@? 16:33 Error Semantic error ESE0307: Overloaded method is used as value */
/* @@@ label Error Syntax error ESY0233: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@? 47:12 Error Semantic error ESE0087: Property 'b' does not exist on type 'C' */