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

class A {
  foo(a: int): void {

  }

  foo(): void {

  }

  /* @@ label */class B {
    foo(a: int): void {

    }

    baz(): void {
      this.foo();
    }
  }
}

/* @@? 25:17 Error Syntax error ESY0233: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@? 31:7 Error Semantic error ESE0124: Expected 1 arguments, got 0. */
/* @@? 31:7 Error Semantic error ESE0127: No matching call signature for foo() */
