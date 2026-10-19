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

function iteratorChainTest(): void {
  const arr: number[] = [1, 2, 3];
  const iter = arr[Symbol.iterator]();
  const result = iter.next();
  const val: number = result.value; // arkts-numeric-semantic - iterator .value chain

  // tuple-like access from builtin
  const entries = arr.entries();
  const entry = entries.next();
  const entryVal: number = entry.value[1]; // arkts-numeric-semantic - tuple from iterator

  // array element from builtin return
  const newArr: number[] = Array.from([1, 2, 3]);
  const elem: number = newArr[0]; // arkts-numeric-semantic - array element from builtin
}

function parseIntChainTest(): void {
  // parseInt return used as array index
  const idx: number = parseInt('42'); // arkts-numeric-semantic - parseInt
  const arr: number[] = [1, 2, 3, 4, 5];
  const val2: number = arr[idx]; // arkts-numeric-semantic - builtin return as array index

  // Math.floor return used as array size
  const size: number = Math.floor(3.7); // arkts-numeric-semantic - Math.floor
  const arr2: number[] = new Array(size); // arkts-numeric-semantic - builtin as Array constructor arg
}

function constructorChainTest(): void {
  // Array constructor with number from builtin
  const len: number = '5'.length; // arkts-numeric-semantic - string length (builtin field)
  const arr: number[] = new Array(len); // arkts-numeric-semantic - builtin field as Array size

  // Number constructor
  const n: number = Number('123'); // arkts-numeric-semantic - Number constructor
  const arr3: number[] = new Array(n); // arkts-numeric-semantic - constructor return as Array size
}

function conflictingCategoryTest(a: number, b: number): void {
  // Two different builtin APIs producing conflicting int/long categories
  const intVal: number = parseInt('42'); // arkts-numeric-semantic - int category
  const longVal: number = Math.floor(3.7); // arkts-numeric-semantic - could be different category

  // Same arg used with both int and long builtins
  const arr: number[] = new Array(a); // arkts-numeric-semantic - arg used as array size
  const arr2: number[] = new Array(b); // arkts-numeric-semantic - same pattern different var
}

function numberConstantBranchTest(): void {
  // Float actually int
  const x: number = 1.0; // arkts-numeric-semantic - float actually int
  const arr: number[] = new Array(x);

  // Non-integer literal
  const y: number = 3.14; // arkts-numeric-semantic - non-integer
  const arr2: number[] = new Array(y);

  // Binary literal
  const b: number = 0b101; // arkts-numeric-semantic - binary literal
  const arr3: number[] = new Array(b);

  // Hex literal
  const h: number = 0xFF; // arkts-numeric-semantic - hex literal
  const arr4: number[] = new Array(h);
}

enum Color { Red = 1, Green = 2, Blue = 3 }

function iteratorEnumTest(): void {
  const arr = [1, 2, 3];
  const iter = arr[Symbol.iterator]();
  const result = iter.next();
  const x: number = result.value; // arkts-numeric-semantic - iterator result value

  const y: number = arr[0]; // arkts-numeric-semantic - array ref assign

  const z: number = Color.Red / 2; // arkts-numeric-semantic - enum value in division
}
