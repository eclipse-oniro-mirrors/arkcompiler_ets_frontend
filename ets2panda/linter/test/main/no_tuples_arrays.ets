/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

class A {
    a: number = -11;
    tuple: [number, string, boolean] = [1, 'hi', true]
    arrs: [[number, number, boolean], [boolean, number, boolean]] = [
        [1, 3.14, true], this.cc()
    ]
    val: (number|string|boolean) [] = new A().tuple
    A() {
        const tuple2: [string, number, boolean] = ['s', 3.14, true]
        let array2: (string | number | boolean)[] = tuple2

    }
    aa(): number {
        let ss: number[] = [-1, -5]
        return ss[0];
    }
    cc(): [boolean, number, boolean] {
        return [true, 33, false];
    }
    dd(): [boolean, string, A] {
        return [true, 'hello', new A()];
    }
    ee() {
        let ccVal: (boolean | number | boolean)[] = this.cc()
        return ccVal;
    }

    ff() {
        let array = tuple;
        return array;
    }
    gg() {
        return this.arrs[0];
    }
}

function test2() {
    const tuple3: [string, number, boolean] = ['s', 3.14, true]
    return tuple3;
}
function test3() {
    let obj: A = new A()
    return obj.dd();
}

let a: A = new A()

const tuple: [number, number, boolean] = [1, 3.14, true]
const tuple1: [number, string, boolean] = [1, '3.14', true]
const array: (number | boolean)[] = tuple
const tuple2: [string, number, boolean] = ['s', 3.14, true]
const array1: (number | string | boolean)[] = tuple1
const array2: (string | number | boolean)[] = tuple2

let tt: (string | number | boolean)[] = test2()
let gg: (boolean | number | boolean)[] = new A().ee()
let ff = new A().ff()
let hh: (boolean | number | boolean)[] =ff
let mm: (boolean | number | boolean)[] = a.gg();
let test: (boolean | string | A)[] = this.test3()

let tuple14: [number, boolean] = [3.14, true]
let array13: (number|boolean)[] = tuple as (number|boolean)[]  //error
let array14: (number|boolean)[] = tuple14 as (number|boolean)[]  //error
let array15: (number|boolean)[] = array as (number|boolean)[]
let array16: (boolean | number | boolean)[] = a.gg() as (boolean | number | boolean)[]   //error
let tuple15: [string, number, boolean] = test2() as [string, number, boolean]
let tuple16: [number, number, boolean] = array as [number, number, boolean]
let tuple17: [number, string, boolean] = ['s', 3.14, true] as [number, string, boolean]
const array17 = Array.from({ length: 5 }, (_, index) => index % 2 === 0 ? index : index % 3 === 0);
let tuple19: [number, boolean] = array17 as [number, boolean]  //error
const originalArray: [number] = [1, 2, 3, 4, 5];
const array18 = originalArray.map((value) => value % 2 === 0 ? true : value * 2);
let tuple20: [number, boolean] = array18 as [number, boolean]   //error
let array20: (number)[] = originalArray as (number)[]   //error

const inputArray: readonly [Record<string, never>, [], () => void] = [
  {},
  [],
  () => {
  }
];
const even = (element: Record<string, never> | [] | (() => void)): boolean => {
  return typeof element === 'function';
};
const res = inputArray.some(even); // error
console.log("res:" + JSON.stringify(res));
console.log(''+inputArray.length) // error
console.log(inputArray.toString()) // error
typeof inputArray.toLocaleString(); // error
function getConcat() {
  inputArray.concat(); // error
  return inputArray.join(','); // error
}
class Demo{
  set(){
    inputArray.slice(1,2); // error
    inputArray.indexOf([]); // error
  }
  get(){
    return inputArray.lastIndexOf([]); // error
  }
}
inputArray.every(()=>{}) // error
inputArray.some(()=>{}) // error
inputArray.forEach(()=>{}) // error
inputArray.map(()=>{}) // error
inputArray.filter(()=>{}) // error
inputArray.reduce((acc, item) => acc + 1, 0); // error
inputArray.reduceRight((acc, item) => acc + 1, 0); // error
inputArray.find((item) => Array.isArray(item)); // error
inputArray.includes(() => {}); // error
inputArray.flat() // error
inputArray.flatMap((item) => [item]); // error
inputArray.findIndex((item) => typeof item === 'function'); // error