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

enum E1 {
  e1_item1,
  e1_item2 = 7,
  e1_item3,
  e1_item4 = 1
}


function main() {

  try {
    arktest.assertEQ(E1.e1_item1.toString(), "0")
    arktest.assertEQ(E1.e1_item1.valueOf(), 0)
    arktest.assertEQ(E1.e1_item2.toString(), "7")
    arktest.assertEQ(E1.e1_item2.valueOf(), 7)
    arktest.assertEQ(E1.e1_item3.toString(), "8")
    arktest.assertEQ(E1.e1_item3.valueOf(), 8)

    let test1 : E1 = E1.getValueOf("e1_item1");
    let test2 : E1 = E1.getValueOf("e1_item2");
    let test3 : E1 = E1.getValueOf("e1_item3");

    arktest.assertEQ(test1.valueOf(), 0)
    arktest.assertEQ(test1.getName(), "e1_item1")
    arktest.assertEQ(test2.valueOf(), 7)
    arktest.assertEQ(test2.getName(), "e1_item2")
    arktest.assertEQ(test3.valueOf(), 8)
    arktest.assertEQ(test3.getName(), "e1_item3")
  }
  catch (e) {
    //Exception happened
  }


}
