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
let a = 5, b = 10, c = 15, d = 20, s = 'foo';
let rest, rest2;
let GeneratedDestructObj_1 = { a: 100, b: 200, s: 'bar' };
a = GeneratedDestructObj_1.a;
b = GeneratedDestructObj_1.b;
s = GeneratedDestructObj_1.s;
 // NOT OK
({ a, b, s } = {}); // NOT OK, not fixable
({ a, ...rest } = { a: 1, b: 2 }); // NOT OK
let GeneratedDestructObj_2 = { a: 1, b: { c: 3, d: 'baz' } };
a = GeneratedDestructObj_2.a;
({ c, d: s } = GeneratedDestructObj_2.b);
 // NOT OK
({ a, b: { ...rest } } = { a: 1, b: { c: 3, d: 'bah' }}); // NOT OK

interface GeneratedObjectLiteralInterface_1 {
    a: number;
    b: number;
}
function getObject() {
  return ({ a: 1, b: 2 } as GeneratedObjectLiteralInterface_1);
}
let GeneratedDestructObj_3 = getObject();
a = GeneratedDestructObj_3.a;
b = GeneratedDestructObj_3.b;
 // NOT OK

// Array destructuring
let GeneratedDestructArray_1 = [10, 20, 30];
a = GeneratedDestructArray_1[0];
b = GeneratedDestructArray_1[1];
c = GeneratedDestructArray_1[2];

[a, b, c] = [];
let GeneratedDestructArray_2 = [[1, 2], [3, 4], [5, 6]];
[a1, a2] = GeneratedDestructArray_2[0];
[b1, b2] = GeneratedDestructArray_2[1];
[c1, c2] = GeneratedDestructArray_2[2];
  
let GeneratedDestructArray_3 = [b, a];
a = GeneratedDestructArray_3[0];
b = GeneratedDestructArray_3[1];

[a, b, ...rest] = [10, 20, 30, 40, 50]; // NOT OK
[a, b, [c, d]] = [10, 20, [300, 400], 50];
[a, b, [c, ...rest]] = [10, 20, [30, 40, 50]]; // NOT OK

let tuple: [number, string, number] = [1, '2', 3];
[a, , b] = tuple;
[a, ...rest] = tuple; // NOT OK

const getArray = (): number[] => [1, 2, 3];
let GeneratedDestructArray_4 = getArray();
a = GeneratedDestructArray_4[0];
b = GeneratedDestructArray_4[1];


const set: Set<number> = new Set([1, 2, 3, 4]);
[a, b, c] = set; // NOT OK

const map: Map<number, number> = new Map();
[[a, b], [c, d]] = map; // NOT OK

// Mixed destructuring
interface GeneratedTypeLiteralInterface_1 {
    e: number;
}
let e: number, f: number, x: GeneratedTypeLiteralInterface_1, g;
[a, b, [x, { f }]] = [1, 2, [{ e: 20 }, { f: 5 }]]; // NOT OK
[a, b, {e, e: f, ...g}] = [1, 2, {e: 10}]; // NOT OK
[a, b, ...{length}] = [1, 2, {e: 10}]; // NOT OK

let GeneratedDestructObj_4 = { a: 1, b: [2, 3] };
a = GeneratedDestructObj_4.a;
[c, d] = GeneratedDestructObj_4.b;
 // NOT OK
let GeneratedDestructObj_5 = { a: 10, b: { s: 'foo', c: [30, 40] } };
a = GeneratedDestructObj_5.a;
({
    s,
    c: [d, f],
  } = GeneratedDestructObj_5.b);
 // NOT OK

// test for default value
({ a, b = 7, s } = { a: 100, b: 200, s: 'bar' }); // NOT OK
({ a, b: { c = 6, d: s } } = { a: 1, b: { c: 3, d: 'baz' }}); // NOT OK
({ a, b : [c, d = 8] } = { a: 1, b: [2, 3] }); // NOT OK
({
  a,
  b: {
    s,
    c: [d = 9, f],
  },
} = { a: 10, b: { s: 'foo', c: [30, 40] } }); // NOT OK


// test for new Expression
class C {
  public a: number = 10;
  public b: number = 10;
  public s: string = "0000";
}

let GeneratedDestructObj_6 = new C();
a = GeneratedDestructObj_6.a;
b = GeneratedDestructObj_6.b;
s = GeneratedDestructObj_6.s;
 // NOT OK

