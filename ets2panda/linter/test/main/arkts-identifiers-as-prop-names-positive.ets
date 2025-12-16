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

// P1: Basic Enum with Unquoted Member\
enum Colors {
  red = '1',
  blue = '2'
}
function testP1_Enum_Unquoted() {
  return Colors.red;
}

class Point {
  a: number = 1.0;
}
// P2: Class with Basic Unquoted Property
function testP2_Class_UnquotedProperty() {
  const p = new Point();
  // EXPECTED RESULT: 1.0
  return p.a;
}

class Box3 {
  width: number = 10;
  height = 5; 
}
// P3: Class with Unquoted Property and Initializer
function testP3_Class_UnquotedProperty_Initialized() {
  const b = new Box3();
  // EXPECTED RESULT: 5
  return b.height;
}

enum StatusCodes {
  OK = 200,
  NotFound = 404
}
// P4: Enum with Unquoted Numerical Members 
function testP4_Enum_Unquoted_Numerical() {
  // EXPECTED RESULT: 404
  return StatusCodes.NotFound;
}
class Calculator {
  add(a: number, b: number) {
    return a + b;
  }
}

// P5: Class with Unquoted Method Name
function testP5_Class_UnquotedMethod(): number {
  // Ensures method names also follow the unquoted rule
  const calc = new Calculator();
  // EXPECTED RESULT: 7
  return calc.add(3, 4);
}

class C6 {
  optionalProp?: string;
}
// P6: Class with Optional Unquoted Property
function testP6_Class_Optional() {
  const c6 = new C6();
  // EXPECTED RESULT: undefined
  return c6.optionalProp;
}

class C7 {
  static count: number = 0;
}
// P7: Class with Static Unquoted Property
function testP7_Class_Static() {
  // EXPECTED RESULT: 0
  return C7.count;
}

class Parent {
  id: number = 1;
}

class Child extends Parent {
  name: string = "Junior";
}
// P8: Class Inheritance with Unquoted Properties
function testP8_Class_Inheritance() {
  const child = new Child();
  // EXPECTED RESULT: "Junior"
  return child.name;
}

class C9<T> {
  value: T;
  constructor(v: T) {
    this.value = v;
  }
}
// P9: Generic Class with Unquoted Property
function testP9_Class_Generic() {
  const c9 = new C9<string>("test value");
  // EXPECTED RESULT: "test value"
  return c9.value;
}