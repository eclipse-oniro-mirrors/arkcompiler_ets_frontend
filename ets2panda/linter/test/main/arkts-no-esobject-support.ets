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

function test1(): ESObject { // error
  return {} as ESObject; // error
}

let a: ESObject = test1(); // error

function test2(a: ESObject): void {} // error

let b = {} as ESObject; // error

type MyType = ESObject; // error

interface I {
  a: ESObject; // error
}

class C {
  a: ESObject = {}; // error
}

class D {
  getData(): ESObject { // error
    return {} as ESObject; // error
  }
}

class E {
  setData(data: ESObject): void {} // error
}

function test3() {
  let local: ESObject = {} as ESObject; // error
}

let arr: ESObject[] = [ {} as ESObject ]; // error

let tup: [number, ESObject] = [1, {} as ESObject]; // error

type UnionType = string | ESObject; // error
let unionVar: UnionType = {} as ESObject; // error

type IntersectType = { id: number } & ESObject; // error

function generic<T>(val: T): T {
  return val;
}

generic<ESObject>({}); // error

let result = test1() as ESObject; // error

interface Extended extends ESObject {} // error

type FuncType = (val: ESObject) => void; // error

declare class Wrapper {
  value: ESObject; // error
}
