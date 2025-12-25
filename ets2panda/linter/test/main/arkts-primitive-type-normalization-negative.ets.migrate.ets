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

// P1: typeof Check yielding the old result
function testP1_Typeof_OldResult() {
  return typeof new Number(1);      // Will print the warning, it will return Object
}

// P2: Conditional Check yielding the old result
function testP2_Conditional_OldResult() {
  if (new Boolean(false)) {          // Will return false always at 1.2 and true in 1.1, the usage will be flagged for the eroor
    return true;
  }
  return false;
}

// P3: Loose Equality (==) with a Primitive Literal
function testP3_LooseEqual_Literal() {
  const numWrapper = new Number(10);        // Same as the above tests, it will be a number in 1.2 and Number in 1.1
  return numWrapper == 10;
}

// P4: Strict Equality (===) with a Primitive Literal
function testP4_StrictEqual_Literal() {
  const strWrapper = new String('a');        // All uses of Boxed Primitives(String, Boolean, Number) with class instantiation will be flagged with the error
  return strWrapper === 'a';
}

// P5: Method Invocation on the Wrapper Object
function testP5_Method_Invocation() {
  return new Number(1.23).toFixed(1);        // All uses of Boxed Primitve will print the error, this is another use case
}

// P6: Using the Old Wrapper Type in a Generic (Array)
function testP6_GenericWrapperType() {
  const wrapperArray: Array<String> = [new String('a'), new String('b')];        // All uses of Boxed Primitve will print the error, this is another use case
  return wrapperArray;
}

// P7: Assigning the Result to the Old Wrapper Type
function testP7_ExplicitTypeAssignment() {
  const b: Boolean = new Boolean(true);        // All uses of Boxed Primitve will print the error, this is another use case
  return b;
}

// P8: Function Argument Expecting the Old Wrapper Type
function processOldWrapper(s: String): number {       
  return s.length;
}
function testP8_FunctionCall() {
  return processOldWrapper(new String('test'));        // All uses of Boxed Primitve will print the error, this is another use case
}

// P9: Object.prototype methods on wrapper
function testP9_ObjectPrototype() {
  const wrapper = new String('test');        // All uses of Boxed Primitve will print the error, this is another use case
  return wrapper.toString();
}

// P10: Template literal coercion
function testP10_Template_Literal() {
  const strWrapper = new String('world');        // All uses of Boxed Primitve will print the error, this is another use case
  return `Hello ${strWrapper}`;
}