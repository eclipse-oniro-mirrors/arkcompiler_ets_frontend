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

  let t1: [string] = ["test-2"];
  let t2: [string, int] = ["new test-2", 777];
  let t3: [String, Double, Int] = ["3-2", 3.2, 32];
  let t16: [String, Int, number, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double] =
    ['one', 2, 3.3, 4.0, 'five', 6, "7", 8.0, 'nine', 10, "11", 12.0, 'thirteen', 14, "15", 16.0];
  let t21 = ['one-2', 22, 3.2, 4.2, 'five-2', 62, "72", 8.2, 'nine-2', 102, "112", 12.2, 'thirteen-2', 142, "152", 16.2, 'seventeen-2', 182, "192", 20.2, " + 2!"] as
             [String, Int, number, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String];

  t1 = t2 as /* @@ label1 */Tuple1;

  t1 = /* @@ label2 */t2 as Tuple1<Int>;
 
  t2 = /* @@ label3 */t1 as Tuple2<string, Int>;

  t2 = /* @@ label4 */t3 as Tuple2<string, Int>;

  let t = t16 as Tuple;
  t21 = /* @@ label5 */t as Tuple3<String, Int, number>;
}


/* @@@ label1 Error Semantic error ESE0170: Type 'Tuple1<T0>' is generic but type argument were not provided. */
/* @@@ label2 Error Semantic error ESE0326: Cannot cast type '[String, Int]' to 'Tuple1<Int>' */
/* @@@ label2 Error Semantic error ESE0318: Type 'Tuple1<Int>' cannot be assigned to type '[String]' */
/* @@@ label3 Error Semantic error ESE0326: Cannot cast type '[String]' to 'Tuple2<String,Int>' */
/* @@@ label4 Error Semantic error ESE0326: Cannot cast type '[String, Double, Int]' to 'Tuple2<String,Int>' */
/* @@@ label5 Error Semantic error ESE0318: Type 'Tuple3<String,Int,Double>' cannot be assigned to type '[String, Int, Double, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String]' */
