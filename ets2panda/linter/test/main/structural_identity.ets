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

class A {
  getName(): string { return 'A'; }
  getType(): string { return 'class'; }
}
class B {
  getName(): string { return 'B'; }
  getType(): string { return 'type'; }
}
function foo(a1: A, ...a2: A[]): void {
}
foo(new B, new A, new B);//ERROR

let a =  new A;
a = new B;//ERROR

let b: B = new A;//ERROR

function bar(bArr: B[]): void {
  bArr[0] = new A;//ERROR
}

let name = (new B as A).getName();

class C extends B {
  getBase(): string { return 'B'; }
}

function goo(): B {
  return new B;
}
let cl: C = goo() as C;

function zoo(b: B): void {
}
zoo(cl as B);

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
fC(i);    // ERR, no inheritance relation
fC(ci);   // ERR, no inheritance relation
fC(cc);   // OK
fC(ii);   // ERR, no inheritance relation
fI(c);    // ERR, no inheritance relation
fI(i);    // OK
fI(ci);   // OK
fI(cc);   // ERR, no inheritance relation
fI(ii);   // OK
fDC(c);   // OK, assignment to dynamic
fDC(i);   // OK, assignment to dynamic
fDC(ci);  // OK, assignment to dynamic
fDC(cc);  // OK, assignment to dynamic
fDC(ii);  // OK, assignment to dynamic
fDI(c);   // OK, assignment to dynamic
fDI(i);   // OK, assignment to dynamic
fDI(ci);  // OK, assignment to dynamic
fDI(cc);  // OK, assignment to dynamic
fDI(ii);  // OK, assignment to dynamic
fOC(c);   // OK, assignment to dynamic
fOC(i);   // OK, assignment to dynamic
fOC(ci);  // OK, assignment to dynamic
fOC(cc);  // OK, assignment to dynamic
fOC(ii);  // OK, assignment to dynamic
fOI(c);   // OK, assignment to dynamic
fOI(i);   // OK, assignment to dynamic
fOI(ci);  // OK, assignment to dynamic
fOI(cc);  // OK, assignment to dynamic
fOI(ii);  // OK, assignment to dynamic

fC(dc);   // ERR, no inheritance relation
fC(di);   // ERR, no inheritance relation
fC(dci);  // ERR, no inheritance relation
fC(dcc);  // ERR, no inheritance relation
fC(dii);  // ERR, no inheritance relation
fC(dic);  // ERR, no inheritance relation
fI(dc);   // ERR, no inheritance relation
fI(di);   // ERR, no inheritance relation
fI(dci);  // ERR, no inheritance relation
fI(dcc);  // ERR, no inheritance relation
fI(dii);  // ERR, no inheritance relation
fI(dic);  // ERR, no inheritance relation
fDC(dc);  // OK, assignment to dynamic
fDC(di);  // OK, assignment to dynamic
fDC(dci); // OK, assignment to dynamic
fDC(dcc); // OK, assignment to dynamic
fDC(dii); // OK, assignment to dynamic
fDC(dic); // OK, assignment to dynamic
fDI(dc);  // OK, assignment to dynamic
fDI(di);  // OK, assignment to dynamic
fDI(dci); // OK, assignment to dynamic
fDI(dcc); // OK, assignment to dynamic
fDI(dii); // OK, assignment to dynamic
fDI(dic); // OK, assignment to dynamic
fOC(dc);  // OK, assignment to dynamic
fOC(di);  // OK, assignment to dynamic
fOC(dci); // OK, assignment to dynamic
fOC(dcc); // OK, assignment to dynamic
fOC(dii); // OK, assignment to dynamic
fOC(dic); // OK, assignment to dynamic
fOI(dc);  // OK, assignment to dynamic
fOI(di);  // OK, assignment to dynamic
fOI(dci); // OK, assignment to dynamic
fOI(dcc); // OK, assignment to dynamic
fOI(dii); // OK, assignment to dynamic
fOI(dic); // OK, assignment to dynamic

