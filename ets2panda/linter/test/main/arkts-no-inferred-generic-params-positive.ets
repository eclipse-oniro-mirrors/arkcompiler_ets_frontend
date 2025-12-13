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

function foo11(arr:Array<string>) {}

class B {}
class C {}
class A <T,U = B>{
  t:T;
  constructor(t:T) {
    this.t = t;
  }
}
new A<C>(new C()) // no err

// builtin
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

// function
function getA<T>(value:T): string {
  return 'res';
}

getA(1) // no err

class B8 {
  static getB<T>(value:T): string {
    return 'res';
  }
}
let b21 = B8.getB('param'); // no err

class C8<T> {
  static getC<T>(value:T): T {
    return value;
  }
}
let c = C8.getC('param'); // no err

class D8 {
  getD<T>(value:T): T {
    return value;
  }
}
let d = new D8().getD('param'); // no err

class AA23<T> {
  constructor(value: T) {}
}

let aa23 = new AA23<number>(42);

class B23<T> extends AA23<T> {
  constructor(value: T) {
    super(value);
  }
}

let b23 = new B23<number>(42) // no err


// extends
class A25<T> {
  getObj(ob: T){
    return ob;
  }
}

class B25 extends A25<string> {

}

let b:B25 = new B25() // no err


function choose<T>(x: T, y: T): T {
  return Math.random() < 0.5 ? x : y
}
let x = choose(10, 20);


function foo<K, N = number>() {
  return 3;
}
foo<string>();
foo<number, string>();

function bar<T>(): T[] {
  return [];
}
bar<string>();

function baz<T>(m: Map<T, string>) {}
baz(new Map<number, string>());

function foobar<T>(t: T) {}
let z: number;
foobar(new Set<number>()); // T is 'Set<any>'

class B17 {}
class C17 {}
class A17 <T,U = B17>{
  t:T;
  constructor(t:T) {
    this.t = t;
  }
}
new A17<C17>(new C17())