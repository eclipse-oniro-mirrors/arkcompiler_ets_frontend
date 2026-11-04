/**
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

function main(): void {
  let t: [number, string] = [1, 'a'];
  let x: Tuple = t;

  // Dummy code 'console.log(...)' will be never executed. Just placeholder.
  if (/* @@ label1 */x instanceof Tuple) {
    console.log("Tuple");
  }
  if (/* @@ label2 */x instanceof Tuple1) {
    console.log("Tuple1");
  }
  if (/* @@ label3 */x instanceof Tuple2) {
    console.log("Tuple2");
  }
  if (x instanceof Tuple3) {
    console.log("Tuple3");
  }
  if (x instanceof Tuple5) {
    console.log("Tuple5");
  }
  if (/* @@ label4 */x instanceof Object) {
    console.log("Object");
  }

  if (/* @@ label5 */t instanceof Tuple) {
    console.log("Tuple");
  }
  if (/* @@ label6 */t instanceof Tuple1) {
    console.log("Tuple1");
  }
  if (/* @@ label7 */t instanceof Tuple2) {
    console.log("Tuple2");
  }
  if (t instanceof Tuple3) {
    console.log("Tuple3");
  }
  if (t instanceof Tuple5) {
    console.log("Tuple5");
  }
  if (/* @@ label8 */t instanceof Object) {
    console.log("Object");
  }
}


/* @@@ label1 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@@ label2 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@@ label3 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@@ label4 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@@ label5 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@@ label6 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@@ label7 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@@ label8 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
