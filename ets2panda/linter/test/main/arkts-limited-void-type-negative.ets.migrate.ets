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

import { unionVoidVal, undefinedFooFun, func3Void, fun4undefined, fun5String } from './ignore_files/arkts-limited-void-type-export'

// Example 1: Basic function
function func1(): void { }
let a: void = func1(); // Error: 'void' used as type annotation / Cannot use type 'void' as value.

// Example 2: Arrow function
const func2 = (): void => { };
let b: void = func2(); // Error: 'void' used as type annotation / Cannot use type 'void' as value.

// Example 3: Class method
class Demo {
  method(): void { }
}
let c: void = new Demo().method(); // Error: 'void' used as type annotation / Cannot use type 'void' as value.

// Example 4: Immediately Invoked Function Expression (IIFE)
let d: void = ((): void => { // Error: 'void' used as type annotation / Cannot use type 'void' as value.
})();

// Example 6: Function parameter
function wrapper(fn: () => void) {
  let f: void = fn(); // Error: 'void' used as type annotation
}

// Example 7: Type assertion
function func3(): void { }
let g: void = func3() as void; // Error: 'void' used as type annotation

// Example 8: Callback function
let h: void = clearInterval(3); // Error: 'void' used as type annotation / Cannot use type 'void' as value.

// Example 9: Array operation
const funcArr: (() => void)[] = [() => { }];
let i: void = funcArr[0](); // Error: 'void' used as type annotation / Cannot use type 'void' as value.

// Example 12: Module export
export function exportedFunc(): void { }
let l: void = exportedFunc(); // Error: 'void' used as type annotation / Cannot use type 'void' as value.

// Example 13: Generic function
function genericFunc<T>(): void { }
let m: void = genericFunc<void>(); // Error: 'void' used as type annotation / Cannot use type 'void' as value.

// Example 14: Function overloading


function overloadFunc(n?: number) { return n; }
let n: void = overloadFunc(); // Error: 'void' used as type annotation / Cannot use type 'void' as value.

// Example 15: Type alias
type VoidFunc = () => void;
const aliasFunc: VoidFunc = () => { };
let o: void = aliasFunc(); // Error: 'void' used as type annotation / Cannot use type 'void' as value.

// Example 16: Interface implementation
interface Task {
  run(): void;
}
class Printer implements Task {
  run(): void { }
}
let p: void = new Printer().run(); // Error: 'void' used as type annotation / Cannot use type 'void' as value.

// Example 17: Optional parameter
function withParam(param?: string): void { }
let q: void = withParam(); // Error: 'void' used as type annotation / Cannot use type 'void' as value.

// Example 18: Rest parameter
function sum(...nums: number[]): void { }
let r: void = sum(1, 2, 3); // Error: 'void' used as type annotation / Cannot use type 'void' as value.


// Example 19: Function currying
const curry = () => (): void => { };
let w: void = curry()();

// Example 20: Recursive call
function recursive(): void {
  return recursive();
}
let ac: void = recursive(); // Error: 'void' used as type annotation / Cannot use type 'void' as value.

// Example 21: Decorator function
function decorator() {
  return (target: object) => { };
}
@decorator()
class Decorated { }
let ad: void = decorator()(Decorated); // Error: 'void' used as type annotation / Cannot use type 'void' as value.


// Example 22: assignment, array usage, parameter usage
function f1(): void { }

let a1 = f1(); // Cannot use type 'void' as value.

a1 = f1(); // Cannot use type 'void' as value.

let a2: void; // type `void` is used as type annotation

let a3: void[] = [f1()]; // Cannot use type 'void' as value.

a3 = [f1()]; // Cannot use type 'void' as value.

a3[0] = f1(); // Cannot use type 'void' as value.

let a4: void = f1(); // type `void` is used as type annotation

function f2(a: void) { } // type `void` is used as type annotation

f2(f1()); // type `void` is used as value

// Example 23: class method, constructor
class A {
  f: void; // type `void` is used as type annotation

