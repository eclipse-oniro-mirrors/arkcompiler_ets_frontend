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

/* The test code is the lowering result of the code:
    interface test {
        testFoo: () => string;
    }

    function ff(): string {
        return "1111"
    }

    class ExpectExtend {
      public tst: test | null;
      extendstst() {
      const tst: test = {
        testFoo : ff,
      };
      this.tst = tst;
    }
*/

function ff(): string {
  return "1111";
}

interface test {
  set testFoo(testFoo: (()=> string))
  get testFoo(): (()=> string)
}

class ExpectExtend {
  public tst: test;
  public extendstst() {
    const tst: test = {
      testFoo: ff,
    };
    (this).tst = tst;
  }
  public constructor(tst: test) {
    this.tst = tst
  }
}

arktest.assertEQ(true, true);
