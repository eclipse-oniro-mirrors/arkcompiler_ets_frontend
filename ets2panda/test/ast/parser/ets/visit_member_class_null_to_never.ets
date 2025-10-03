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

class expr01701 {
    val: int;
  
    static run(): int {
      let x: expr01701 | null= null;
      let res: int;
      try {
        res = x!.val;
      } catch (error) {
        return 0;
      }
      return 2;
    }
}
  
function dasgAssert() {
    let result = expr01701.run();
    arktest.assertEQ(result, 0);
}
  
dasgAssert();


/* @@? 23:15 Warning Warning W0014: Bad operand type, the operand of the non-nullish expression is 'null' or 'undefined'. */