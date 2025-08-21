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

interface GeneratedObjectLiteralInterface_1 {
    name: string;
    idx: number;
    handler: string;
}
export function animations() {
  let anyvar: any = undefined;
  let symvar: symbol;
  let unknownType: unknown;
  let undefType: undefined;
  let nullType: null;
  const state = 0;

  const flag = false;

  const status: GeneratedObjectLiteralInterface_1 = { name: 'A', idx: 0, handler: 'foo' };

  type Person = [string, number];
  const user: Person = ['John', 32];
  const age = user[1];

  interface Point {
    x: number;
    y: number;
}
  type P = keyof Point;
  type AxeX = Point['x'];
  type P_NULL = P | null;
  type P_UNKNOWN = P | unknown;
  type P_ANY = P | any;
  type P_P = P | Person;
  type P_P_NULL = P | Person | null;

  const typeU = typeof user;

  let isNumber: (x: any) => x is number = (x: any): x is number => {
    return typeof x === 'number';
};

  const regex = /go*d/;

  throw new TypeError('labuda');
}

const c = 'c';
const d = 10;
interface ComputedPropertyT {
    a: string; // String-like name
    __5: string; // Number-like name
    [c]: string; // String-like name
    [d]: string; // Number-like name
}

class LiteralAsPropertyName {
  __2: string;
  Two: number;
}

const litAsPropName: LiteralAsPropertyName = {
  __2: 'two',
  Two: 2,
};

interface Dictionary {
    [key: string]: unknown;
}
let dict: Dictionary;

function bar(key: string, val: any) {
  if (key in dict) {
    dict[key] = val;
  }
}

interface I1 {
  m1(): number;
  m2(): void;
}

interface I2 {
  m2(): string;
  m3(): boolean;
}

type IntersectionT = I1 & I2;

interface DescribableFunction {
    description: string;
    (someArg: number): boolean;
}
function callFunctionObject(fn: DescribableFunction) {
  console.log(fn.description + ' returned ' + fn(5));
}
const funcWithDescr: DescribableFunction = (x: number) => x % 2 == 0;
funcWithDescr.description = 'isEven';
callFunctionObject(funcWithDescr);

class G<T> {
  val: T;
  getVal(): T {
    return this.val;
  }
}
interface GeneratedTypeLiteralInterface_1 {
    x: 2;
}
class H extends G<GeneratedTypeLiteralInterface_1> {}
interface GeneratedTypeLiteralInterface_2 {
    y: string;
}
interface GeneratedTypeLiteralInterface_3 {
    y: 'constant';
}
const g: G<GeneratedTypeLiteralInterface_2> = new G<GeneratedTypeLiteralInterface_3>();
function generic<T>(t: T): number {
  return 10;
}
interface GeneratedTypeLiteralInterface_4 {
    z: boolean;
}
generic<GeneratedTypeLiteralInterface_4>({ z: true });

function typeAssertions(): void {
  const num = 1 as any;
  const myCanvas = document.getElementById('main_canvas') as HTMLCanvasElement;
}

interface GeneratedObjectLiteralInterface_2 {
    a: number;
    b: string;
}
function dynamicProperties(): void {
  const x: GeneratedObjectLiteralInterface_2 = { a: 5, b: 'text' };
  x['c'] = 100200;
  console.log(x['c']);

  const y: any = { q: 100, w: true };
  y.e = 'dynamic';
  console.log(y.e);
}

function genericArrayType(): void {
  const x: Array<string> = ['1', '2', '3'];
  const y: Array<number> = new Array(1, 2, 3);
  const z: number[] = [1, 2, 3];

  let arrayFunc: <T extends object>(array: Array<T>) => Array<string> = <T extends object>(array: Array<T>): Array<string> => {
    return array.map((x) => x.toString());
};
}

class C {
  constructor(n: number) {
  }
}
function undefinedValue(n: string): void {
  n = undefined;
  undefinedValue(undefined);
  let c = new C(undefined);
}

let a1 = [0,1]
let a2 = [1,2,3,4,5]
let a3: number[] = [1,2,3,4,5]
let a4: Array<number> = [1,2,3,4,5]
let a5 = new Array(10)
let a6 = new Int8Array(10)
let a7 = new Uint8Array(10)
let a8 = new Uint8ClampedArray(10)
let a9 = new Int16Array(10)
let a10 = new Uint16Array(10)
let a11 = new Int32Array(10)
let a12 = new Uint32Array(10)
let a13 = new Float32Array(10)
let a14 = new Float64Array(10)
let a15 = new BigInt64Array(10)
let a16 = new BigUint64Array(10)

a1[0] = 1
a2[0] = 1
a3[0] = 1
a4[0] = 1
a5[0] = 1
a6[0] = 1
a7[0] = 1
a8[0] = 1
a9[0] = 1
a10[0] = 1
a11[0] = 1
a12[0] = 1
a13[0] = 1
a14[0] = 1
a15[0] = BigInt(1)
a16[0] = BigInt(1)
