/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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
import {ohFunction1, ohFunction2, OhosI} from './oh_modules/ohos_lib'
interface I {
  a: number;
  b: string;
}

class C {
  a: number;
  b: string;
}

interface GeneratedTypeLiteralInterface_1 {
    x: number;
    y: string;
}
class C2 {
  q: GeneratedTypeLiteralInterface_1;
  w: any;
  e: I;
  r: C;
}

interface GeneratedObjectLiteralInterface_1 {
    a: number;
    b: string;
}
function localVariable(): void {
  // Declaration
  let a1: GeneratedObjectLiteralInterface_1 = { a: 1, b: 'a' }; // NOT OK
  let a2: any = { a: 2, b: 'b' }; // OK - ASSIGNMENT TO ANY
  let a3: { a: number; b: string } = { a: 30, b: 'c' }; // NOT OK
  let a4: I = { a: 4, b: 'd' }; // OK
  let a5: C = { a: 5, b: 'e' }; // OK
  let a6: C2 = {
    // OK
    q: { x: 6, y: 'f' }, // NOT OK
    w: { a: 7, b: 'g' }, // OK - ASSIGNMENT TO ANY
    e: { a: 8, b: 'h' }, // OK
    r: { a: 9, b: 'i' }, // OK
  };

  // Assignment
  a1 = { a: 11, b: 'a' }; // NOT OK
  a2 = { a: 12, b: 'b' }; // OK - ASSIGNMENT TO ANY
  a3 = { a: 13, b: 'c' }; // NOT OK
  a4 = { a: 14, b: 'd' }; // OK
  a5 = { a: 15, b: 'e' }; // OK
  a6 = {
    // OK
    q: { x: 16, y: 'f' }, // NOT OK
    w: { a: 17, b: 'g' }, // OK - ASSIGNMENT TO ANY
    e: { a: 18, b: 'h' }, // OK
    r: { a: 19, b: 'i' }, // OK
  };
}

interface GeneratedObjectLiteralInterface_2 {
    a: number;
    b: string;
}
function defaultParamValue(): void {
  function foo(x: GeneratedObjectLiteralInterface_2 = { a: 21, b: 'a' }) {
    console.log(x.a, x.b);
  } // NOT OK
  function foo2(x: any = { a: 22, b: 'b' }) {
    console.log(x.a, x.b);
  } // NOT OK
  function foo3(x: { a: number; b: string } = { a: 23, b: 'c' }) {
    console.log(x.a, x.b);
  } // NOT OK
  function foo4(x: I = { a: 24, b: 'd' }) {
    console.log(x.a, x.b);
  } // OK
  function foo5(x: C = { a: 25, b: 'e' }) {
    console.log(x.a, x.b);
  } // OK

  // Function call
  foo({ a: 21, b: 'a' }); // NOT OK
  foo2({ a: 22, b: 'b' }); // OK - ASSIGNMENT TO ANY
  foo3({ a: 23, b: 'c' }); // NOT OK
  foo4({ a: 24, b: 'd' }); // OK
  foo5({ a: 25, b: 'e' }); // OK
}

interface GeneratedObjectLiteralInterface_3 {
    a: number;
    b: string;
}
function returnFromFunction(): void {
  function bar() {
    return ({ a: 31, b: 'a' } as GeneratedObjectLiteralInterface_3);
  } // NOT OK
  let bar2: () => any = (): any => {
    return { a: 32, b: 'b' };
}; // OK - ASSIGNMENT TO ANY
  let bar3: () => {
    a: number;
    b: string;
} = (): {
    a: number;
    b: string;
} => {
    return { a: 33, b: 'c' };
}; // NOT OK
  let bar4: () => I = (): I => {
    return { a: 34, b: 'd' };
}; // OK
  let bar5: () => C = (): C => {
    return { a: 35, b: 'e' };
}; // OK
}

