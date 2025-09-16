/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

let q: int = 1;  // variable in top-level scope
let s = "abc"

switch (q) {
  case 1: {
    q += 2; 
    break;
  }
  case 2: {
    arktest.assertTrue(false)
    break;
  }
}

class TestException extends Exception {
}

arktest.assertEQ(q, 3)
arktest.assertEQ(s, "abc")

try {
  s += "cba";
  q += 5;
  throw new TestException();
}
catch (e: TestException) {
}
catch (e) {
  arktest.assertTrue(false)
}

arktest.assertEQ(q, 8)
arktest.assertEQ(s, "abccba")

function main(): void {
  ETSGLOBAL.q = 1;
  let q: int;  // function main scope, top-level q is shadowed
  q = q + 30;
  arktest.assertEQ(q, 30)
  arktest.assertEQ(ETSGLOBAL.q, 1)

  s = "def";
  arktest.assertEQ(s, "def")
}
