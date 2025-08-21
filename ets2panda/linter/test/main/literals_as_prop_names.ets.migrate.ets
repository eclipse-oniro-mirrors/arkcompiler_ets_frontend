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

import {
  Component,
  State,
} from '@kit.ArkUI';
 
import { ExportLitAsPropName } from './ignore_files/good';

enum LiteralAsPropertyNameEnum {
  One = "one",
  PrivateTwo = "_2",
  Two = "Two"
}

class LiteralAsPropertyName {
  public one: string = "1111111111";
  private __2: string;
  Two: number;
}

const litAsPropName: LiteralAsPropertyName = {
  one: "1",
  __2: 'two',
  Two: 2,
};

console.log(litAsPropName.one);
console.log(litAsPropName.__2);
console.log(litAsPropName.Two);

class LiteralAsPropertyName_fix {
  public one: string = "1111111111";
  private _2: string;
  Two: number;
}

const litAsPropName_fix: LiteralAsPropertyName_fix = {
  one: "1111111111",
  _2: 'two',
  Two: 2,
};

console.log("Fixed listAsPropName:");
console.log(litAsPropName_fix.one);
console.log(litAsPropName_fix._2);
console.log(litAsPropName_fix.Two);

let x = {"name": 20, 2: 30}

console.log(x["name"]);
console.log(x[2]);

class X_class {
    public name: number;
    public _2: number;
}

interface GeneratedObjectLiteralInterface_1 {
    name: number;
    _2: number;
}
let x_fix: GeneratedObjectLiteralInterface_1 = {name: 20, _2: 20};

console.log("Fixed x object literal:");
console.log(x_fix.name);
console.log(x_fix._2);

interface litAsPropNameIface {
  one: string;
  ___2: string;
  __2: number;
}
const int: litAsPropNameIface = {
  one: '12321',
  ___2: 'weqwewq',
  __2: 123
};

const imp: ExportLitAsPropName = { 1: 234 };

LiteralAsPropertyNameEnum['One']

LiteralAsPropertyNameEnum.PrivateTwo;

{
  const enum Direction {
   __empty = 1,
  }
}

const enum Direction16 {
  ___x5c = 1,
  __x5c = 1,
}

const enum Direction17 {
  ___x5c = 1,
  __x5c = 1,
}
let case17 = Direction17.___x5c
let case172 = Direction17.__x5c

const enum Direction11 {
__x21x21 = 1,
}
const enum Direction23 {
aaa = 1,
}
// ArkUI
@Component
struct Index {
@State message: string = 'Hello World';
private case11 = Direction11.__x21x21
build() {
}
}


class A{
 public age:number = 1; 
}

let a:A = { "age": 30}  


class B {
  public age: number = 1 // error in arkts2
}

let obj11: Record<string, number> = {
  ['value']: 1, // Error in arkts 2.0
  'value2': 1 // ok
}

class CompPropClass {
  ['CompProp'] = 1; // Error in arkts 2.0
  [2] = 'CompProp2'; // Error in arkts 2.0
  [LiteralAsPropertyNameEnum.One] = 3; // Error in arkts 2.0
}

let compPropObj = {
  ['CompProp']: 1, // Error in arkts 2.0
  [2]: 'CompProp2', // Error in arkts 2.0
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

let de3 = TEST2[TEST.A] //error
let de4 = TEST2[TEST.A| TEST.B] //error
let de5 = TEST[TEST.C] //ok

export enum DeviceTypes12 {
  PHONE = 'phone',
  TABLET = 'tablet',
  __2IN1 = '2in1',
}

export class A{
  public static a= DeviceTypes12.__2IN1
}