interface GeneratedObjectLiteralInterface_4 {
    a: number;
    b: string;
}
function ternaryOperator(): void {
  // In ternary operator
  const condition = true;
  const a1 = condition ? ({ a: 41, b: 'a' } as GeneratedObjectLiteralInterface_4) : { a: 42, b: 'b' }; // NOT OK
  const a2: any = condition ? { a: 43, b: 'c' } : { a: 44, b: 'd' }; // OK - ASSIGNMENT TO ANY
  const a3: { a: number; b: string } = condition
    ? { a: 45, b: 'e' }
    : { a: 46, b: 'f' }; // NOT OK
  const a4: I = condition ? { a: 47, b: 'g' } : { a: 48, b: 'h' }; // OK
  const a5: C = condition ? { a: 49, b: 'i' } : { a: 50, b: 'j' }; // OK
}

interface GeneratedObjectLiteralInterface_6 {
    a: number;
    b: string;
}
function arrayLiteral(): void {
  const arr1 = [
    ({ a: 51, b: 'a' } as GeneratedObjectLiteralInterface_6),
    { a: 52, b: 'b' },
  ]; // NOT OK
  const arr2: any[] = [
    { a: 53, b: 'c' },
    { a: 54, b: 'd' },
  ]; // OK - ASSIGNMENT TO ANY
  const arr3: { a: number; b: string }[] = [
    { a: 55, b: 'e' },
    { a: 56, b: 'f' },
  ]; // NOT OK
  const arr4: I[] = [
    { a: 57, b: 'g' },
    { a: 58, b: 'h' },
  ]; // OK
  const arr5: C[] = [
    { a: 59, b: 'i' },
    { a: 60, b: 'j' },
  ]; // OK
}

enum E {
  OK,
  NO_OK,
}
interface I1 {
	v: E | number
}

interface I2 {
	v: E
}

let i1: I1 = {v:E.OK}
let i2: I2 = {v:E.NO_OK}

function g1(a: E) {
  let ii1: I1 = {v:a}
  let ii2: I2 = {v:a}
}

function g(): boolean {
  return true;
}
interface CondI {
  a: number;
}
let a1: CondI = {
  a: g() ? 0 : 1,
};
let b1: CondI = {
  a: (g() ? 0 : 1) as number,
};
let c1 = g() ? 0 : 1;
let d1: CondI = {
  a: c1,
};
let e1: CondI = {
a: 0|1|2|3
}
let f1: 0|1|2|3 = 3
let ff : CondI = {
    a: f1
}

let dict = new Map<string, string | number>();
dict.set('1', 123)

interface III {
  param?: string | number | boolean
}

let test1: III = { param: dict.get('1') } as III
let test2: III = { param: dict.get('1')! } as III
let test3: III = { param: dict.get('1') as number } as III
let test4: III = { param: dict.get('1') as (number | string) } as III
export interface Style {
}
export class SwitchMenuStyle implements Style {
}
export class ProfileOneLineSwitchMenuStyle extends SwitchMenuStyle {
}
export class ProfileSwitchMenuStyle extends SwitchMenuStyle {
}
export let hfpProfileSwitchMenuStyle = new ProfileSwitchMenuStyle();
export let hfpProfileOneLineSwitchMenuStyle = new ProfileOneLineSwitchMenuStyle();

export interface SettingsBaseMenuData {
  style?: Style;
}

function test(isDisConnected:boolean){
  let a={style: isDisConnected ? hfpProfileOneLineSwitchMenuStyle: hfpProfileSwitchMenuStyle} as SettingsBaseMenuData
}

interface PPP {
  x: number
  y: number | undefined
  z?: number
}

let p1: PPP = {x: 10, y: 10}
let p2: PPP = {x: 10, y: undefined}
let p3: PPP = {x: 10, y: undefined, z: undefined}
let p4: PPP = {x: 10, y: undefined, z: 10}
let p5: PPP = {x: 10, y: 10, z: 10}
const cp1: PPP = {x: 10, y: 10}
const cp2: PPP = {x: 10, y: undefined}
const cp3: PPP = {x: 10, y: undefined, z: undefined}
const cp4: PPP = {x: 10, y: undefined, z: 10}
const cp5: PPP = {x: 10, y: 10, z: 10}

const oi: OhosI = { f: 1 };

ohFunction1({d: oi})
ohFunction1({d: {f: 1}})
ohFunction2({d: oi})
ohFunction2({d: {f: 1}})
