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

import { number } from './invalid_identifier_number.js' // invalid import keyword
import abstract from './invalid_identifier_number.js' // invalid import keyword
import {value1 as interface} from './invalid_identifier_number.js' // invalid import keyword

// invalid keyword declarations
const as: string = '';
const enum: string = '';
const static: string = '';
const async: string = '';
const let: string = '';
const super: string = '';
const await: string = '';
const this: string = '';
const break: string = '';
const class: string = '';
const function: string = '';
const package: string = '';
const private: string = '';
const public: string = '';
const protected: string = '';
const return: string = '';
const while: string = '';
const do: string = '';
const for: string = '';
const switch: string = '';
const throw: string = '';
const try: string = '';
const if: string = '';
const else: string = '';
const null: null = null;
const true: boolean = true;
const false: boolean = false;

class A {
  private const: string = "const"; // invalid: const as property name
  constructor(static: string) {} // invalid parameter name
  get async() { return 1; } // invalid accessor name
  set async(v: number) {} // invalid accessor name
  method(let: number) { return let + 1; } // invalid param
}

function test1(class: string): void { } // invalid param name

const test2: string = (string: string): string => { // invalid param name
    return string;
};

function long(string: string): string { // invalid function name
    return string;
}

interface char {
  private: string; // invalid prop name
  function: string; // invalid prop name
}

enum C {
  as = 1, // invalid enum member
  static = 2
}

namespace launch {
  export let await = 1; // invalid in module scope
}

@Entry
@Component
struct protected { // invalid struct name.
  @State message: string = 'Hello World';
  abstract: number = 123; // invalid property name.
  async: number = 123; // invalid property name.
  internal: number = 123; // invalid property name.

  build() {
    RelativeContainer() {
      Text(this.message)
        .onClick(() => {
        })
    }
  }
  char() { // invalid method name
  }
  func() {
  }
}

struct object { //  invalid struct name.
  @State message: string = 'Hello World';

  build() {
    RelativeContainer() {
      Text(this.message)
    }
  }
}

namespace quarantine {
  interface int { //  invalid interface name.
      test: short
  }

  class SyncPrimitives {
    public int: int = 0 //  invalid property name.
  }
}

// Reserved identifiers used in exports
let _if = null;
let _import = null;
let _export = null;
let _await = null;
let _arguments = null;
let _eval = null;
let _default = null;
let _as = null;

export {
  _if as if,
  _import as import,
  _export as export,
  _await as await,
  _arguments as arguments,
  _eval as eval,
  _default as default,
  _as as as
};

let taskpool: Record<string, string>[] = []; // 'taskpool' is an invalid keyword

taskpool.forEach(Map => {}) // 'map' is an invalid keyword
taskpool.forEach(json => {}) // 'json' is an invalid keyword
