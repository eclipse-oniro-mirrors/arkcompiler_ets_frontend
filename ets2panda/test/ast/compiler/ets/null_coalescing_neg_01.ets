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

function foo1(z: int|undefined) {
  let x = z ?? /* @@ label1 */10000000000;
  return x;
}

function foo2(z: short|undefined) {
  let x = z ?? /* @@ label2 */10000000;
  return x;
}

function foo3(z: byte|undefined) {
  let x = z ?? /* @@ label3 */10000;
  return x;
}

function foo4(z: int|undefined) {
  let x = z ?? /* @@ label4 */100.0;
  return x;
}

function foo5(z: long|undefined) {
  let x = z ?? /* @@ label5 */100000.0f;
  return x;
}

function foo6(z: float|undefined) {
  let x = z ?? /* @@ label6 */100000.0;
  return x;
}



/* @@@ label1 Error Semantic error ESE0345: Value is out of range */
/* @@@ label2 Error Semantic error ESE0345: Value is out of range */
/* @@@ label3 Error Semantic error ESE0345: Value is out of range */
/* @@@ label4 Error Semantic error ESE0318: Type 'Double' cannot be assigned to type 'Int' */
/* @@@ label5 Error Semantic error ESE0318: Type 'Float' cannot be assigned to type 'Long' */
/* @@@ label6 Error Semantic error ESE0318: Type 'Double' cannot be assigned to type 'Float' */