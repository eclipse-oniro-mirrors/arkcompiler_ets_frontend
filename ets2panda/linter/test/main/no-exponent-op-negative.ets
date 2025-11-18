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


let y:number = 3;
y **= 4; // Violates the rule

let result1 = (1 + 2) ** (3 * 2); // Violates the rule

function power1(base: number, exponent: number) {
  return base ** exponent; // Violates the rule
}

class MathOps {
  value: number = 2 ** 3; // Violates the rule
  
  calc(n: number) {
    return n **= 2; // Violates the rule
  }
}

function defaultArg(base: number, exp: number = 2 ** 2) { // Violates the rule
  return base;
}

let call = (() => 3)() ** 2; // Violates the rule

let chained = 2 ** (3 ** 2); // Violates the rule

let text = `result: ${3 ** 2}`; // Violates the rule
