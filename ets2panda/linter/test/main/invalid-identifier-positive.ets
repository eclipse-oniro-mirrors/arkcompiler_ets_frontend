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

import {value3} from './invalid_identifier_number.js'
import value2  from './invalid_identifier_number.js'
import {value1 as importedNumberValue}  from './invalid_identifier_number.js'

const _if = 1;
const _return = 2;
const yieldValue = 3;
const letValue = 4;
const staticCount = 5;
const privateField = 6;
const enumValue = 7;
const _class = 'C';
const asValue = 'test';
const asyncTask = Promise.resolve();

let numberValue = 123;
let message = "ok";

function test1(value: number): number { return value + 1; }

function compute(_private: number, _public: number) {
  return _private + _public;
}

class A {
  private _const: string = "const";
  static count: number = 0;

  constructor(name: string) {}

  asyncMethod() {
    return Promise.resolve(42);
  }

  static callStatic() {
    console.log("called");
  }
}

interface Valid {
  name: string;
  value: number;
}

enum Colors {
  Red,
  Blue,
  Green
}

namespace validSpace {
  export let value = 10;
}

@Entry
@Component
struct Demo {
  @State message: string = 'Test';
  build() {
    Column() {
      Text(this.message);
    }
  }
}

const f1 = (_switch: number): number => _switch + 1;

let records: Record<string, string>[] = [];
records.forEach(item => console.log(item));

import type { Intl } from './test'

export {default as Logger} from './test'
export {byte as default}

class AAA{
  get(){}
  set: ()=>void = ()=>{}
}