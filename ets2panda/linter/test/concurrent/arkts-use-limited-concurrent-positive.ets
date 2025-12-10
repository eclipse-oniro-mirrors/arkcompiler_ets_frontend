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

// P1: Function Argument
function logDirective(str: string){
  console.log(`Checking string: ${str}`);
}
logDirective('use concurrent'); // Should pass: Not a directive.

// P2: Variable Assignment
const directiveValue = 'use concurrent'; // Should pass: It is the value being assigned to a variable

// P3: Switch Case Comparison
function checkValue(val: string) {
  switch (val) {
      case 'use concurrent': // Should pass: It is a string literal used as a case comparison value within a switch statement
          return true;
      default:
          return false;
  }
}

// P4: Not as the very first statement
function notADirective() {
  let x = '1';
  "use concurrent";  // Should pass: just a string literal, ignored as a directive
  if (x === 'use concurrent') { // Should pass: Comparison element
  }
  for (const val of ['use concurrent', 'test']) { // Should pass: Just a string literal, inside of for loop
    console.log(val);
  }
};

// P5: Exact Generic Instantiation
const cases = [
    'use concurrent', // Should pass:  A string literal array element
    "use strict",
];