/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
let f1: Function = () => {}; //  NOT OK
let g1: Function = function() {}; //  NOT OK
let h1: Function = new Function('return 42'); //  NOT OK
let g2 = function() {}; // OK
let h2 = new Function('return 42'); //  NOT OK
let j1 = () => {}; // OK
let f2 = (x: number) => x + 1; // OK


type F<R> = () => R;
type F1<P, R> = (p: P) => R;
type F2<P1, P2, R> = (p1: P1, p2: P2) => R;

let f3: F<void> = () => {}; // OK
let g3: F<number> = () => 42; // OK
let h3: F1<string, number> = (s: string) => s.length; // OK
let i: F2<number, string, boolean> = (n: number, s: string) => s.length > n; // OK

export let add: (a: number, b: number) => number; // OK
export let add1: (a: string) => object; // OK

interface C2 {
  f2: Function  // NOK
}

class A3 {
  func3: Function = () => {} // NOK
}

class A4 extends A3 {
  f4: Function = () => {}  // NOK
}

interface C5 {
  a5;
}

class D5 implements C5 {
  a5: Function = () => {}  // NOK
}

function run6(fn: Function) {
  fn();
}

function getFunction8(): Function {
  return () => {};
}

abstract class F17 {
  abstract f17: Function;
}

class H23 {
  async foo1(): Promise<Function> {
    return Function;
  }
}

class I24 {
  constructor(par: Function, par1?: string) {}
}
class I24_1 extends I24 {
  constructor(par: Function) {
    super(par)
  }
}


class I25 {
  constructor(par: number, par1?: string) {}

  foo3(fn: Function) {
    console.log("this is Arkts1")
  }
}
class I25_1 extends I25 {
  constructor() {
    super(2)
  }

  override foo3(fn: Function): void {
    console.log("this is Arkts2")
  }
}

function foo30(par1: Function, par2: Function[], par3: Function) {
  console.log("ArkTs foo30")
}

function foo31(par1?: Function) {
  console.log("ArkTs foo31")
}

function push32(...items: Function[]) {
  items.forEach(item => {
  });
}

function foo33(par1: number, par2: boolean, par3: string, par4: [string, Array<Function>]) {
  console.log("ArkTs foo33")
}

let array34 = [Function, 1, 2, 3];

class L36 {
  foo1();
  foo1(par:Function, par1:string);
  foo1(par?:Function, par1?:string){
    console.log('arkts36')
  }
}

let tup38:[['arkts'|Function, 1|true, false], string[], Function] = [['arkts', 1, false], ['arkts1', 'arkts2'], tup38_1];

//枚举
enum E39 {
  Function,
  BLUE = 1,
  YELLOW
}

//回调参数不明确
function handleEvent41(callback: Function) {
  callback("event", Date.now());
}

let fn43: Function = (x: number) => x + 1;
function process43(input: number): Function {
  if (typeof input === "string") return fn43;
  if (typeof input === "number") return fn43;
  return fn43;
}

//as
async function fetch(url: string) {
  return new Animal42();
}
const fetchData44: Function = async (url: string) => {
  const res = await fetch(url);
  return res as Function;
};

//高阶函数
function compose45(...fns: Function[]): Function {
  return fns.reduce((prev, curr) => (...args: Function[]) => curr(prev(...args)) as Function);
}

export let add: (a: number, b: number) => number;
export let add1: (a: string) => object;

f1();

class AB3 {
  fn3: Function = () => {};
}
let ab3 = new AB3();
ab3.fn3();

const fn29: Function[] = [];
fn29[1]();

SysApiWrapper.setTimeout = (handler: Function | string, delay?: number): number => {
  (handler as Function)?.();
  return Math.random();
}

new Function('return 42')();

Function('return 42')();

function bar(a1?: Function) {
  a1?.();     //ts like function call error for union type
}

type Function1 = ()=>void

function bar1(a1?: Function1){
  a1?.();
}

type _Function = ()=>void

function bar2(a1?: _Function){
  a1?.();
}