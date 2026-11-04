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

function main() {

  let t0: []  = [];

  let t11 = new Tuple1<String>("test-1");
  let t12: [string] = ["test-2"];

  let t21 = new Tuple2<String, Int>("new test-1", 7);
  let t22: [string, int] = ["new test-2", 777];

  let t31: Tuple3<String, Int, Double> = new Tuple3<String, Int, Double>("3", 2, 1.1);
  let t32: [String, Int, Double] = ["3-2", 32, 3.2] as [String, Int, Double];

  let t16: [String, Int, number, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double] =
    ['one', 2, 3.3, 4.0, 'five', 6, "7", 8.0, 'nine', 10, "11", 12.0, 'thirteen', 14, "15", 16.0];

  let tn20 = new TupleN<String, Int, number, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double>(
    'one', 2, 3.3, 4.0, 'five', 6, "7", 8.0, 'nine', 10, "11", 12.0, 'thirteen', 14, "15", 16.0,
    ['seventeen', 18, "19", 20.0]);

  let tn21 = ['one-2', 22, 3.2, 4.2, 'five-2', 62, "72", 8.2, 'nine-2', 102, "112", 12.2, 'thirteen-2', 142, "152", 16.2, 'seventeen-2', 182, "192", 20.2, " + 2!"] as
             [String, Int, number, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String];

  let t: Tuple = t0;
  arktest.assertEQ(" : length = 0", t + " : length = " + t.length);
 
  t = t11 as Tuple;
  arktest.assertEQ("test-1 : length = 1", t + " : length = " + t.length);
 
  t = t12 as Tuple;
  arktest.assertEQ("test-2 : length = 1", t + " : length = " + t.length);

  arktest.assertEQ("new test-1,7 : length = 2", (t21 as Tuple) + " : length = " + (t21 as Tuple).length);

  t = t22 as Tuple;
  arktest.assertEQ("new test-2,777 : length = 2", t + " : length = " + t.length);

  t = t31 as Tuple;
  arktest.assertEQ("3,2,1.1 : length = 3", t + " : length = " + t.length);

  arktest.assertEQ("3-2,32,3.2 : length = 3", (t32 as Tuple) + " : length = " + (t32 as Tuple).length);

  t = t16 as Tuple;
  arktest.assertEQ("one,2,3.3,4,five,6,7,8,nine,10,11,12,thirteen,14,15,16 : length = 16", t + " : length = " + t.length);

  arktest.assertEQ("one,2,3.3,4,five,6,7,8,nine,10,11,12,thirteen,14,15,16,seventeen,18,19,20 : length = 20", (tn20 as Tuple) + " : length = " + (tn20 as Tuple).length);

  t = tn21 as Tuple;
  arktest.assertEQ("one-2,22,3.2,4.2,five-2,62,72,8.2,nine-2,102,112,12.2,thirteen-2,142,152,16.2,seventeen-2,182,192,20.2, + 2! : length = 21", tn21 + " : length = " + tn21.length);
}
