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

import {unionVoidVal,undefinedFooFun,func3Void,fun4undefined,fun5String} from './limit_void_type2'
// Example 1: Basic function
function func1(): void { }
let a: void = func1();
// Example 2: Arrow function
const func2 = (): void => { };
let b: void = func2();
// Example 3: Class method
class Demo {
  method(): void { }
}
let c: void = new Demo().method();
// Example 4: Immediately Invoked Function Expression (IIFE)
let d: void = (function (): void { })();
// Example 5: Asynchronous function
async function asyncFunc(): Promise<void> { }
let e: void = await asyncFunc();
// Example 6: Function parameter
function wrapper(fn: () => void) {
  let f: void = fn();
}
// Example 7: Type assertion
function func3(): void { }
let g: void = func3() as void;
// Example 8: Callback function
setTimeout((): void => { }, 1000);
let h: void = setTimeout(() => { }, 1000);
// Example 9: Array operation
const funcArr: (() => void)[] = [() => { }];
let i: void = funcArr[0]();
// Example 10: Object method
const obj = {
  action: (): void => { }
};
let j: void = obj.action();
// Example 11: Strict mode
// @ts-strict
function func4(): void { }
let k: void = func4();
// Example 12: Module export
export function exportedFunc(): void { }
let l: void = exportedFunc();
// Example 13: Generic function
function genericFunc<T>(): void { }
let m: void = genericFunc();
// Example 14: Function overloading
function overloadFunc(): void;
function overloadFunc(n: number): number;
function overloadFunc(n?: number) { return n; }
let n: void = overloadFunc();
// Example 15: Type alias
type VoidFunc = () => void;
const aliasFunc: VoidFunc = () => { };
let o: void = aliasFunc();
// Example 16: Interface implementation
interface Task {
  run(): void;
}
class Printer implements Task {
  run(): void { }
}
let p: void = new Printer().run();
// Example 17: Optional parameter
function withParam(param?: string): void { }
let q: void = withParam();
// Example 18: Rest parameter
function sum(...nums: number[]): void { }
let r: void = sum(1, 2, 3);
// Example 19: This parameter
function withThis(this: Window): void { }
let s: void = withThis.call(window);
// Example 20: Generator function
function* genFunc(): Generator<void> {
  yield;
}
let u: void = genFunc().next().value;
// Example 21: Function currying
const curry = () => (): void => { };
let w: void = curry()();
// Example 22: Method chaining
class Chain {
  first(): this { return this; }
  last(): void { }
}
let x: void = new Chain().first().last();
// Example 23: Destructuring assignment
const [func] = [(): void => { }];
let y: void = func();
// Example 24: Type mapping
type Wrapper<T> = { value: T };
const wrapped: Wrapper<() => void> = { value: () => { } };
let z: void = wrapped.value();
// Example 25: Conditional type
type Conditional<T> = T extends boolean ? () => void : never;
const condFunc: Conditional<boolean> = () => { };
let aa: void = condFunc();
// Example 26: Mixed type
interface Mixed {
  (): void;
  prop: string;
}
const mixed: Mixed = Object.assign(() => { }, { prop: "" });
let ab: void = mixed();
// Example 27: Recursive call
function recursive(): void {
  return recursive();
}
let ac: void = recursive();
// Example 28: Decorator function
function decorator() {
  return function (target: any) { };
}
@decorator()
class Decorated { }
let ad: void = decorator()(Decorated);

function f1(): void { }

let a1 = f1(); // type `void` is used as value

a1 = f1(); // type `void` is used as value

let a2: void; // type `void` is used as type annotation

let a3: void[] = [f1()]; // type `void` is used as type annotation

a3 = [f1()]; // type `void` is used as value

a3[0] = f1(); // type `void` is used as value

let a4: void = f1(); // type `void` is used as type annotation

function f2(a: void) { } // type `void` is used as type annotation

f2(f1()); // type `void` is used as value

class A {
  f: void; // type `void` is used as type annotation

  m(p: void) { } // type `void` is used as type annotation

  constructor(a: void) { // type `void` is used as type annotation
    this.f = a;
  }
}

function f3(): void | Promise<void> { } // type `void` is not allowed in union type

class B {
  m(): void | number { } // type `void` is not allowed in union type
}

type ss = void;
let sspar: ss;

type ff = string;
let ffpar: ff;

let sread: readonly [void] = [undefined];

interface BT {
  qaq: Function;
}

class C {
  private cc?: BT;

  private d(): void {
    this.cc = {
      qaq: (caller?: string): void => this.qaqq(caller)
    }
  }
  private qaqq(caller?: string): void {
    return;
  }
}

function foo(): void { }
function bar(): void { }

let aa = '1';
let bb = aa === '1' ? foo() : bar(); // Error

aa === '1' ? foo() : bar(); // No error
let dd;
dd = aa === '1' ? foo() : bar(); // Error
interface testB {
  u: void;  // Error
  fooIf(): void;
}

function foo1():void{
    return foo(); // No Error
}

function foocfe(a: number): string | void {
  if (a >= 0) {
    return "a >= 0";
  }
}

function foocfe2(a: number): string | void {
  if (a < 0) {
    return;
  }
  return "a >= 0";
}
function fooefc(): void { }
let ss: void = foo()
let t: void | number = foo()
let t2: void | number = 1;

function greet(hour: number): string | void {
  if (hour < 12) {
    return;
  } else if (hour < 18) {
    return "Good afternoon";
  } else {
    return;
  }
}

function logOrReturn(flag: boolean): string | void {
  if (flag) {
    return "Flag is true";
  }
  console.log("Flag is false");
  return;
}

function justLogs(): string | void {
  console.log("Hello!");
}

function getStatus(code: number): string | void {
  switch (code) {
    case 1: return "OK";
    case 2: return "Warning";
  }
}

function tryThing(): string | void {
  try {
    return "Worked!";
  } catch (e) {
    console.error(e);
  }
}

class A1 {
  // test
  aa?: boolean | void

  test(a: number): boolean | void {
    if (a > 0) {
      // will return
      return;
    }
    // will return
    return;
  }

  test1(a: number): void | boolean {
    if (a > 0) {
      // will return
      return true;
    }
    // will return
    return;
  }

  test2(a: number): void | boolean {
    if (a > 0) {
      // will return
      return;
    }
  }

  test3(a: number): void | boolean {
    if (a > 0) {
      // will return
      return;
    }
    return false;
  }

  test4(): void | boolean {
    // will return
  }
}

function test(a: number): boolean | void {
  if (a > 0) {
    // will return
    return;
  }
  // will return
  return;
}

function test1(a: number): void | boolean {
  if (a > 0) {
    // will return
    return true;
  }
  // will return
  return;
}

function test2(a: number): void | boolean {
  if (a > 0) {
    // will return
    return;
  }
}

function test3(a: number): void | boolean {
  if (a > 0) {
    // will return
    return;
  }
  return false;
}

function test4(a: number): void | boolean {
  // will return
}

let unionVoid = (()=>{
  return undefined;
})() as void;   //error
function undefinedFoo(){
  return undefined;
}
function unionFoo(): undefined | boolean{
  return undefined;
}
let asVoidFun = undefinedFoo() as void;   //error
let undefinedAsVoid = undefined as void;   //error
let unionFooAsVoid = unionFoo() as void;  //error

function func6() {
  return undefined as void;   //error
}

let exportAsVoidFun = unionVoidVal as void;
let exportUndefinedFooFun = undefinedFooFun() as void;   //error
let exportfunc3Void = func3Void() as void;   //error
console.log(fun5String() as void);
typeof (fun4undefined() as void)   //error
