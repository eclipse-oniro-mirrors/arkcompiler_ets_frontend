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

let n1: 1 = 1;
let n2: 0.1 = 0.5;

let f1: true = true;
let f2: null;
let f3: null = null;
let f4: undefined = undefined;
let f5: 'undefined';
let f6: 's' = 333;
let f7: 'y' = 'ssss';
let f8: aaa = 'ssss';

function getOne(): 1 {
  return 1;
}

function isAvaliable(): true {
  return true;
}

function setFlag(flag: true) {
  console.log(flag);
}

function setPrecision(precision: 0.1) {
  console.log(precision);
}

interface Config {
  readonly enable: true;
  readonly threshold: 100;
}

class A {
  fd: 'yyy' = 111;
  n1: 1 = 6;
  rg: undefined;
  ytt: 'y' = 'ssss';
  foo: aaa = 'ssss';
}

let x: 10;
let persons: Record<string, 'OptionalPerson'>;

let s = '123';
let c1 = s.length as 1;

let arr: 1[];

let b: [1] = [1];

let a: boolean | 1 = 1;

let tup: [['arkts' | boolean, 1 | true, false], string[], number] = [['arkts', 1, false], ['arkts1', 'arkts2'], 1.5];
