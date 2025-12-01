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
({ a, b, s } = { a: 100, b: 200, s: 'bar' }); // NOT OK
({ a, b, s } = {}); // NOT OK, not fixable
({ a, ...rest } = { a: 1, b: 2 }); // NOT OK
({ a, b: { c, d: s } } = { a: 1, b: { c: 3, d: 'baz' }}); // NOT OK
({ a, b: { ...rest } } = { a: 1, b: { c: 3, d: 'bah' }}); // NOT OK

function getObject() {
  return { a: 1, b: 2 };
}
({ a, b } = getObject()); // NOT OK

// Array destructuring
[a, b, c] = [10, 20, 30];
[a, b, c] = [];
[[a1, a2], [b1, b2], [c1, c2]] = [[1, 2], [3, 4], [5, 6]];  
[a, b] = [b, a];
[a, b, ...rest] = [10, 20, 30, 40, 50]; // NOT OK
[a, b, [c, d]] = [10, 20, [300, 400], 50];
[a, b, [c, ...rest]] = [10, 20, [30, 40, 50]]; // NOT OK

let tuple: [number, string, number] = [1, '2', 3];
[a, , b] = tuple;
[a, ...rest] = tuple; // NOT OK

const getArray = (): number[] => [1, 2, 3];
[a, b] = getArray();

const set: Set<number> = new Set([1, 2, 3, 4]);
[a, b, c] = set; // NOT OK

const map: Map<number, number> = new Map();
[[a, b], [c, d]] = map; // NOT OK

// Mixed destructuring
let e: number, f: number, x: { e: number }, g;
[a, b, [x, { f }]] = [1, 2, [{ e: 20 }, { f: 5 }]]; // NOT OK
[a, b, {e, e: f, ...g}] = [1, 2, {e: 10}]; // NOT OK
[a, b, ...{length}] = [1, 2, {e: 10}]; // NOT OK

({ a, b : [c, d] } = { a: 1, b: [2, 3] }); // NOT OK
({
  a,
  b: {
    s,
    c: [d, f],
  },
} = { a: 10, b: { s: 'foo', c: [30, 40] } }); // NOT OK

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

({ a, b, s } = new C()); // NOT OK

