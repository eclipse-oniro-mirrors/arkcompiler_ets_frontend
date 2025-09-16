/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

const isTrue = true;
switch (isTrue) {
  case true:
    console.log('It\'s true');
    break;
  case false:
    console.log('It\'s false')
    break;
}

let objValue = { key: "value" };

switch (objValue) {
  case { key: "value" }:
    console.log("Matching object!");
    break;
  default:
    console.log("Non-matching object");
}

let arrayValue = [1, 2, 3];

switch (arrayValue) {
  case [1, 2, 3]:
    console.log("Matching array!");
    break;
  default:
    console.log("Non-matching array");
}

const isT = 'true';
switch (isT) {
  case 'true':
    console.log('It\'s true');
    break;
  case 'false':
    console.log('It\'s false')
    break;
}

const number1 = 1;
switch (number1) {
  case 1:
    console.log('It\'s 1');
    break;
  case 2:
    console.log('It\'s 2')
    break;
}

enum Test {
  A = 1,
  B = 2
}
const test = Test.A
switch (test) {
  case Test.A:
    console.log('It\'s A');
    break;
  case Test.B:
    console.log('It\'s B')
    break;
}

const number2 = -1.1;
switch (number2) {
  case -1.1:
    console.log('It\'s 1');
    break;
  case 2:
    console.log('It\'s 2')
    break;
}

const number3 = Number.NaN;
switch (number3) {
  case -1.1:
    console.log('It\'s 1');
    break;
  case 2:
    console.log('It\'s 2');
    break;
}

const number4 = Number.MAX_VALUE;
switch (number4) {
  case -1.1:
    console.log('It\'s 1');
    break;
  case 2:
    console.log('It\'s 2');
    break;
}

let day:int = 2;
let dayName = "empty";
switch (day) {
    case 0:
        dayName = "Sunday";
        break;
    case 1:
        dayName = "Monday";
        break;
    case 2:
        dayName = "Tuesday";
        break;
    case 3:
        dayName = "Wednesday";
        break;
    case 4:
        dayName = "Thursday";
        break;
    case 5:
        dayName = "Friday";
        break;
    case 6:
        dayName = "Saturday";
        break;
    default:
        dayName = "Unknown";
}

const number01: number = 100;
function Foo(number01: number) {
  switch (number01) {
    case 1:
      console.log('It\'s 1');
      break;
    case 2:
      console.log('It\'s 2')
      break;
  }
}
Foo(number01)

const num11: number = 2;
switch (num11) {
  case 1: {
    console.log('One');
    break;
  }
  case 2: {
    console.log('Two');
    break;
  }
}

let num0 = 2;
switch (num0) {
  case 1: {
    console.log('One');
    break;
  }
  case 2: {
    console.log('Two');
    break;
  }
}

let num00: number = 2;
switch (num00) {
  case 1: {
    console.log('One');
    break;
  }
  case 2: {
    console.log('Two');
    break;
  }
}

let num1 = 0;
switch (num1) {
  case 0:
    console.log('One');
    break;
  default:
    console.log('Other number');
}

let num111 = 1.0;
switch (num111) {
  case 1.0:
    console.log('One');
    break;
  case 2.0:
    console.log('Other number');
}

const number134 = 100;
switch (number134) {
  case 1:
    console.log('It\'s 1');
    break;
  case 2:
    console.log('It\'s 2')
    break;
}

let str11 = new String("hello");
switch (str11) {
  case "hello":
    console.log("Case 1")
    break
  case "world":
    console.log("Default Case")
}

let isnum1: string | number = 1;
switch (isnum1) {
  case 1.1:
    console.log('It's num');
    break;
  default:
    console.log('F');
}

type sw1 = number

function FunSw4(): sw1 {
  return 1
}

switch (FunSw4()) {
  case 1:
    console.log('A');
    break;
  default:
    console.log('F');
}

type sw2 = number | string

function FunSw5(): sw2 {
  return 1.11
}

switch (FunSw5()) {
  case 1.11:
    console.log('A');
    break;
  default:
    console.log('F');
}

let number7 = Math.floor(1.5);
switch (number7) {
  case Math.floor(1.5):
    console.log("Case 1");
    break;
  default:
    console.log("Default case");
}
switch (85 + 5.5) {
  case 90.5:
    console.log('A');
    break;
  default:
    console.log('F');
}
const add = (a: number, b: number): number => a + b;
const result1 = add(10, 5);
switch (result1) {
  case 15:
    console.log('A');
    break;
  case 14:
    console.log('B');
    break;
  default:
    console.log('F');
}
function FunSw2(): number {
  return 1.1
}

switch (FunSw2()) {
  case 1.1:
    console.log('A');
    break;
  default:
    console.log('F');
}

let number4 = Infinity;
switch (number4) {
  case Infinity:
    console.log("Case 1");
    break;
  default:
    console.log("Default case");
}

let number5 = -Infinity;
switch (number5) {
  case -Infinity:
    console.log("Case 1");
    break;
  default:
    console.log("Default case");
}

let number6 = NaN;
switch (number6) {
  case NaN:
    console.log("Case 1");
    break;
  default:
    console.log("Default case");
}
// 1.0
let num111 = 1.0;
switch (num111) {
  case 1.0:
    console.log('One');
    break;
  case 2.0:
    console.log('Other number');
}

let number33: number = 1;
switch (number33) {
  case 1.1:
    console.log("Case 1");
    break;
  case 2:
    console.log("Case 2");
    break;
  case 3:
    console.log("Case 3");
    break;
  default:
    console.log("Default case");
}

const num11: number = 2;
switch (num11) {
  case 1: {
    console.log('One');
    break;
  }
  case 2: {
    console.log('Two');
    break;
  }
}

let num00: number = 2;
switch (num00) {
  case 1: {
    console.log('One');
    break;
  }
  case 2: {
    console.log('Two');
    break;
  }
}

enum Direction1 {
  North,
  South,
  East,
  West
}
function funSE() : string | Direction1 {
  return 'aaa'
}
switch (funSE()) { 
  case 'aaa':
    console.log('aaa');
    break;
  case Direction1.North:
    console.log('aaa');
    break;
  default:
    console.log('F');
}

enum H {
  RED,
  BLUE
}
function foo11(e: H) {
  switch (e) {
    case H.RED:
  }
}

function foo(index: number) {
  switch (index) {
    case 0:
      break;
  }
}

