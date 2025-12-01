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

type Type1 = {};
type Type2 = {a: number; b: string;};
type Type3 = {a: number, b: number,}[];
type Type4 = Type1 | {a: number, b: string} | {c: number, d: string};
type Type5<T, K> = {a: T, b: K, c: number};
type Type6 = {
  a: number
  b: string
  c: {
    x: number
    y: string
    z: Type1
  }
};
export type Type7 = {a: number};

let var1: {} = {};
let var2: {a: number, b: string} = {a:1, b:'2'};
let var3: {a: number, b: number,}[];
let var4: Type1 | {a: number, b: string} | {c: number, d: string};
let var5 = var1 as {a: number, b: string};
let var6: {
  a: number
  b: string
  c: {
    x: number
    y: string
    z: Type2
  }
} = {
  a: 1,
  b: '2',
  c: {
    x: 3,
    y: '4',
    z: {
      a:5,
      b:'6'
    }
  }
};

function f(p: {x: number, y: string}): {z: boolean} {
  return {z: true};
}

class C {
  f: {a: number, b: string} = {a: 1, b:'2'};

  m(p: {c: number, d: string}): {e: boolean} {
    return {e: true};
  }
}

// Generic types
class GC<T> {}
class GC2 extends GC<{ x: 10 }> {}
const gc: GC<{ y: string }> = new GC<{ y: '20' }>();

type GType<K = {z: number}> = GC<K>;

function g<T>(t: T): number {
  return 10;
}
g<{ z: boolean }>({ z: true });

type TypeMembers = {
  a: number;
  'b': string;
  3: boolean;
  ['x']: number;

  m: (p: string) => number;
  n(p: number): void;
  get val(): number;
  set val(p: number);

  (p: number): string;
  new (p: string): C;
  [key: string]: unknown;
}

let typeMembers: {
  a: number;
  'b': string;
  3: boolean;
  ['x']: number;

  m: (p: string) => number;
  n(p: number): void;
  get val(): number;
  set val(p: number);

  (p: number): string;
  new (p: string): C;
  [key: string]: unknown;
};

// Capture type from enclosing local scope
export function captureFromLocalScope<T>(t: T): {ret: T} { // Non-fixable, type captures type parameter from enclosing declaration
  let v1: {local: T}; // Non-fixable, type captures type parameter from enclosing declaration

  type LocalType = {a: number, b: string};
  let v2: { x: LocalType } = {x: {a: 1, b: '2'}}; // Non-fixable, type captures another type declared in local scope

  type LocalType2<K> = ({k: K}); // Non-fixable, type captures type parameter from enclosing declaration

  const one = 1;
  let v3: {[one]: 1}; // Non-fixable, type captures another object declared in local scope

  class LocalClass {x: number = 1};
  let v4: { y: LocalClass } = {y: new LocalClass()}; // Non-fixable, type captures another type declared in local scope

  let v5: { lc: typeof LocalClass }; // Non-fixable, type captures another type declared in local scope

  let v6: { lc: typeof v1 }; // Non-fixable, type captures another object declared in local scope

  return {ret: t};
}