/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

let n1: int = 1;
let n2: number = 0.5;

let f1: boolean = true;
let f3 = null;
let f4 = undefined;
let f5: string;
let f6: int = 333;
let f7: string = 'ssss';

function getOne(): int {
  return 1;
}

function isAvaliable(): boolean {
  return true;
}

function setFlag(flag: boolean) {
  console.log(flag);
}

function setPrecision(precision: number) {
  console.log(precision);
}

interface Config {
  readonly enable: boolean;
  readonly threshold: int;
}

class A {
  fd: int = 111;
  n1: int = 6;
  rg: boolean = false;
  ytt: string = 'ssss';
  foo: string = 'ssss';
}

let x: number;
let persons: Record<string, OptionalPerson>;

let s = '123';
let c1 = s.length as number;

let arr: number[];

let b: [number] = [1];

let a: boolean | number = 1;

let tup: [[string | boolean, number | boolean, boolean], string[], number] = [
  ['arkts', 1, false],
  ['arkts1', 'arkts2'],
  1.5
];
