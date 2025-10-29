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

class A {
  field1: number = 10;
  private field2: number = 20;
  foo() { }
}

function main(): void {
  let a: A = new A();
  type keyofA = keyof A;

  let x1: ("abcd" | keyofA | number | A)[] = ["abcd", "field2", "foo", 123, a];
  let x2: ("abcd" | keyofA | number | A)[] = ["abcd", "other field", "foo", 123, a];

  let x3: ["abcd", keyofA, number, A] = ["abcd", "field2", 123, a];
  let x4: ["abcd", keyofA, number, A] = ["abcd", "other field", 123, a];

  let x5: Array<"abcd" | keyofA | number | A> = new Array<"abcd" | keyofA | number | A>("abcd", "field2", "foo", 123, a);
  let x6: Array<"abcd" | keyofA | number | A> = new Array<"abcd" | keyofA | number | A>("abcd", "other field", "foo", 123, a);
}


/* @@? 1:3 Error Semantic error ESE0371: Cannot find type 'keyofA'. */
/* @@? 24:3 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@? 29:20 Error Semantic error ESE0371: Cannot find type 'keyofA'. */
/* @@? 30:20 Error Semantic error ESE0371: Cannot find type 'keyofA'. */
/* @@? 32:26 Error Semantic error ESE0371: Cannot find type 'keyofA'. */
/* @@? 32:68 Error Semantic error ESE0371: Cannot find type 'keyofA'. */
/* @@? 33:26 Error Semantic error ESE0371: Cannot find type 'keyofA'. */
/* @@? 33:68 Error Semantic error ESE0371: Cannot find type 'keyofA'. */