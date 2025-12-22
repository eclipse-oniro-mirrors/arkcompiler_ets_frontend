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

// --- 1. Shadowing Safety Check (Should not be refactored) ---
function safetyCheck(ctor: AggregateErrorConstructor) { // ERROR Cannot find type 
  return ctor([Error("param")]); // ERROR This expression is not callable.
}

// --- 2. Mixed Parameters (Constructor in the middle) ---
function createComplex(prefix: string, ctor: AggregateErrorConstructor, code: number) { // ERROR Cannot find type 
  return ctor([Error(prefix)], `${code}`); // ERROR This expression is not callable.
}

// --- 3. Variable Assignment (Aliasing) ---
function aliasExample(ctor: AggregateErrorConstructor) { // ERROR Cannot find type 
  const myFunc = ctor;
  return myFunc([Error("alias")]); // ERROR This expression is not callable.
}

// --- 7. Conditional / Ternary Usage ---
function conditionalCreate(ctor: AggregateErrorConstructor, makeIt: boolean) { // ERROR Cannot find type 
  return makeIt ? ctor([Error("yes")]) : null; // ERROR This expression is not callable.
}

// --- 8. Arrow Functions ---
const createArrow = (ctor: AggregateErrorConstructor) => { // ERROR Cannot find type 
  return ctor([Error("arrow")]); // ERROR This expression is not callable.
};

// --- 9. Usage inside an Object Method ---
class ErrorFactory {
  // Method parameter test case
  create(ctor: AggregateErrorConstructor) { // ERROR Cannot find type 
    return ctor([Error("method")]); // ERROR This expression is not callable.
  }
}

function createArray(ctor: ArrayConstructor) { // ERROR Cannot find type 
  return ctor<number>(1, 2, 3); // ERROR This expression is not callable.
}
function createBigInt(ctor: BigIntConstructor) { // ERROR Cannot find type 
  return ctor(1); // ERROR This expression is not callable.
}
function anotherName(ctorName0: BigIntConstructor) { // ERROR Cannot find type 
  ctorName0(1); // ERROR This expression is not callable.
  const ctorName1 = ctorName0
  const rs1 = ctorName1(1); // ERROR This expression is not callable.
  type BigIntConstructor1 = BigIntConstructor; // ERROR Illegal start of Type Alias expression.
  let ctorName2:BigIntConstructor1 = ctorName1 // ERROR Cannot find type 
  const rs2 = ctorName2(1); // ERROR This expression is not callable.
}