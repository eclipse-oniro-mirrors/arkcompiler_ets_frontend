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

class ArrayCls {
  static create(size: number): number[] { return []; }
}

namespace NumericNs {
  export const N: number = 10;
  export const M: number = 20;
}

function typeAnnotationTest(): void {
  const b: byte = 127;
  const arr1: number[] = new Array(b); // arkts-numeric-semantic - byte type

  const s: short = 32767;
  const arr2: number[] = new Array(s); // arkts-numeric-semantic - short type

  const l: long = 100;
  const arr3: number[] = new Array(l); // arkts-numeric-semantic - long type

  const arr4: number[] = new Array(NumericNs.N); // arkts-numeric-semantic - namespace field
  const arr5: number[] = new Array(NumericNs.M); // arkts-numeric-semantic - namespace field
}

function unionTypeTest(): void {
  const a: number = 5; // arkts-numeric-semantic
  const arr: number[] = new Array(a); // arkts-numeric-semantic

  // readonly array - length access
  const roArr: readonly number[] = [1, 2, 3];
  const len: number = roArr.length; // arkts-numeric-semantic
  const arr2: number[] = new Array(len); // arkts-numeric-semantic
}

function unknownCastTest(): void {
  const x: number = 3; // arkts-numeric-semantic
  const arr: number[] = new Array(x); // arkts-numeric-semantic
}