  m(p: void) { } // type `void` is used as type annotation

  constructor(a: void) { // type `void` is used as type annotation
    this.f = a;
  }
}

// Example 24: Usage on union Type
function f3(): undefined | Promise<void> { 
  return undefined;
} // type `void` is not allowed in union type

class B {
  m(): undefined | number { 
    return undefined;
  } // type `void` is not allowed in union type
}

// Example 25: Usage void as type alias
type ss = void;
let sspar: ss; // type `void` is used as type annotation

type ff = string;
let ffpar: ff; // type `void` is used as type annotation

// Example 26: Tuple Type
let sread: readonly [void] = [undefined]; // type `void` is used as type annotation

// Example 27: Ternary operator
function foo(): void { }
function bar(): void { }

let test5 = '1';
let bb = test5 === '1' ? foo() : bar(); // Error: Cannot use type 'void' as value.

let dd: void;
dd = test5 === '1' ? foo() : bar(); // Error: Cannot use type 'void' as value.

// Example 28: Void in Union Type (Autofixable : change void to undefined - add/replace expected returns)
function foocfe(a: number): string | undefined { // Error: 'void' used as type annotation. (Autofix: void => undefined)
  if (a >= 0) {
    return "a >= 0";
  }
  return undefined;
}

function foocfe2(a: number): string | undefined { // Error: 'void' used as type annotation. (Autofix: void => undefined)
  if (a < 0) {
    return undefined; //Autofix: replace with return undefined
  }
  return "a >= 0";
}

function greet(hour: number): string | undefined { // Error: 'void' used as type annotation. (Autofix: void => undefined)
  if (hour < 12) {
    return undefined; //Autofix: replace with return undefined
  } else if (hour < 18) {
    return "Good afternoon";
  } else {
    return undefined; //Autofix: replace with return undefined
  }
  return undefined;
}

function getStatus(code: number): string | undefined { // Error: 'void' used as type annotation. (Autofix: void => undefined)
  switch (code) {
    case 1: return "OK";
    case 2: return "Warning";
  }
  return undefined;
  // Autofix: add return statement
}

function tryThing(): string | undefined { // Error: 'void' used as type annotation.  (Autofix: void => undefined)
  try {
    return "Worked!";
  } catch (e) {
    console.error(e);
  }
  return undefined;
  // Autofix: add return statement
}

class A1 {
  aa?: boolean | void

  test(a: number): boolean | undefined { // Error: 'void' used as type annotation.  (Autofix: void => undefined)
    if (a > 0) {
      //Autofix: replace with return undefined
      return undefined;
    }
    //Autofix: replace with return undefined
    return undefined;
  }

  test2(): undefined | boolean { // Error: 'void' used as type annotation.  (Autofix: void => undefined)
    // Autofix: add return statement
  
    return undefined;
  }

}

// Example 29: Void type usage/assignment to union type variable.
// Error: 'void' used as type annotation / Cannot use type 'void' as value.
let ss2: void = foo()
let t: void | number = foo()
let t2: void | number = 1;

// Example 30: Void type usage with 'as' alias
let unionVoid = (() => {
  return undefined;
})() as void;   // Error: 'void' used as type annotation.

function undefinedFoo() {
  return undefined;
}

let asVoidFun = undefinedFoo() as void;   // Error: 'void' used as type annotation.
let undefinedAsVoid = undefined as void;   // Error: 'void' used as type annotation.

function func6() {
  return undefined as void;   // Error: 'void' used as type annotation.
}

// Example 31: Imported things as void
let exportAsVoidFun = unionVoidVal as void; // Error: 'void' used as type annotation.
let exportUndefinedFooFun = undefinedFooFun() as void;   // Error: 'void' used as type annotation.
let exportfunc3Void = func3Void() as void;   // Error: 'void' used as type annotation.
console.log(fun5String() as void); // Error: 'void' used as type annotation.
typeof (fun4undefined() as void)   // Error: 'void' used as type annotation.
