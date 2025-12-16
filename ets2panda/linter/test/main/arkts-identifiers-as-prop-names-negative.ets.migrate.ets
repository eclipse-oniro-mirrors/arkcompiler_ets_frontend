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

import {
  Component,
  State,
} from '@kit.ArkUI';

class C1 {
  validName: string = "";    // Error: string is a predefined type, cannot be used as an identifier.
}
// P1: Quoted Valid Identifier (Class Property)
function testP1_Class_QuotedValid() {
  const c1 = new C1();
  return c1.validName;    // Error: Since the property is not valid, this won't find the property.
}

enum E1 {
  status = 1    // Error: Number, string or computed value property name 'status' is not allowed, 
                  // use classes to access data by property names that are identifiers
}
// P2: Quoted Valid Identifier (Enum Member)
function testP2_Enum_QuotedValid() {
  return E1.status;    // Error: Since the property is not valid, this won't find the property.
}

enum E2 {
  'default' = 100    // Error: Number, string or computed value property name 'default' is not allowed, 
                     // use classes to access data by property names that are identifiers
}
// P3: Reserved Keyword (Enum Member)
function testP3_Enum_ReservedKeyword() {
  return E2['default'];    // Error: Since the property is not valid, this won't find the property.
}

enum E3 {
  log_key = 1    // Error: Number, string or computed value property name 'log key' is not allowed, 
                    // use classes to access data by property names that are identifiers
}
// P4: Invalid Identifier with Spaces (Enum Member)
function testP4_Enum_InvalidChar_Spaces() {
  return E3.log_key;    // Error: Since the property is not valid, this won't find the property.
}

enum E4 {
  __200OK = 200    // Error: Number, string or computed value property name '200OK' is not allowed, use classes to access data by property names that are identifiers.
}
// P5: Invalid Identifier Starting with a Number (Enum Member)
function testP5_Enum_Starts_With_Number() {
  return E4.__200OK;    // Error: Since the property is not valid, this won't find the property.
}

class C2 {
  id: number;    // Error: string is a predefined type, cannot be used as an identifier.
}
// P6: Quoted Valid Identifier (Single Quote Style)
function testP6_Class_SingleQuoteValid() {
  const c2 = new C2();
  return c2.id;    // Error: Since the property is not valid, this won't find the property.
}

interface I1 {
  readonlyProp: string;    // Error: 'Identifier expected, got 'string' literal.
}

class C3 {
  get "computedValue"() {    // Error: Call signatures in object types are not supported.
    return 1;
  }
}
// N7: Quoted Getter/Setter Name
function testP7_Class_QuotedAccessor() {
  const c3 = new C3();
  return c3.computedValue;    // Error: Since the property is not valid, this won't find the property.
}

enum LiteralAsPropertyNameEnum {
  One = "one",
  PrivateTwo = "_2",    // Error: 'Identifier expected, got 'string' literal.
  Two = "Two"
}

class LiteralAsPropertyName {
  public one: string = "1111111111";
  Two: number;    // Error: 'Identifier expected, got 'string' literal.
}

const litAsPropName: LiteralAsPropertyName = {
  one: "1",
  Two: 2,    // Error: 'Identifier expected, got 'string' literal.
};

console.log(litAsPropName.one.toString());
console.log(litAsPropName.Two.toString());    // Error: Since the property is not valid, this won't find the property.

class LiteralAsPropertyName_fix {
  public one: string = "1111111111";
  public _2: string = '0';
  Two: number = 0;
}

const litAsPropName_fix: LiteralAsPropertyName_fix = {
  one: "1111111111",
  _2: 'two',
  Two: 2,
};

console.log("Fixed listAsPropName:");
console.log(litAsPropName_fix.one);
console.log(litAsPropName_fix._2);
console.log(litAsPropName_fix.Two.toString());

class X_class {
  public name: number = 4;
  public _2: number = 4;
}

let x_fix: X_class = {name: 20, _2: 20};

console.log("Fixed x object literal:");
console.log(x_fix.name.toString());
console.log(x_fix._2.toString());

interface litAsPropNameIface {
  one: string;
  __2: number;    // Error: 'Identifier expected, got 'string' literal.
}
const int: litAsPropNameIface = {
  one: '12321',
  __2: 123    // Error: 'Identifier expected, got 'string' literal.
};

LiteralAsPropertyNameEnum['One']

LiteralAsPropertyNameEnum.PrivateTwo;

{
  const enum Direction {
    __empty = 1,    // Error: 'Identifier expected, got 'string' literal.
  }
}

const enum Direction16 {
  ___x5c = 1,    // Error: 'Identifier expected, got 'string' literal.
  __x5c = 1,    // Error: 'Identifier expected, got 'string' literal.
}

const enum Direction17 {
  ___x5c = 1,    // Error: 'Identifier expected, got 'string' literal.
  __x5c = 1,    // Error: 'Identifier expected, got 'string' literal.
}
let case17 = Direction17.___x5c    // Error: Since the property is not valid, this won't find the property.
let case172 = Direction17.__x5c    // Error: Since the property is not valid, this won't find the property.

const enum Direction11 {
  __x21x21 = 1,    // Error: 'Identifier expected, got 'string' literal.
}
const enum Direction23 {
  aaa = 1,    // Error: 'Identifier expected, got 'string' literal.
}
// ArkUI
@Component
struct Index1 {
  @State message: string = 'Hello World';
  private case11 = Direction11.__x21x21    // Error: Since the property is not valid, this won't find the property.
  build() {
  }
}

class B {
  public age: number = 1     // Error: 'Identifier expected, got 'string' literal.
}

let obj11: Record<string, number> = {
  ['value']: 1,     // Parameter declaration should have an explicit type annotation.
  'value2': 1    // Unexpected token ':'
}

class CompPropClass {
  ['CompProp'] = 1;    // Error in arkts 2.0
  [LiteralAsPropertyNameEnum.One] = 3; // Error in arkts 2.0
}

let compPropObj : CompPropClass = {
  ['CompProp']: 1, // Error in arkts 2.0
  [LiteralAsPropertyNameEnum.One]: 3 // Error in arkts 2.0
};

enum TEST {
  A,
  B,
  C,
}
enum TEST2 {
  A,
  B,
  C,
}

let de3 = TEST2[TEST.A]       // Error : Type TEST2 is not compatible with TEST
let de4 = TEST2[TEST.A| TEST.B]       // Error : Type TEST2 is not compatible with TEST

export enum DeviceTypes12 {
  PHONE = 'phone',
  TABLET = 'tablet',
  __2IN1 = '2in1',    // Error: Number, string or computed value property name '2IN1' is not allowed, use classes to access data by property names that are identifiers.
}

export class A{
  public static a= DeviceTypes12.__2IN1    // Error: Since the property is not valid, this won't find the property.
}

class TestKeywords {
  "number" = 1    // Error
  "string" = 'a'  // Error
}

enum TestKeywordsEnum {
  "const" = 'const',    // Error: Number, string or computed value property name 'const' is not allowed, use classes to access data by property names that are identifiers.
  "as" = 'as'    // Error: Number, string or computed value property name 'as' is not allowed, use classes to access data by property names that are identifiers.
}