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
import {B, C11, Event, EventListener, EventBus} from './arkts-no-inferred-generic-params-ts'
import Session from './arkts-no-inferred-generic-params-ts'

export default class A2 {
  id?: string;
  map?: Map<string, C11>;
}
export class A3 {
  id?: string;
  map?: Map<string, B>;
}

let session: Map<string,Session.A> = new Map<string, Session.A>(); // err

let bb : Map<string,Array<Session.A>> = new Map<string, Array<Session.A>>(); // err

let eventListener : Map<EventListener<Object>,Event.EventEmitter<Object>> = new Map<EventListener<Object>, Event.EventEmitter<Object>>(); // err

let event2 : EventBus<object> = new EventBus<object>(); // err

let event3 : EventBus<Object> = new EventBus<Object>(); // err

let event4 : Array<Object> = new Array<Object>(); // err

let event5 : Array<object> = new Array<object>(); // err

let repairableArr: Array<number> = new Array<number>(); // err
repairableArr = new Array<number>(); // err

const repairableMap: Map<string, number> = new Map<string, number>(); // err

class MyClass<T> {
  public repairableSet: Set<string> = new Set<string>(); // err
  public repairableMap: Map<string, () => string[]> = new Map<string, () => string[]>(); // err
  static repairableStaticMap: Map<string, () => string[]> = new Map<string, () => string[]>(); // err
  constructor() {}
}

const irreparableB = new MyClass<number>();
const repairableC: MyClass<number> = new MyClass<number>(); // err
repairableC.repairableSet = new Set<string>(); // err
repairableC.repairableMap = new Map<string, () => string[]>(); // err
MyClass.repairableStaticMap = new Map<string, () => string[]>(); // err

const promise: Promise<String> = new Promise<String>(() => { return ''; }); // err

function testA(): Map<string, string> {
  return new Map<string, string>(); // err
}

async function testB(): Promise<Map<string, string>> {
  return new Map<string, string>(); // err
}

class MyClassB {
  testA(): Map<string, string> {
    return new Map<string, string>(); // err
  }

  async testB(): Promise<Map<string, string>> {
    return new Map<string, string>(); // err
  }
}

const testMap: Map<string, number> = new Map<string, number>([ // err
  ['123', 1],
  ['sfe', 2]
]);

function foo(arr:Array<string>) {}
foo(new Array()); // err, no autofix

let a12: Array<string> | undefined = new Array<string>(); // err
let aa33: Array<string> | Set<string> | undefined = new Array<string>(); // err
let b32: Array<string> | Array<number> | undefined = new Array(); // err, no autofix
let test: string[] = new Array<string>(); // err

class SubArr extends Array {}
let subArr = new SubArr();
class SubSet extends Set{}
let subSet = new SubSet();
class SubMap extends Map{}
let subMap = new SubMap();
class SubWeakMap extends WeakMap{}
let subWeakMap = new SubWeakMap();
class SubWeakSet extends WeakSet{}
let subWeakSet = new SubWeakSet();

class C {}
class D<T> {}
let a33: D<[C]> = new D<[C]>(); // err

function fun(a?:Map<string,string>){
  let c:Map<string,string>|undefined
  if(a === undefined){
    a = new Map() // err, no autofix
  }
  if(a){
    a = new Map() // err, no autofix
  }
  if(c){
    c = new Map<string, string>() // err
  }
  if(c === undefined){
    c = new Map<string, string>() // err
  }
}

class A23<T> {
  constructor(value: T) {
  }
}

let a23 = new A23(42); // err, no autofix

let aa1: Array<number> = new Array<number>(); // err
let aa2: Array<number> = new Array<number>();

// complex scene
export class StoreCache<T_DECODE, T_ENCODE> {
  private readonly storeManager: StoreManager<T_DECODE, T_ENCODE>;
  constructor(storeManager: StoreManager<T_DECODE, T_ENCODE>) {
    this.storeManager = storeManager;
  }
}

export class MemoryCache<T> extends StoreCache<T, T> {
  constructor(name: string) {
    super(new MemoryStoreManger(name));  // err, no autofix
  }
}

class MemoryStoreManger<T> implements StoreManager<T, T> {
  private readonly _cache: Map<string, number | T> = new Map<string, number | T>();
  private readonly _name: string;

  constructor(name: string) {
    this._name = name;
  }

  get(key: string, def?: number | T | undefined): Promise<number | T | undefined> {
    return Promise.resolve(this._cache.get(key) ?? def);
  }
}

export interface StoreManager<T_DECODE, T_ENCODE> {
  get(key: string, def?: T_ENCODE | number): Promise<T_ENCODE | number | undefined>;
}

// constructor
class A41<T> {
  constructor(c: C41<T>) {
  }
}

class B41<T> extends A41<T> {
  constructor(value: string) {
    super(new C41(value)) // err, no autofix
  }
}

class C41<T> {
  constructor(val: string) {}
}

class A24<T> {
  constructor(c: C24<T>) {
  }
}

class B24 extends A24<string> {
}

class C24<T> {
  constructor(val: string) {}
  getObj(ob: T){
    return ob;
  }
}

let b44:B24 = new B24(new C24('a')) //new C error, no autofix

// Union
let a: Array<string> | undefined = new Array<string>(); // err, autofix
let aa: Array<string> | Set<string> | undefined = new Array<string>(); // err, autofix
let abc: string[] = new Array<string>(); // err, autofix
let b: Array<string> | Array<number> | undefined = new Array(); // err, no autofix

let abc2: string[][] = new Array<string[]>(); // err

let abc3: string[][] = new Array<string[]>(new Array); // err

let abc4: string[][] = new Array<string[]>(new Array()); // err

let  test_weakMap1:WeakMap<object, string> = new WeakMap<object, string>(); // err, autofix
let  test_weakSet1:WeakSet<object> = new WeakSet<object>(); // err, autofix

// nest
let caseArray1: Array<Array<Array<number>>> = new Array<Array<Array<number>>>(new Array(new Array())) // report all, only autofix outer

// in function or method
class A63 {
  test(key: string): Promise<string> {
    if (key == undefined || key.length == 0.0) {
      return new Promise<string>((reject) => { // err, autofix
        reject("empty key");});
    } else {
      return new Promise<string>((resolve) => { // err, autofix
        resolve("not empty key");});
    }
  }
}

function test1(key: string): Promise<string> {
  if (key == undefined || key.length == 0.0) {
    return new Promise<string>((reject) => { // err, autofix
      reject("empty key");});
  } else {
    return new Promise<string>((resolve) => { // err, autofix
      resolve("not empty key");});
  }
}

function processNumbers(nums: number[]): Array<string> {
  const isEven: (n: number)=> Array<number> = (n: number): Array<number> =>{
   if (n % 2 === 0){
     return new Array<number>() // err, autofix to number
   }
    return new Array<number>() // err, autofix to number
  }

  const isEven2: (n: number)=> Array<number> = (n: number) =>{
    if (n % 2 === 0){
      return new Array() // err, no autofix
    }
    return new Array() // err, no autofix
  }

  const isEven3 = (n: number) : Array<number> =>{
    if (n % 2 === 0){
      return new Array<number>() // err, autofix to number
    }
    return new Array<number>() // err, autofix to number
  }

  const isEven4 = (n: number) =>{
    if (n % 2 === 0){
      return new Array() // err, no autofix
    }
    return new Array() // err, no autofix
  }
  return new Array<string>() // err, autofix to string
}

function a(): Array<string>{
  class Person {
    constructor(name: string) {
      return new Array(); // err, no autofix
    }
  }
  return new Array<string>(); // err, autofix
}
