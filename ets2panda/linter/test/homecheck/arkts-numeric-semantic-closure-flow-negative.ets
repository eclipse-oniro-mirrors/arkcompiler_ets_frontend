/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

function closureTest(): void {
  let captured: number = 10;
  const fn = () => { captured = 20; };
  fn();
  const arr: number[] = new Array(captured); // arkts-numeric-semantic - closure reassignment
}

function ifConditionTest(a: number): void {
  if (a > 0) { // arkts-numeric-semantic - local in if condition
    console.log(a.toString());
  }
  const arr: number[] = new Array(a);
}

async function asyncOuterTest(): Promise<void> {
  async function inner(): Promise<number> {
    return 42;
  }
  const val = await inner();
  const arr: number[] = new Array(val); // arkts-numeric-semantic - async outer method
}

function closureReassignTest2(): void {
  let captured: number = 10; // arkts-numeric-semantic
  const fn = () => { captured = 20; };
  fn();
  const arr: number[] = new Array(captured); // arkts-numeric-semantic
}

function ifConditionTest2(a: number): void {
  if (a > 0) {
    console.log(a.toString());
  }
  const arr: number[] = new Array(a); // arkts-numeric-semantic
}

function binaryOpTest2(): void {
  const x: number = 10; // arkts-numeric-semantic
  const y: number = x + 5; // arkts-numeric-semantic - binary op result
  const arr: number[] = new Array(y);

  const z: number = x * 2; // arkts-numeric-semantic
  const arr2: number[] = new Array(z);
}

function castTypeofTest2(): void {
  const x: number = 5; // arkts-numeric-semantic
  const y = x as number; // arkts-numeric-semantic - cast expression
  const arr: number[] = new Array(y);

  if (typeof x === 'number') {
    const arr2: number[] = new Array(x); // arkts-numeric-semantic
  }
}

function deepCallTest(a: number): void {
  g1(a);
}
function g1(a: number): void { g2(a); }
function g2(a: number): void { g3(a); }
function g3(a: number): void {
  const arr: number[] = new Array(a); // arkts-numeric-semantic
}

function promiseThenTest2(): void {
  const v: number = 10; // arkts-numeric-semantic
  Promise.resolve(v).then((val: number) => {
    const arr: number[] = new Array(val); // arkts-numeric-semantic - Promise.then arrow
  });
}

function enumDivisionTest2(): void {
  enum Color { Red = 1, Green = 2 }
  const e: number = Color.Red; // arkts-numeric-semantic
  const result: number = e / 2; // arkts-numeric-semantic - enum in division
  const arr: number[] = new Array(result);
}

function promiseThenTest3(): void {
  Promise.resolve(10).then((v: number) => {
    const arr: number[] = new Array(v); // arkts-numeric-semantic - Promise.then arrow
  });
}

function deepCallChainTest(a: number): void {
  // 3+ level deep call chain triggers call depth limit
  h1(a);
}

function h1(a: number): void {
  h2(a);
}

function h2(a: number): void {
  h3(a);
}

function h3(a: number): void {
  const arr: number[] = new Array(a); // arkts-numeric-semantic - call depth limit
}

function tupleCastTest(): void {
  const t: [number, number] = [1, 2];
  t[0] = 2 / 3; // arkts-numeric-semantic - tuple assignment

  const x: number = 1;
  const y = x as number; // arkts-numeric-semantic - cast expression
  const z = typeof x === 'number' ? 1 : 0; // arkts-numeric-semantic - typeof expression
}
