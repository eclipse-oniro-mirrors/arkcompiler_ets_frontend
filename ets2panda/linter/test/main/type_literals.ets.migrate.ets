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

interface Type1 {
}
interface Type2 {
    a: number;
    b: string;
}
interface GeneratedTypeLiteralInterface_1 {
    a: number;
    b: number;
}
type Type3 = GeneratedTypeLiteralInterface_1[];
interface GeneratedTypeLiteralInterface_2 {
    a: number;
    b: string;
}
type Type4 = Type1 | GeneratedTypeLiteralInterface_2 | {c: number, d: string};
interface Type5<T, K> {
    a: T;
    b: K;
    c: number;
}
interface Type6 {
    a: number;
    b: string;
    c: {
        x: number;
        y: string;
        z: Type1;
    };
}
export interface Type7 {
    a: number;
}

interface GeneratedTypeLiteralInterface_5 {
}
let var1: GeneratedTypeLiteralInterface_5 = {};
interface GeneratedTypeLiteralInterface_6 {
    a: number;
    b: string;
}
let var2: GeneratedTypeLiteralInterface_6 = {a:1, b:'2'};
interface GeneratedTypeLiteralInterface_7 {
    a: number;
    b: number;
}
let var3: GeneratedTypeLiteralInterface_7[];
interface GeneratedTypeLiteralInterface_8 {
    a: number;
    b: string;
}
let var4: Type1 | GeneratedTypeLiteralInterface_8 | {c: number, d: string};
interface GeneratedTypeLiteralInterface_10 {
    a: number;
    b: string;
}
let var5 = var1 as GeneratedTypeLiteralInterface_10;
interface GeneratedTypeLiteralInterface_11 {
    a: number;
    b: string;
    c: {
        x: number;
        y: string;
        z: Type2;
    };
}
let var6: GeneratedTypeLiteralInterface_11 = {
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

interface GeneratedTypeLiteralInterface_13 {
    x: number;
    y: string;
}
function f(p: GeneratedTypeLiteralInterface_13): {z: boolean} {
  return {z: true};
}

interface GeneratedTypeLiteralInterface_15 {
    a: number;
    b: string;
}
class C {
  f: GeneratedTypeLiteralInterface_15 = {a: 1, b:'2'};

  m(p: {c: number, d: string}): {e: boolean} {
    return {e: true};
  }
}

// Generic types
class GC<T> {}
interface GeneratedTypeLiteralInterface_18 {
    x: 10;
}
class GC2 extends GC<GeneratedTypeLiteralInterface_18> {}
interface GeneratedTypeLiteralInterface_19 {
    y: string;
}
const gc: GC<GeneratedTypeLiteralInterface_19> = new GC<{ y: '20' }>();

interface GeneratedTypeLiteralInterface_21 {
    z: number;
}
type GType<K = GeneratedTypeLiteralInterface_21> = GC<K>;

function g<T>(t: T): number {
  return 10;
}
interface GeneratedTypeLiteralInterface_22 {
    z: boolean;
}
g<GeneratedTypeLiteralInterface_22>({ z: true });

interface TypeMembers {
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

interface GeneratedTypeLiteralInterface_23 {
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
let typeMembers: GeneratedTypeLiteralInterface_23;

// Capture type from enclosing local scope
export function captureFromLocalScope<T>(t: T): {ret: T} { // Non-fixable, type captures type parameter from enclosing declaration
  let v1: {local: T}; // Non-fixable, type captures type parameter from enclosing declaration

  interface LocalType {
    a: number;
    b: string;
}
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