fC(oc);   // ERR, no inheritance relation
fC(oi);   // ERR, no inheritance relation
fC(oci);  // ERR, no inheritance relation
fC(occ);  // ERR, no inheritance relation
fC(oii);  // ERR, no inheritance relation
fC(oic);  // ERR, no inheritance relation
fI(oc);   // ERR, no inheritance relation
fI(oi);   // ERR, no inheritance relation
fI(oci);  // ERR, no inheritance relation
fI(occ);  // ERR, no inheritance relation
fI(oii);  // ERR, no inheritance relation
fI(oic);  // ERR, no inheritance relation
fDC(oc);  // OK, assignment to dynamic
fDC(oi);  // OK, assignment to dynamic
fDC(oci); // OK, assignment to dynamic
fDC(occ); // OK, assignment to dynamic
fDC(oii); // OK, assignment to dynamic
fDC(oic); // OK, assignment to dynamic
fDI(oc);  // OK, assignment to dynamic
fDI(oi);  // OK, assignment to dynamic
fDI(oci); // OK, assignment to dynamic
fDI(occ); // OK, assignment to dynamic
fDI(oii); // OK, assignment to dynamic
fDI(oic); // OK, assignment to dynamic
fOC(oc);  // OK, assignment to dynamic
fOC(oi);  // OK, assignment to dynamic
fOC(oci); // OK, assignment to dynamic
fOC(occ); // OK, assignment to dynamic
fOC(oii); // OK, assignment to dynamic
fOC(oic); // OK, assignment to dynamic
fOI(oc);  // OK, assignment to dynamic
fOI(oi);  // OK, assignment to dynamic
fOI(oci); // OK, assignment to dynamic
fOI(occ); // OK, assignment to dynamic
fOI(oii); // OK, assignment to dynamic
fOI(oic); // OK, assignment to dynamic

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

class Ct {
  a: number = 1
  b: string = ""
}

interface I {
  a: number
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
  x = xy; // ERR, 'X | Y' assigned to 'X'
  xy = x; // OK

  xy = xz; // ERR, 'X | Z' assigned to 'X | Y'
  xz = xy; // ERR, 'X | Y' assigned to 'X | Z'

  xyz = xz; // OK
  xz = xyz; // ERR, 'X | Y | Z' assigned to 'X | Z'
  
  x = w; // OK
  w = x; // ERR, 'X' assigned to 'W' 

  x = xw; // OK
  xw = x; // OK

  xw = zw; // ERR, 'Z | W' assigned to 'X | W'
  zw = xw; // ERR, 'X | W' assigned to 'Z | W'
  
  xz = zw; // OK
  zw = xz; // ERR, 'X | Z' assigned to 'Z | W'
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

  u3 as C1[] | C0[];
  u3 as C2<boolean>[];
  u3 as C2<C1>[];

  u3 as C1[] | C2<number>[] | C2<string>[] | C0[];
  u3 as C1[] | C2<string>[] | undefined[];
  u3 as C2<number>[] | C2<string>[] | C2<boolean>[];

  u3 as C1[] | C2<number>[] | C2<string>[];
  u3 as C2<string>[] | C2<number>[] | C0[];
  u3 as U3 | U4;
}


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
const a2: lang.ISendable = t2; // OK , Sendable can be thought of as implement ISendable
const a3: lang.ISendable = t2 as lang.ISendable; // OK 
const a4: lang.ISendable = t3; // ERROR
const a5: lang.ISendable = t3 as lang.ISendable; // ERROR


// Appears on SyntaxKind.BinaryExpression node
let b1:lang.ISendable;
b1 = t1; // OK
b1 = t2; // OK , Sendable can be thought of as implement ISendable
b1 = t2 as lang.ISendable; // OK
b1 = t3; // ERROR
b1 = t3 as lang.ISendable; // ERROR


// Appears on SyntaxKind.CallExpression node
function cf(value: lang.ISendable):void {}
cf(t1); // OK
cf(t2); // OK , Sendable can be thought of as implement ISendable
cf(t2 as lang.ISendable); // OK
cf(t3); // ERROR
cf(t3 as lang.ISendable); // ERROR

function cfT<T>(value: T):void {}
cfT<lang.ISendable>(t1); // OK
cfT<lang.ISendable>(t2); // OK , Sendable can be thought of as implement ISendable
cfT<lang.ISendable>(t2 as lang.ISendable); // OK
cfT<lang.ISendable>(t3); // ERROR
cfT<lang.ISendable>(t3 as lang.ISendable); // ERROR


// Appears on SyntaxKind.handleNewExpression node
class DC1 {
  name:string;

  constructor(value:lang.ISendable) {
    this.name = value.name;
  }
}
new DC1(t1); // OK
new DC1(t2); // OK , Sendable can be thought of as implement ISendable
new DC1(t2 as lang.ISendable); // OK
new DC1(t3); // ERROR
new DC1(t3 as lang.ISendable); // ERROR


// Appears on SyntaxKind.ObjectLiteralExpression/SyntaxKind.ArrayLiteralExpression node
interface EI1 {
  sendable: lang.ISendable;
}
const e1: EI1 = { sendable:t1 }; // OK
const e2: EI1 = { sendable:t2 }; // OK , Sendable can be thought of as implement ISendable， avoid [arkts-no-untyped-obj-literals] rule
const e3: EI1 = { sendable:t2 as lang.ISendable }; // OK
const e4: EI1 = { sendable:t3 }; // ERROR
const e5: EI1 = { sendable:t3 as lang.ISendable }; // ERROR
const e1s: EI1[] = [{ sendable:t1 }]; // OK
const e2s: EI1[] = [{ sendable:t2 }]; // OK , Sendable can be thought of as implement ISendable.  avoid [arkts-no-untyped-obj-literals] rule
const e3s: EI1[] = [{ sendable:t2 as lang.ISendable }]; // OK
const e4s: EI1[] = [{ sendable:t3 }]; // ERROR
const e5s: EI1[] = [{ sendable:t3 as lang.ISendable }]; // ERROR

