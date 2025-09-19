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

let a: number[] = [1]
let b: (number | string)[] = a // error

let a1: (number | string)[] = [1]
let b1: (number | string)[] = a1 // ok

let b2: (number | string)[];
b2 = a // error

class A {
    a: (number | string)[] = [1];
}

let aA: A = new A();
aA.a = a; // error

let a3: (number | string)[] = new Array<number>(1, 2); // error

function test(a: number[]): void {
  let b: (number | string)[] = [1];
  b = a; // error
}

let arrayTypeImmutableA2: [number] = [1];
let arrayTypeImmutableB2: [number | string] = arrayTypeImmutableA2;  // error

class ArrayTypeImmutableA{
  arrayTypeImmutableA: number[] = [1];
  arrayTypeImmutableB: (number | string)[] = this.arrayTypeImmutableA;  // error
  arrayTypeImmutableB1: (number | string)[] = this.arrayTypeImmutableA;  // error

  arrayTypeImmutableA2: [number] = [1];
  arrayTypeImmutableB2: [number | string] = arrayTypeImmutableA2;   // error
  arrayTypeImmutableB21: [number | string] = this.arrayTypeImmutableA2;   // error
}

interface IA {
  ia: string;
}

type TA = string | IA

interface IB {
  i: TA|TA[]
}

class CA {
  static fun(...a: IB[]): void{};
}

CA.fun({
  i: [    { ia: '1'},    { ia: '2'},    { ia: '3'},  ] as IA[]  // error
} as IB)


class A {
  a: number[] = [-11,0];
  arrayData: (number| string| boolean)[] = [1, 'hi']
  arrs: (number| boolean)[] = new A().a //error
  val: (number|string|boolean) [] = new A().arrayData
  A() {
    const val1 = new A().a
    let array2: (string | number | boolean)[] = val1 //error
  }
  aa(ss:(number| boolean)[]) {
    ss = this.a //error
  }
  cc(): (boolean| number)[] {
    return [true, 33];
  }
  dd(): (boolean| string| A)[] {
    return [true, 'hello', new A()];
  }
  ee() {
    let ccVal: (boolean | number | boolean)[] = this.cc()
    return ccVal;
  }

  ff() {
    let array: (number| boolean|string)[] = newArr; //error
    return array;
  }
  gg() {
    return this.arrs;
  }
}

function test2():(string|number|boolean)[] {
  return ['s', 3.14, true];
}
function test3() {
  let obj: A = new A()
  return obj.dd();
}

let objA: A = new A()

const newArr: (number| boolean)[] = [1, true]
const newArr1: (number|string|boolean)[] = [1, '3.14', true]
const array: (number | boolean|string)[] = newArr1
const newArr2: (string|number|boolean)[] = ['s', 3.14, false]
const array1: (number | string | boolean)[] = newArr2
const array2: (string | number | boolean)[] = newArr1

let tt: (boolean | number | boolean)[] = this.test2() //error
let gg: (boolean | number | boolean)[] = new A().ee()
let ff = new A().ff()
let hh: (boolean | number | string)[] =ff
let mm: (boolean | number | boolean)[] = objA.gg();
let test: (boolean | A | string)[] = test3()

let array13: (number|boolean|string)[] = newArr as (number|string)[] //error
let array14: (number|boolean|number)[] = [3.14, true] as (number|boolean)[]
let array15: (boolean|number)[] = array as (number|boolean)[] //error
let array16: (boolean | number | boolean)[] = objA.gg() as (boolean | number)[]
let tuple15: (number|boolean|string)[] = this.test2() as (string|number|boolean)[]
let tuple16: (number|boolean)[]  = array as [number, number, boolean]
let array17: (number|string|boolean)[] = ['s', 3.14, true] as (number|string|boolean)[]
const array18 = Array.from({ length: 5 }, (_, index) => index % 2 === 0 ? index : index % 3 === 0);
let array19: (number|boolean)[] = array18 as (number)[] //error
const originalArray: number[] = [1, 2, 3, 4, 5];
const array20 = originalArray.map((value) => value % 2 === 0 ? true : value * 2);
let array21: [number, boolean] = array20 as [number, boolean]
let array22: (number|string)[] = array20 as (number)[] //error
const array23: (number)[] = [1, 2, 3, 4, 5];

let aaa: number[] = [1]
let bbb: (number | string)[] = aaa  //error
const fn29: Function[] = [];
function bar<T>(): T[] {
  return [];
}
let a: number[] = [];
let repairableArr: Array<number> = new Array();
repairableArr = new Array(3);
Reflect.apply<A, number[], void>(() => {}, objA, []);
if (handler.apply) handler.apply(objA, objA, []);

let readonlyArr: ReadonlyArray<number> = [];
let arr66 = new Array<number>();
readonlyArr = arr66; //error

let stringArray: string[] = []
let correctArr: (string | number)[] = []

const Foo = (): string[] => stringArray;

const FooBar = (): (string | number)[] => stringArray;
const Baz = (): (string | number)[] => correctArr;

async function Foo_a(): Promise<(string| number)[]> {
    return stringArray;
}

async function Foo_b(): Promise<(string| number)[]> {
    return correctArr;
}

AppStorage.SetOrCreate('passwordArray', [] as string[]);
