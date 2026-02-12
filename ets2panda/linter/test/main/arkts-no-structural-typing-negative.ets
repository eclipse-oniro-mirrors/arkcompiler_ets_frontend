/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
  OhosLibC,
  OhosLibI,
  OhosLibIC,
  OhosLibII,
  OhosLibCC,
  OhosLibCI
} from './oh_modules/ohos_lib'

import {
  DynLibC,
  DynLibI,
  DynLibIC,
  DynLibII,
  DynLibCC,
  DynLibCI
} from './dynamic_lib'

import type { lang } from './@arkts.lang';

class IdentLibC {}
interface IdentLibI {}
interface IdentLibII extends IdentLibI {}
class IdentLibCC extends IdentLibC {}
class IdentLibCI implements IdentLibI {}

function fC(a: IdentLibC) {}
function fI(a: IdentLibI) {}

function fDC(a: DynLibC) {}
function fDI(a: DynLibI) {}

function fOC(a: OhosLibC) {}
function fOI(a: OhosLibI) {}

const c: IdentLibC = {};
const i: IdentLibI = {};
const ci: IdentLibCI = {};
const cc: IdentLibCC = {};
const ii: IdentLibII = {};

const dc: DynLibC = {};
const di: DynLibI = {};
const dci: DynLibCI = {};
const dcc: DynLibCC = {};
const dii: DynLibII = {};
const dic: DynLibIC = {};

const oc: OhosLibC = {};
const oi: OhosLibI = {};
const oci: OhosLibCI = {};
const occ: OhosLibCC = {};
const oii: OhosLibII = {};
const oic: OhosLibIC = {};

fC(c);    // OK
fC(cc);   // OK
fI(i);    // OK
fI(ci);   // OK
fI(ii);   // OK

fDC(dc);  // OK, assignment to dynamic
fDC(dcc); // OK, assignment to dynamic
fDI(di);  // OK, assignment to dynamic
fDI(dci); // OK, assignment to dynamic
fDI(dii); // OK, assignment to dynamic

fOC(oc);  // OK, assignment to dynamic
fOC(occ); // OK, assignment to dynamic
fOI(oi);  // OK, assignment to dynamic
fOI(oci); // OK, assignment to dynamic
fOI(oii); // OK, assignment to dynamic

// Structural typing is now relaxed for 'as' expression, all cases are OK
c as IdentLibC;
i as IdentLibC;
ci as IdentLibC;
cc as IdentLibC;
ii as IdentLibC;
c as IdentLibI;
i as IdentLibI;
ci as IdentLibI;
cc as IdentLibI;
ii as IdentLibI;
c as DynLibC;
i as DynLibC;
ci as DynLibC;
cc as DynLibC;
ii as DynLibC;
c as DynLibI;
i as DynLibI;
ci as DynLibI;
cc as DynLibI;
ii as DynLibI;
c as OhosLibC;
i as OhosLibC;
ci as OhosLibC;
cc as OhosLibC;
ii as OhosLibC;
c as OhosLibI;
i as OhosLibI;
ci as OhosLibI;
cc as OhosLibI;
ii as OhosLibI;

dc as IdentLibC;
di as IdentLibC;
dci as IdentLibC;
dcc as IdentLibC;
dii as IdentLibC;
dic as IdentLibC;
dc as IdentLibI;
di as IdentLibI;
dci as IdentLibI;
dcc as IdentLibI;
dii as IdentLibI;
dic as IdentLibI;
dc as DynLibC;
di as DynLibC;
dci as DynLibC;
dcc as DynLibC;
dii as DynLibC;
dic as DynLibC;
dc as DynLibI;
di as DynLibI;
dci as DynLibI;
dcc as DynLibI;
dii as DynLibI;
dic as DynLibI;
dc as OhosLibC;
di as OhosLibC;
dci as OhosLibC;
dcc as OhosLibC;
dii as OhosLibC;
dic as OhosLibC;
dc as OhosLibI;
di as OhosLibI;
dci as OhosLibI;
dcc as OhosLibI;
dii as OhosLibI;
dic as OhosLibI;

oc as IdentLibC;
oi as IdentLibC;
oci as IdentLibC;
occ as IdentLibC;
oii as IdentLibC;
oic as IdentLibC;
oc as IdentLibI;
oi as IdentLibI;
oci as IdentLibI;
occ as IdentLibI;
oii as IdentLibI;
oic as IdentLibI;
oc as DynLibC;
oi as DynLibC;
oci as DynLibC;
occ as DynLibC;
oii as DynLibC;
oic as DynLibC;
oc as DynLibI;
oi as DynLibI;
oci as DynLibI;
occ as DynLibI;
oii as DynLibI;
oic as DynLibI;
oc as OhosLibC;
oi as OhosLibC;
oci as OhosLibC;
occ as OhosLibC;
oii as OhosLibC;
oic as OhosLibC;
oc as OhosLibI;
oi as OhosLibI;
oci as OhosLibI;
occ as OhosLibI;
oii as OhosLibI;
oic as OhosLibI;

/**
 * Add the Sendable exception to the [arks-no-struct_typing] rule.
 */