// // Appears on SyntaxKind.PropertyDeclaration node
// PropertyDeclaration does not do [arkts-no-structural-typing] check
// export class FC1 {
//   prop1: lang.ISendable = t1; // OK
//   prop2: lang.ISendable = t2; // OK
//   prop3: lang.ISendable = t3; // OK
// }


// union
class GC1 { }
function ff1(value: lang.ISendable):void {}
function ff2(value: lang.ISendable | GC1):void {}
function ff3(value: TC1 | TC2): void { ff1(value); } // OK , Sendable can be thought of as implement ISendable
function ff4(value: TC1 | TC3): void { ff1(value); } // ERROR
function ff5(value: TC1): void { ff2(value); } // OK
function ff6(value: TC2): void { ff2(value); } // OK , Sendable can be thought of as implement ISendable
function ff7(value: TC3): void { ff2(value); } // ERROR
function ff8(value: TC1 | TC2): void { ff2(value); } // OK , Sendable can be thought of as implement ISendable
function ff9(value: TC1 | TC3): void { ff2(value); } // ERROR

/**
 * More contexts are checked in 'arkts 2.0' mode.
 */
class A_20 {}
class B_20 {}

// Return statement
function foo_20(): A_20 {
  return new B_20();// ERROR
}
function bar_20(): B_20 {
  return new A_20();// ERROR
}

// Array literal expression
const a_20 = new A_20();
const b_20 = new B_20();
const arrA_20: A_20[] = [a_20, b_20];// ERROR
const arrB_20: B_20[] = [a_20, b_20];// ERROR

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
let aa: A_10 = new B_10() as A_10; // ERROR


class F<T> {
}
class D extends B_10 {}

const imageSet: Set<aod.ImageType> = new Set(this.context.triggers?.map((t) => t.imageType));
const orderedImage: aod.ImageType[] = Array.from(imageSet); 

let obj1: A_10 = new A_10();
let obj2: B_10 = new B_10();
let obj3: B_10 = obj1; // Not OK
let obj4: B_10 = obj1 as B_10; // NOT OK
let obj5: B_10 = new A_10() as B_10; // NOT OK
let obj6: B_10 = new D() as B_10; // OK
let obj7: A_10 = test() as A_10; // NOT OK

let bb: F<B_10> = new F<A_10>(); // NOT OK
let cc1: F<B_10 | string> = new F<A_10>(); // NOT OK
let cc2: F<B_10 | A_10> = new F<A_10 | D>(); // OK
let cc3: F<A_10> = new F<A_10 | string>(); // OK
let arr1: Array<A_10> = [obj2] // NOT OK
let arr2: Array<A_10> = [obj1, obj2] // obj2 NOT OK
let arr3: Array<A_10 | string> = [obj1, obj2, 'hello'] // obj2 NOT OK
let arr4: Array<A_10 | B_10> = [obj1, obj2] // OK

function test(): B_10 {
  return obj2;
}

function test1(): B_10 {
  return obj1; // NOT OK
}

class D_10 {
  obj1: A_10 = new A_10();
  obj2: B_10 = new B_10();
  obj3: B_10 = obj1; // Not OK
  obj4: B_10 = obj1 as B_10; // NOT OK
  obj5: B_10 = new A_10() as B_10; // NOT OK
  obj6: B_10 = new D() as B_10; // OK
  arr1: Array<A_10> = [obj2] // NOT OK
  arr2: Array<A_10> = [obj1, obj2] // NOT OK
  arr3: Array<A_10 | string> = [obj1, obj2, 'hello'] // obj2 NOT OK
  b: F<B_10> = new F<A_10>(); // NOT OK
  c: F<B_10 | string> = new F<A_10>(); // NOT OK
  cc: F<B_10 | A_10> = new F<A_10 | D>(); // OK

  test(): B_10{
    let obj1: A_10 = new A_10();
    return obj1; // NOT OK
  }
}

function test_10(): B_10 {
  let obj1: A_10 = new A_10();
  return obj1; // NOT OK
}

class MyObj2 {

}
class MyObj1 {
  
}
class MyObj3 {

}
interface goodPerson extends IPerson {
  like: MyObj2,
  like1: MyObj3
}

let lily: goodPerson = {
  like: new MyObj1(),
  name: 'Alice',
  age: 30,
  sayHello:()=> {
      return new MyObj2()
  }
}

function foo2(rule:Record<string,string|Object>){
  let b:Array<string> = rule['123'] as Array<string>
}