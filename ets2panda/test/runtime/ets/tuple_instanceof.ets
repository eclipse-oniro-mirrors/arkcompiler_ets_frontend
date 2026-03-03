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

function checkTuples(x: Object): string {
  if (x instanceof Tuple2) {
    return "x[1] = " +  x.unsafeGet(1);
  } else if (x instanceof Tuple3) {
    return "Unreachable!";
  } else if (x instanceof Tuple1) {
    return "x[0] = " +  x.unsafeGet(0);
  } else if (x instanceof Tuple) {
    return "Tuple with length = " + x.length;
  } else {
    return "Not a tuple: " + x.toString();
  }
}

function main() {
  let a: [string, string] = ["aa", "aaa"];
  arktest.assertEQ("x[1] = aaa", checkTuples(a as Object));

  let b: [string, int] = ["bb", 7];
  arktest.assertEQ("x[1] = 7", checkTuples(b as Object));

  let c: [string, double, int] = ["ccc", 3.14, 8];
  arktest.assertEQ("x[1] = 3.14", checkTuples(c as Object));

  let d: [string] = ["tuple"];
  arktest.assertEQ("x[0] = tuple", checkTuples(d as Object));

  let e: [Object, Object, Object, double] = ["eee", ["zero =", 0] as [string, int],  9, 3.15];
  arktest.assertEQ("x[1] = zero =,0", checkTuples(e as Object));

  let f: [] = [];
  arktest.assertEQ("Tuple with length = 0", checkTuples(f as Object));

  let g: Tuple = c as Tuple;
  arktest.assertEQ("x[1] = 3.14", checkTuples(g));

  arktest.assertEQ("Not a tuple: 55", checkTuples(new Int(55) as Object));

  arktest.assertEQ("Not a tuple: test", checkTuples("test" as Object));
}
