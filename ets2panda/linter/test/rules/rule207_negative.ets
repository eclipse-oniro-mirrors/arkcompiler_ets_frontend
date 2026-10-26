/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

function foo_test_1() {
    console.log(arguments[0]);
}

function foo_test_2() {
    console.log(arguments[1]);
}

function foo_test_3() {
    for (let i = 0; i < arguments.length; i++) {
        console.log(arguments[i]);
    }
}

function foo_test_4() {
    console.log('Number of arguments:', arguments.length);
}

function foo_test_5() {
    console.log(arguments);
}

function foo_test_6() {
    let args = Array.prototype.slice.call(arguments);
    console.log(args);
}

function foo_test_7 {
    console.log(Array.prototype.join.call(arguments, ', '));
}

function foo_test_8 {
    console.log(arguments);
}

function foo_test_9 {
    let args = Array.prototype.slice.call(arguments);
    console.log(args);
}

function foo_test_10() {
    if (arguments.length > 0) {
        console.log('Parameters received:', arguments);
    } else {
        console.log('No parameters received.');
    }
}

class C {
  arguments: IArguments; // No error

  constructor() {
    this.arguments = arguments; // Error on right-hand side `arguments`
  }

  m() {
    console.log(arguments); // Error
  }
}

function foo_test_11() {
  let c = new C();
  c.m();
}

// Test: arguments in arrow function - Error
const arrowFn = () => {
  console.log(arguments[0]); // Error
};

// Test: arguments in function expression - Error
const funcExpr = function() {
  console.log(arguments.length); // Error
};

// Test: arguments in async function - Error
async function asyncFn() {
  console.log(arguments); // Error
}

// Test: arguments in generator function - Error
function* genFn() {
  console.log(arguments[0]); // Error
}

// Test: arguments in nested function - Error
function outerFunc() {
  function innerFunc() {
    console.log(arguments); // Error
  }
  innerFunc();
}

// Test: arguments used in property access chain - Error
function chainFunc() {
  const val = arguments[0].toString(); // Error
  return val;
}

// Test: arguments in class static method - Error
class StaticMethod {
  static sm() {
    console.log(arguments); // Error
  }
}

// Test: arguments in getter - Error
class GetterClass {
  _val: number = 0;
  get val() {
    console.log(arguments); // Error
    return this._val;
  }
}

// Test: arguments in setter - Error
class SetterClass {
  _val: number = 0;
  set val(v: number) {
    console.log(arguments); // Error
    this._val = v;
  }
}
