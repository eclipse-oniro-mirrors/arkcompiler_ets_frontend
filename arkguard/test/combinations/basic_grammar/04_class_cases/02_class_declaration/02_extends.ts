/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import assert from "assert";
class C1 {
  num: number = 1;
  method_c1(): string {
    return 'c1'
  }
}
class C2 extends C1 {
  num2: number = 2;
}

module M1 {
  export class C3 {
    prop_c3: number = 3;
    method_c31(): number {
      return 31;
    }
    method_c32(): number {
      return 32;
    }
  }
  class C4 extends M1.C3 {
    method_c32(): number {
      return 42;
    }
  }
  let insC4 = new C4();
  assert(insC4.method_c31() === 31)
  assert(insC4.method_c32() === 42)
}
let insC1 = new C1();
assert(insC1.method_c1() === 'c1')
let insC2 = new C2();
assert(insC2.num2 === 2)
let insC3 = new M1.C3();
assert(insC3.method_c31() === 31)
assert(insC3.method_c32() === 32)