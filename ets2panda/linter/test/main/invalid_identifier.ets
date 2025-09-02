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

import { number } from './invalid_identifier_number.js'

import abstract from './invalid_identifier_number.js'

const asVar: string = '';

const internal: string = "internal";

const static: string = "static";

const as: string = "as";

const enum: string = "enum";

const launch: string = "launch";

const switch: string = "switch";

const async: string = "async";

const let: string = "let";

const super: string = "super";

const await: string = "await";

const native: string = "native";

const this: string = "this";

const break: string = "break";

const false: boolean = false;

const new: string = "new";

const throw: string = "throw";

const final: string = "final";

const null: null = null;

const true: boolean = true;

const class: string = "class";

const for: string = "for";

const override: string = "override";

const try: string = "try";

const function: string = "function";

const package: string = "package";

const undefined: undefined = undefined;

const constructor: string = "constructor";

const if: string = "if";

const private: string = "private";

const while: string = "while";

const continue: string = "continue";

const implements: string = "implements";

const protected: string = "protected";

const public: string = "public";

const do: string = "do"

const interface: string = "interface";

const return: string = "return";

const boolean: boolean = true;

const double: number = 3.14;

const Boolean: Boolean = true;

const Double: number = 3.14;

const Number: Number = 42;

const byte: number = 255;

const float: number = 3.14;

const object: object = {};

const Byte: number = 255;

const Float: number = 3.14;

const Object: Object = {};

const bigint: bigint = 123n;

const int: number = 42;

const short: number = 32767;

const Bigint: bigint = 123n;

const Int: number = 42;

const Short: number = 32767;

const char: string = "a";

const string: string = "hello";

const void: void = undefined;

class String {

    private const: string = "const";

    string: string = "hello";

    constructor(string: string) {
    }

    private static Long() {
    }

    public void() {
    }

    test(string: string): string {
        return string;
    }
}

function test1(string: string): string {
    return string;
}

const test2: string = (string: string): string => {
    return string;
};

function long(string: string): string {
    return string;
}

interface Test3 {
    string: string;
}

@Entry
@Component
struct Char {
  @State message: string = 'Hello World';
  abstract:number = 123;
  async:number = 123;
  internal:number = 123;

  build() {
    RelativeContainer() {
      Text(this.message)
        .onClick(() => {
        })
    }
  }
  char(){
  }
  func(){
  }
}

struct Long {
  @State message: string = 'Hello World';

  build() {
    RelativeContainer() {
      Text(this.message)
    }
  }
}

enum char {
  short,
  number = 2,
  int = 3,
}

namespace char {
    let number : number
}

namespace quarantine {
  interface int {
      test: short
  }

  interface test {
      int: int
  }
}
class StringBuilder {
}

class Map {
}

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

let a = 1;

let records: Record<string, string>[] = [];

records.forEach(Map => {})
records.forEach(json => {})
