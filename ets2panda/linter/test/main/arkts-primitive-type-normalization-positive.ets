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

// P1: typeof Check on a Falsy Primitive
function testP1_Typeof_Falsy() {
  const boolPrimitive = false;
  
  // EXPECTED RESULT: "boolean"
  return typeof boolPrimitive;
}

// P2: Conditional (Falsy) Check
function testP2_Conditional_Falsy() {
  const falsyPrimitive = 0; // The primitive value
  let conditionPassed = false;

  // Checks the primitive value
  if (falsyPrimitive) {
    conditionPassed = true; // Will NOT be reached
  }

  // EXPECTED RESULT: false
  return conditionPassed;
}

// P3: Strict Equality with NaN (Primitive)
function testP3_StrictEqual_NaN() {
  const nanPrimitiveA = NaN;
  const nanPrimitiveB = NaN;
  
  // EXPECTED RESULT: false (NaN rule holds)
  return nanPrimitiveA === nanPrimitiveB;
}

// P4: Strict Equality between two Identical Primitives
function testP4_StrictEqual_Identical() {
  const strPrimitive1 = 'Key';
  const strPrimitive2 = 'Key';
  
  // EXPECTED RESULT: true
  return strPrimitive1 === strPrimitive2;
}

// P5: Strict Equality with a Primitive Literal
function testP5_StrictEqual_Literal() {
  const numPrimitive = 999;
  const literalNum = 999;
  
  // EXPECTED RESULT: true
  return numPrimitive === literalNum;
}