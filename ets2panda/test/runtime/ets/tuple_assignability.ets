/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

function foo(): string {
   return "+4 at last!"
}

function main() {

  let t0 = new Tuple0();

  let t11 = new Tuple1<String>("test-1");
  let t12: [string] = ["test-2"];

  let t21 = new Tuple2<String, Int>("new test-1", 7);
  let t22: [string, int] = ["new test-2", 777];

  let t31 = new Tuple3<String, Int, Double>("3", 2, 1.1);
  let t32: [String, Int, Double] = ["3-2", 32, 3.2];

  let t16: [String, Int, number, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double] =
    ['one', 2, 3.3, 4.0, 'five', 6, "7", 8.0, 'nine', 10, "11", 12.0, 'thirteen', 14, "15", 16.0];

  let tn20 = new TupleN<String, Int, number, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double>(
    'one', 2, 3.3, 4.0, 'five', 6, "7", 8.0, 'nine', 10, "11", 12.0, 'thirteen', 14, "15", 16.0,
    ['seventeen', 18, "19", 20.0]);

  let tn21 = ['one-2', 22, 3.2, 4.2, 'five-2', 62, "72", 8.2, 'nine-2', 102, "112", 12.2, 'thirteen-2', 142, "152", 16.2, 'seventeen-2', 182, "192", 20.2, " + 2!"] as
             [String, Int, number, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String];

  tn21[0] = 'new-one';
  tn21[20] = "+3" + " now!"
  arktest.assertEQ(tn21[0], 'new-one')
  arktest.assertEQ(tn21[10 + 10], "+3 now!")

  tn21[0] = "one" + '-' + "one";
  arktest.assertEQ(tn21.unsafeGet(0), 'one-one')

  tn21[25 - 5] = foo();
  arktest.assertEQ(tn21.unsafeGet(5 * 4), foo())

  let t: Tuple = t0;
  t = t12;
  t = t22;
  t = t31;
  t = tn20;
  arktest.assertEQ(t.unsafeGet(1), 2)
  arktest.assertEQ(t.unsafeGet(18), "19")

  t0 = tn21;
  t0 = t32;
  t0 = t22;
  t0 = t12;
  arktest.assertEQ(t0.unsafeGet(0), "test-2")


  t12 = t21
  t12 = t32;
  arktest.assertEQ(t12[0], "3-2")
  arktest.assertEQ(t12.unsafeGet(2), 3.2)
  t12 = tn20;

  t22 = t32;
  t22 = tn21;
  arktest.assertEQ(t22[1], 22)
  arktest.assertEQ(t22.unsafeGet(7 + 13), "+4 at last!")

  t32 = t16;
  t32 = tn20;

  tn20 = tn21;
  tn21 = t32;		// Smart cast!
  arktest.assertEQ(tn21.unsafeGet(19), 20.0)
  arktest.assertEQ(tn21.unsafeGet(4), 'five')
}