@Sendable
class TC1 implements lang.ISendable {
  name:string = 'tc1';
}

@Sendable
class TC2 {
  name:string = 'tc2';
}

class TC3 {
  name:string = 'tc3';
}
const t1 = new TC1();
const t2 = new TC2();
const t3 = new TC3();


// Appears on SyntaxKind.VariableDeclaration node
const a1: lang.ISendable = t1; // OK
const a3: lang.ISendable = t2 as lang.ISendable; // OK


// Appears on SyntaxKind.BinaryExpression node
let b1:lang.ISendable;
b1 = t1; // OK
b1 = t2 as lang.ISendable; // OK


// Appears on SyntaxKind.CallExpression node
function cf(value: lang.ISendable):void {}
cf(t1); // OK
cf(t2 as lang.ISendable); // OK

function cfT<T>(value: T):void {}
cfT<lang.ISendable>(t1); // OK
cfT<lang.ISendable>(t2 as lang.ISendable); // OK


// Appears on SyntaxKind.ObjectLiteralExpression/SyntaxKind.ArrayLiteralExpression node
interface EI1 {
  sendable: lang.ISendable;
}
const e1: EI1 = { sendable:t1 }; // OK
const e3: EI1 = { sendable:t2 as lang.ISendable }; // OK
const e1s: EI1[] = [{ sendable:t1 }]; // OK
const e3s: EI1[] = [{ sendable:t2 as lang.ISendable }]; // OK

class A1 { v: number = 0 }
class B1 { v: number = 0 }
let a1 = new B1() as A1; // Should report error Cannot assert B1 as A1

class C3<T> {  }
class A3 { v: number = 0 }
class B3 { v: number = 0 }
let b3: C3<B3> = new C3<A3>(); // Should report error Cannot assign C3<A3> to C3<B3>

class A4 { u: number = 0 }
class B4 { u: number = 0 }
const func4: () => A4 = () => {
  return new B4(); // Should report error Function expecting A4 cannot return B4
}

function goo(): B {
  return new B;
}
let cl: C = goo() as C;

function zoo(b: B): void {
}
zoo(cl as B);
class Ct {
  a: number = 1
  b: string = ""
}

class Cz {
  x: Ct | null = new Ct();
  y: I = this.x as I
}

let x: Ct | null = new Ct();
let y: I = x as I

class X {}
class Y {}
class Z {}
class W extends X {}

function union(x: X, xy: X | Y, xz: X | Z, xyz: X | Y | Z, w: W, xw: X | W, zw: Z | W) {
  xy = x; // OK

  xyz = xz; // OK

  x = w; // OK

  x = xw; // OK
  xw = x; // OK

  xz = zw; // OK
}

class C0 {}
class C1 {}
class C2<T> {}

type U1 = number | string | boolean;
type U2 = number[] | string[] | boolean[];
type U3 = C1 | C2<number> | C2<string>;
type U4 = C1[] | C2<number>[] | C2<string>[];

function testUnionStructuralIdentityNegative(u1: U1, u2: U2, u3: U3, u4: U4) {
  // no CTE expected
  u1 as number;
  u1 as string;
  u1 as boolean;

  u1 as number | string;
  u1 as number | boolean;
  u1 as string | boolean;

  u1 as number | string | boolean;
  u1 as boolean | number | string;
  u1 as U1;

  // no CTE expected
  u2 as number[];
  u2 as string[];
  u2 as boolean[];

  u2 as number[] | string[];
  u2 as number[] | boolean[];
  u2 as string[] | boolean[];

  u2 as number[] | string[] | boolean[];
  u2 as boolean[] | number[] | string[];
  u2 as U2;

  // no CTE expected
  u3 as C1;
  u3 as C2<number>;
  u3 as C2<string>;

  u3 as C1 | C2<number>;
  u3 as C1 | C2<string>;
  u3 as C2<number> | C2<string>;

  u3 as C1 | C2<number> | C2<string>;
  u3 as C2<string> | C2<number> | C1;
  u3 as U3;

  // no CTE expected
  u4 as C1[];
  u4 as C2<number>[];
  u4 as C2<string>[];

  u4 as C1[] | C2<number>[];
  u4 as C1[] | C2<string>[];
  u4 as C2<number>[] | C2<string>[];

  u4 as C1[] | C2<number>[] | C2<string>[];
  u4 as C2<string>[] | C2<number>[] | C1[];
  u4 as U4;
}

