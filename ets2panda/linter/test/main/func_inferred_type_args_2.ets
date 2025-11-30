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
import { AttributeUpdater } from '@kit.ArkUI';

const irreparableArr = new Array();
let repairableArr: Array<number> = new Array();
repairableArr = new Array();

const irreparableMap = new Map();
const repairableMap: Map<string, number> = new Map();
repairableMap = new Map();

class MyClass<T> {
  public irreparableMap = new Map();
  public repairableSet: Set<string> = new Map();
  public repairableMap: Map<string, () => string[]> = new Map();
  static repairableStaticMap: Map<string, () => string[]> = new Map();
  constructor() {}
}

const irreparableA = new MyClass();
const irreparableB = new MyClass<number>();
const repairableC: MyClass<number> = new MyClass();
repairableC.irreparableMap = new Map();
repairableC.repairableSet = new Set();
repairableC.repairableMap = new Map();
MyClass.repairableStaticMap = new Map();

const promise: Promise<String> = new Promise(() => { return ''; });

function testA(): Map<string, string> {
  return new Map();
}

async function testB(): Promise<Map<string, string>> {
  return new Map();
}

function testC(): Map<string, string> {
  return new Set();
}

async function testD(): Promise<Map<string, string>> {
  return new Set();
}

class MyClassB {
  testA(): Map<string, string> {
    return new Map();
  }

  async testB(): Promise<Map<string, string>> {
    return new Map();
  }

  testC(): Map<string, string> {
    return new Set();
  }

  async testD(): Promise<Map<string, string>> {
    return new Set();
  }
}

const testMap: Map<string, number> = new Map([
  ['123', 1], // my comment 1
  ['sfe', 2] // my comment 2
]);
let a : Array<string> = new Array();
function foo(arr:Array<string>) { }
foo(new Array()); 
@Observed
export class AppIconCloneBadgeVm {
  @Track public cloneBadgeModifier: AttributeUpdater<CommonAttribute> = new AttributeUpdater<CommonAttribute>();
}
class B {}
class C {}
class A <T,U = B>{
  t:T;
  constructor(t:T) {
    this.t = t;
  }
}
new A<C>(new C())
let a: Array<string> | undefined = new Array();
let aa: Array<string> | Set<string> | undefined = new Array();
let b: Array<string> | Array<number> | undefined = new Array();
let test: string[] = new Array();

class A {
  test(key: string): Promise<string> {
    if (key == undefined || key.length == 0.0) {
      return new Promise((resolve, reject) => {
        reject("empty key");});
    } else {
    }
  }
}
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
let a: D<[C]> = new D();

function fun(a?:Map<string,string>){
  let c:Map<string,string>|undefined
  if(a === undefined){
    a = new Map()
  }
  if(a){
    a = new Map()
  }
  if(c){
    c = new Map()
  }
  if(c === undefined){
    c = new Map()
  }
}