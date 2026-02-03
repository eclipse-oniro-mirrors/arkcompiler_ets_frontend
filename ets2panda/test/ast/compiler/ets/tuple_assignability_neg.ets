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

  let t1: [string] = ["test"];

  let t2: [string, int] = ["new test", 2];

  let t3: [String, Double, Int] = ["3", 3.3, 3];

  let t16: [String, Int, number, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double] =
    ['one', 2, 3.3, 4.0, 'five', 6, "7", 8.0, 'nine', 10, "11", 12.0, 'thirteen', 14, "15", 16.0];

  let t21 = ['one-2', 22, 3.2, 4.2, 'five-2', 62, "72", 8.2, 'nine-2', 102, "112", 12.2, 'thirteen-2', 142, "152", 16.2, 'seventeen-2', 182, "192", 20.2, " + 2!"] as
            [String, Int, number, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String];

  t3 = /* @@ label01 */t0;
  t2 = /* @@ label02 */t0;
  t1 = /* @@ label03 */t0;

  t2 = /* @@ label04 */t1
  t3 = /* @@ label05 */t1;
  t16 = /* @@ label06 */t1;

  t2 = /* @@ label07 */t3;
  t3 = /* @@ label08 */t2;
  t21= /* @@ label09 */t2;

  t3 = /* @@ label10 */t16;
  t21 = /* @@ label11 */t3;

  t16 = /* @@ label12 */t2;
  t21 = /* @@ label13 */t16; 
  t21 = /* @@ label14 */t1;
}


/* @@@ label01 Error Semantic error ESE0318: Type '[]' cannot be assigned to type '[String, Double, Int]' */
/* @@@ label02 Error Semantic error ESE0318: Type '[]' cannot be assigned to type '[String, Int]' */
/* @@@ label03 Error Semantic error ESE0318: Type '[]' cannot be assigned to type '[String]' */
/* @@@ label04 Error Semantic error ESE0318: Type '[String]' cannot be assigned to type '[String, Int]' */
/* @@@ label05 Error Semantic error ESE0318: Type '[String]' cannot be assigned to type '[String, Double, Int]' */
/* @@@ label06 Error Semantic error ESE0318: Type '[String]' cannot be assigned to type '[String, Int, Double, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double]' */
/* @@@ label07 Error Semantic error ESE0318: Type '[String, Double, Int]' cannot be assigned to type '[String, Int]' */
/* @@@ label08 Error Semantic error ESE0318: Type '[String, Int]' cannot be assigned to type '[String, Double, Int]' */
/* @@@ label09 Error Semantic error ESE0318: Type '[String, Int]' cannot be assigned to type '[String, Int, Double, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String]' */
/* @@@ label10 Error Semantic error ESE0318: Type '[String, Int, Double, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double]' cannot be assigned to type '[String, Double, Int]' */
/* @@@ label11 Error Semantic error ESE0318: Type '[String, Double, Int]' cannot be assigned to type '[String, Int, Double, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String]' */
/* @@@ label12 Error Semantic error ESE0318: Type '[String, Int]' cannot be assigned to type '[String, Int, Double, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double]' */
/* @@@ label13 Error Semantic error ESE0318: Type '[String, Int, Double, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double]' cannot be assigned to type '[String, Int, Double, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String]' */
/* @@@ label14 Error Semantic error ESE0318: Type '[String]' cannot be assigned to type '[String, Int, Double, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String, Int, String, Double, String]' */
