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
import {OhosInterface} from './oh_modules/ohos_lib';
import { collections } from './@arkts.collections';
// #14071
class A {
  v: string = '';
}
function SetProperty<T extends Object>(oldObj: T, str: string, obj: Object): void {
  oldObj[str] = obj; // Should report error
}
function GetProperty<T extends Object, U>(oldObj: T, str: string): U {
  return oldObj[str]; // Should report error
}
function test() {
  let a: A = { v: 'abc' };
  SetProperty(a, 'u', 'def');
  return GetProperty<A, string>(a, 'v') + GetProperty<A, string>(a, 'u');
}

let ar1 = [1, 2, 3, 4];
let ar2 = [1, '2', 3, 4];
let ar3: number[] = [];

ar1[2];
ar2[2];
ar3[2];

const r0 = [1, 2, 3][1];
let r1 = [1, 2, 3, 4][0]
let r2 = [1, '2', 3, 4][0]

function fobject1(o: object) {
  o['j']
}

function fobject2(o: Object) {
  o['k']
}

let array1 = [0,1]
let array2 = [1,2,3,4,5]
let array3: number[] = [1,2,3,4,5]
let array4: Array<number> = [1,2,3,4,5]
let array5 = new Array(10)
let array6 = new Int8Array(10)
let array7 = new Uint8Array(10)
let array8 = new Uint8ClampedArray(10)
let array9 = new Int16Array(10)
let array10 = new Uint16Array(10)
let array11 = new Int32Array(10)
let array12 = new Uint32Array(10)
let array13 = new Float32Array(10)
let array14 = new Float64Array(10)
let array15 = new BigInt64Array(10)
let array16 = new BigUint64Array(10)

array1[0];
array2[0];
array3[0];
array4[0];
array5[0];
array6[0];
array7[0];
array8[0];
array9[0];
array10[0];
array11[0];
array12[0];
array13[0];
array14[0];
array15[0];
array16[0];

function fff1(r: Record<string, number>) {
    r['bob']
}

enum CCCCCCCCC {
  KATE,
  BOB,
  ROB,
}

CCCCCCCCC['KATE']
CCCCCCCCC['BOB']
CCCCCCCCC['ROB']

CCCCCCCCC[CCCCCCCCC.KATE]
CCCCCCCCC[CCCCCCCCC.BOB]
CCCCCCCCC[CCCCCCCCC.ROB]

let arr32 = new Float32Array([1,2,3])

let iter_arr32 = arr32[Symbol.iterator]()
let tmp_arr32 = iter_arr32.next().value;
while (!!tmp_arr32) {
  console.log(tmp_arr32[0])

  tmp_arr32 = iter_arr32.next().value
}

let arr = new Array<string>()
arr = ['a','f','g']
let iter_arr = arr[Symbol.iterator]()
let tmp_arr = iter_arr.next().value;
while (!!tmp_arr) {
  console.log(tmp_arr[0])
  tmp_arr = iter_arr.next().value
}

// #14415
class ArrayContainer {
  numbers: number[] = [];
}
class NullableArray {
  container: ArrayContainer | null = null;

  print() {
    console.log(this.container?.numbers[0]);
  }
}

let str1 = 'sssss'
let str2 = "aaaaa"
let str3 = `sssss`
let str4 = new String('sssss')
let str5 = str1
let str6 = str2

str1[1]
str2[1]
str3[1]
str4[1]
str5[1]
str6[1]

class AString extends String {}
let str7 = new AString('dwdd')
str7[1]

type IndexableUnion = string[] | (number | string)[] | Uint8Array;
type NonIndexableUnion = string[] | number[] | Uint8Array | number;

function indexUnion(iu: IndexableUnion, niu: NonIndexableUnion) {
  iu[0];
  niu[0];
}

function testLibraryUnnamedType(a: OhosInterface) {
  a['kek'];
}

class MMap<T, U> extends Map<T, U> {}

let mmap1 = new Map<number, string>();
let mmap2 = new Map<string, number>();
let mmap3 = new MMap<string, string>();

mmap1[1];
mmap2['222'];
mmap3["kkr"];

@Sendable
class MyClass extends collections.BitVector {
  constructor() {
    super(0);
    for (let i = 0; i < this.length; i++) {
      this[i] = 1;
    }
  }
}

class AA {
  i: number = 1.0;
  func(b: number) {}
}
let a: object = new AA();
let b: AA = new AA();
b[i] = 1.0;
a["i"] = 2.0;
(b as object)["i"] = 2.0;
(b as object)["func"](1.0);
(b as object)["func1"](1.0);

class A1 {
  foo1() {}
  static foo() {}
  aa: number = 1.0;
}
class B1{
  a: A1 = new A1();
}
let b1: B1 = new B1();
(b1 as object)["a"]["foo1"]();
(b1 as object)["a"]["foo"]();
(b1 as object)["a"]["aa"];
