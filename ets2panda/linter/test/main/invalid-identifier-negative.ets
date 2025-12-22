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

import { number } from './invalid_identifier_number.js' // invalid import keyword
import abstract from './invalid_identifier_number.js' // invalid import keyword
import {value1 as interface} from './invalid_identifier_number.js' // invalid import keyword

// invalid keyword declarations
const abstract: string = '';
const enum: string = '';
const let: string = '';
const this: string = '';
const as: string = '';
const export: string = '';
const native: string = '';
const throw: string = '';
const async: string = '';
const extends: string = '';
const new: string = '';
const true: string = '';
const await: string = '';
const false: string = '';
const null: string = '';
const try: string = '';
const break: string = '';
const final: string = '';
const overload: string = '';
const typeof: string = '';
const case: string = '';
const for: string = '';
const override: string = '';
const undefined: string = '';
const class: null = null;
const function: boolean = true;
const private: boolean = false;
const while: string = '';
const const: string = '';
const if: string = '';
const protected: string = '';
const constructor: string = '';
const implements: string = '';
const public: string = '';
const continue: string = '';
const import: string = '';
const return: string = '';
const default: string = '';
const in: string = '';
const static: string = '';
const do: string = '';
const instanceof: string = '';
const switch: string = '';
const else: string = '';
const interface: string = '';
const super: string = '';
const Any: string = '';
const bigint: string = '';
const BigInt: string = '';
const boolean: string = '';
const Boolean: string = '';
const byte: null = null;
const Byte: boolean = true;
const char: boolean = false;
const Char: string = '';
const double: string = '';
const Double: string = '';
const float: string = '';
const Float: string = '';
const int: string = '';
const Int: string = '';
const long: string = '';
const Long : string = '';
const number: string = '';
const Number: string = '';
const Object: string = '';
const object: null = null;
const short: boolean = true;
const Short: boolean = false;
const string: string = '';
const String: string = '';
const void: string = '';
const Intl: string = '';
const Global: null = null;


import Any   from './test'   //error: default import
import {bigint , BigInt } from './test'    //Named imports have already reported errors in the test file, so no further reporting is needed
import {int as int} from './test'    //error: Alias import
import * as Number from './test'    //error: Alias default import
import Short , { object, as } from './test'   //error: Short--> default import



function test1(string: string): string {  // error: function name
  return string;
}
function test2(number: number): number {  // error: function name
  return number;
}
function test3(boolean: boolean): boolean {  // error: function name
  return boolean;
}


function number(aa: number): number {  // error: function parameters name
  return aa;
}
function boolean(aa: boolean): boolean {  // error: function parameters name
  return aa;
}

class String { // error: class name
  private aa: string = "const";
  public bb: string = "void";
  protected cc: string = "static";
}

class A {
  private const: string = "const";  // error: class property name
  public void: string = "void";  // error: class property name
  protected static: string = "static";  // error: class property name
  number(aa: number){   // error: class method name
    console.log('' + aa);
  }
}

@Entry
@Component
struct Char { // error: struct name
  @State message: string = 'Hello World';
  aa:number = 123;
  bb:number = 123;
  cc:number = 123;
  build() {

  }
}

enum char { // error: enum name
  aa,
  bb = 2,
  cc = 3,
}

enum AA {
  short,  // error: enum member name
  number = 2, // error: enum member name
  int = 3,  // error: enum member name
}

namespace char {  // error: namespace name
  let aa : number
}

namespace quarantine {
  interface int {// error: interface name
    aa: number
  }

  interface test {
    aa: number
  }
}

namespace quarantine {
  interface A {
    test: number
  }

  interface B {
    int: number  // error: interface property name
    number():void  // error: interface method name
  }
}

type byte  = number;  //error

export const Byte  = 'int'  //export declarations name
let long: string = 'long';
let Long: string = 'Long'
export {long, Long}    //The export does not report an error because an error was already reported earlier in the file
export {long as Int, Long as let}    //Rename export error
export {static, private} from './test'  //The export does not report an error because an error was already reported earlier in the file
export {static as double, private as Double} from './test'  //Rename export error
export type { do, in } from './test'  //The export does not report an error because an error was already reported earlier in the file


interface B{
  get(){} //error: get keyword in interface
  set: ()=>void = ()=>{}  //error: set keyword in interface
}