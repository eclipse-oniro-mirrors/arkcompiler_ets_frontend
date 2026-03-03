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

  let t0: [] = [];
  for (let val: Any of t0) {
     throw new Error("Unreachable!");
  }

  let t31: Tuple3<String, Int, Double> | string  = new Tuple3<String, Int, Double>("3", 2, 1.1);
  let x: string = "";;
  for (let val: Any of t31) {
     x += val;
  }
  arktest.assertEQ("321.1", x);
 
  t31 = "TEST1";
  x = ""; 
  for (let val of t31) {
     x += val;
  }
  arktest.assertEQ("TEST1", x);

  let t32: [String, Int, Double] | string = ["3-2", 32, 3.2] as [String, Int, Double];
  x = "";
  for (let val: Any of t32) {
    x += val;
  }
  arktest.assertEQ("3-2323.2", x);

  t32 = "TEST2";
  x = "";
  for (let val of t32) {
    x += val;
  }
  arktest.assertEQ("TEST2", x);
 
  let a = new Array<Any>(5);
  a[0] = 'seventeen';
  a[1] = 18;
  a[2] = "19";
  a[3] = 20.0;
  a[4] = " + 1!";

  let tn1 = new TupleN<String, Int, number, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double>(
    'one', 2, 3.3, 4.0, 'five', 6, "7", 8.0, 'nine', 10, "11", 12.0, 'thirteen', 14, "15", 16.0, a);

  let tn2: [String, Int, number, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String] = 
    ['one', 2, 3.3, 4.0, 'five', 6, "7", 8.0, 'nine', 10, "11", 12.0, 'thirteen', 14, "15", 16.0, 'seventeen', 18, "19", 20., " + 1!"];

  let t: Tuple = tn1;
  let x1 = "";
  for (let val: Any of t) {
    x1 += val;
  }

  t = tn2 as Tuple;
  let x2 = "";
  for (let val: Any of t) {
    x2 += val;
  }
  arktest.assertEQ(x1, x2);                                                    
}
