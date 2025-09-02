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

class A {}
class B {}
class C extends A {}

function test1(ab?: A | B) {
  const a = ab as A ?? new A(); // No error in ArkTS 1.2
  const b = ab as B ?? new B(); // No error in ArkTS 1.2
}

function test2(): A | null {
  return new B(); // Error in ArkTS 1.2
}

function test3(a: A): C | undefined {
  return a ? a as C : undefined; // No error in ArkTS 1.2
}

function test4(): void {
  console.log(new A() as B); // Error in ArkTS 1.2
  console.log(new B() as C); // Error in ArkTS 1.2
  console.log(new A() as C); // No error in ArkTS 1.2
  console.log(new C() as A); // No error in ArkTS 1.2
}

interface I {
  a?: A;
}
let i: I = {
  a: new A() // No error in ArkTS 1.2
};