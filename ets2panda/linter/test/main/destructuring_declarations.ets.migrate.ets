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

// Object destructuring
let GeneratedDestructObj_1 = { a: 100, b: 200, c: 'bar' };
let a = GeneratedDestructObj_1.a;
let b = GeneratedDestructObj_1.b;
let c = GeneratedDestructObj_1.c;
 // NOT OK
let { a2, ...rest2 } = { a2: 1, b2: 2 }; // NOT OK
let GeneratedDestructObj_2 = { a3: 1, b3: { c3: 3, d3: 'baz' }};
let a3 = GeneratedDestructObj_2.a3;
let { c3, d3: e3 } = GeneratedDestructObj_2.b3;
 // NOT OK
let { a4, b4: { ...rest4 } } = { a4: 1, b4: { c4: 3, d4: 'bah' }}; // NOT OK

let { a, b, c } = {}; // NOT OK, not fixable

interface GeneratedObjectLiteralInterface_1 {
    a5: number;
    b5: number;
}
function getObject() {
  return ({ a5: 1, b5: 2 } as GeneratedObjectLiteralInterface_1);
}
let GeneratedDestructObj_3 = getObject();
let a5 = GeneratedDestructObj_3.a5;
let b5 = GeneratedDestructObj_3.b5;
 // NOT OK

// Array destructuring
const GeneratedDestructArray_1 = [10, 20, 30];
const a6 = GeneratedDestructArray_1[0];
const b6 = GeneratedDestructArray_1[1];
const c6 = GeneratedDestructArray_1[2];

const [a7, b7, ...rest7] = [10, 20, 30, 40, 50]; // NOT OK
const [a8, b8, [c8, e8]] = [10, 20, [300, 400], 50];
const [a9, b9, [c9, ...rest9]] = [10, 20, [30, 40, 50]]; // NOT OK

const GeneratedDestructArray_2 = [[1, 2], [3, 4], [5, 6]];
const [a1, a2] = GeneratedDestructArray_2[0];
const [b1, b2] = GeneratedDestructArray_2[1];
const [c1, c2] = GeneratedDestructArray_2[2];
  
const GeneratedDestructArray_3 = [[1, 2, 3], [3, 4, 5], [5, 6, 7]];
const [a1, a2, a3] = GeneratedDestructArray_3[0];
const [b1, b2, b3] = GeneratedDestructArray_3[1];
const [c1, c2, c3] = GeneratedDestructArray_3[2];
 

let tuple: [number, string, number] = [1, '2', 3];
let [a10, , b10] = tuple;
let [a11, ...rest11] = tuple; // NOT OK

const getArray = (): number[] => [1, 2, 3];
let GeneratedDestructArray_4 = getArray();
let a12 = GeneratedDestructArray_4[0];
let b12 = GeneratedDestructArray_4[1];


const set: Set<number> = new Set([1, 2, 3, 4]);
let [a13, b13, c13] = set; // NOT OK

const map: Map<number, number> = new Map();
let [[a14, b14], [c14, d14]] = map; // NOT OK

// Mixed destructuring
let [a15, b15, [x15, { f15 }]] = [1, 2, [{ e15: 20 }, { f15: 5 }]]; // NOT OK
let [a16, b16, {e16, e16: f16, ...g16}] = [1, 2, {e16: 10}]; // NOT OK
{ let [a17, b17, ...{length}] = [1, 2, 3, 4]; } // NOT OK

let GeneratedDestructObj_4 = { a18: 1, b18: [2, 3] };
let a18 = GeneratedDestructObj_4.a18;
let [c18, d18] = GeneratedDestructObj_4.b18;
 // NOT OK
let GeneratedDestructObj_5 = { a19: 10, b19: { c19: 'foo', d19: [30, 40] } };
let a19 = GeneratedDestructObj_5.a19;
let {
    c19,
    d19: [e19, f19],
  } = GeneratedDestructObj_5.b19;
 // NOT OK

// test for default value
let { a, b, c = 9 } = { a: 100, b: 200, c: 'bar' }; // NOT OK
let { a3, b3: { c3 = 10, d3: e3 } } = { a3: 1, b3: { c3: 3, d3: 'baz' }}; // NOT OK
let { a5, b5 = 7 } = getObject(); // NOT OK
let { a18, b18: [c18 = 66, d18] } = { a18: 1, b18: [2, 3] }; // NOT OK
let {
  a19,
  b19: {
    c19,
    d19: [e19, f19 = 9],
  },
} = { a19: 10, b19: { c19: 'foo', d19: [30, 40] } }; // NOT OK

// test for new Expression
class C {
  public a: number = 10;
  public b: number = 10;
  public s: string = "0000";
}

let GeneratedDestructObj_6 = new C();
let a = GeneratedDestructObj_6.a;
let b = GeneratedDestructObj_6.b;
let s = GeneratedDestructObj_6.s;
 // NOT OK