function testUnionStructuralIdentityPositive(u1: U1, u2: U2, u3: U3, u4: U4) {
  u1 as Number;
  u1 as String;
  u1 as Boolean;

  u1 as Number | String;
  u1 as Number | Boolean;
  u1 as String | Boolean;

  u1 as Number | String | Boolean;
  u1 as Boolean | Number | String;
  u1 as U1 | U2;

  u2 as Number[];
  u2 as String[];
  u2 as Boolean[];

  u2 as Number[] | String[];
  u2 as Number[] | Boolean[];
  u2 as String[] | Boolean[];

  u2 as Number[] | String[] | Boolean[];
  u2 as Boolean[] | Number[] | String[];
  u2 as U2 | U1;

  u3 as C1 | C0;
  u3 as C2<boolean>;
  u3 as C2<C1>;

  u3 as C1 | C2<number> | C2<string> | C0;
  u3 as C1 | C2<string> | number;
  u3 as C2<number> | C2<string> | C2<boolean>;

  u3 as C1 | C2<number> | C2<boolean>;
  u3 as C2<string> | C2<number> | C0;
  u3 as U3 | U4;

  u3 as C1[] | C0[];  // Should report error,
  u3 as C2<boolean>[];  // Should report error,
  u3 as C2<C1>[];  // Should report error,

  u3 as C1[] | C2<number>[] | C2<string>[] | C0[];  // Should report error,
  u3 as C1[] | C2<string>[] | undefined[];  // Should report error,
  u3 as C2<number>[] | C2<string>[] | C2<boolean>[];  // Should report error,

  u3 as C1[] | C2<number>[] | C2<string>[];  // Should report error,
  u3 as C2<string>[] | C2<number>[] | C0[];  // Should report error,
  u3 as U3 | U4;  // Should report error,
}

/**
 * More contexts are checked in 'arkts 2.0' mode.
 */
class A_20 {}
class B_20 {}

// Return statement
function foo_20(): A_20 {
  return new B_20();// ERROR, not compatible type
}
function bar_20(): B_20 {
  return new A_20();// ERROR, not compatible type
}

// Array literal expression
const a_20 = new A_20();
const b_20 = new B_20();
const arrA_20: A_20[] = [a_20, b_20];// ERROR, not compatible type
const arrB_20: B_20[] = [a_20, b_20];// ERROR, not compatible type

// Property declaration
class C_20 {
  a: A_20 = new B_20();// ERROR
  b: B_20 = new A_20();// ERROR
}

class A_10 {
  v: number = 0
}
class B_10 {
  v: number = 0
}
let aa: A_10 = new B_10() as A_10; // ERROR, not compatible type


class F<T> {
}
class D extends B_10 {}

let obj1: A_10 = new A_10();
let obj2: B_10 = new B_10();
let obj4: B_10 = obj1 as B_10; // NOT OK, not compatible type
let obj5: B_10 = new A_10() as B_10; // NOT OK, not compatible type
let obj6: B_10 = new D() as B_10; // OK
let obj7: A_10 = test() as A_10; // NOT OK, not compatible type

let bb: F<B_10> = new F<A_10>(); // NOT OK, not compatible type
let cc1: F<B_10 | string> = new F<A_10>(); // NOT OK, not compatible type
let cc2: F<B_10 | A_10> = new F<A_10 | D>(); // NOT OK, not compatible type
let cc3: F<A_10> = new F<A_10 | string>(); // OK
let arr1: Array<A_10> = [obj2] // NOT OK, not compatible type
let arr2: Array<A_10> = [obj1, obj2] // obj2 NOT OK, not compatible type
let arr3: Array<A_10 | string> = [obj1, obj2, 'hello'] // obj2 NOT OK, not compatible type
let arr4: Array<A_10 | B_10> = [obj1, obj2] // OK

function test(): B_10 {
  return obj2;
}

class D_10 {
  obj1: A_10 = new A_10();
  obj2: B_10 = new B_10();
  obj3: B_10 = obj1; // Not OK, not compatible type
  obj4: B_10 = obj1 as B_10; // NOT OK, not compatible type
  obj5: B_10 = new A_10() as B_10; // NOT OK, not compatible type
  obj6: B_10 = new D() as B_10; // OK
  arr1: Array<A_10> = [obj2] // NOT OK, not compatible type
  arr2: Array<A_10> = [obj1, obj2] // NOT OK, not compatible type
  arr3: Array<A_10 | string> = [obj1, obj2, 'hello'] // obj2 NOT OK, not compatible type
  b: F<B_10> = new F<A_10>(); // NOT OK, not compatible type
  c: F<B_10 | string> = new F<A_10>(); // NOT OK, not compatible type
  cc: F<B_10 | A_10> = new F<A_10 | D>(); // NOT OK, not compatible type on arkTs 1.2

  test(): B_10{
    let obj1: A_10 = new A_10();
    return obj1; // NOT OK, not compatible type
  }
}

function test_10(): B_10 {
  let obj1: A_10 = new A_10();
  return obj1; // NOT OK, not compatible type
}

class MyObj2 {

}
class MyObj1 {

}
class MyObj3 {

}

function foo2(rule:Record<string,string|Object>){
  let b:Array<string> = rule['123'] as Array<string>
}
class A {}
class B {}
class C extends A {}

function test1(ab?: A | B) {
  const a = ab as A ?? new A(); // No error in ArkTS 1.2
  const b = ab as B ?? new B(); // No error in ArkTS 1.2
}

function test2(): A | null {
  return new B(); // Error in ArkTS 1.2, not compatible type
}

function test3(a: A): C | undefined {
  return a ? a as C : undefined; // No error in ArkTS 1.2
}

interface I {
  a?: A;
}
let i: I = {
  a: new A() // No error in ArkTS 1.2
};