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
'use static'

const arr: int[] = [1, 2, 3, 4];

for (let i = 0; i < arr.length; i++) {
    arr[i]; //legal
}

for (let i = 0; i < 100; i++) {
    console.log(i); //legal
}

const arr2: int[] = [1, 2, 3, 4];
for (let i = 0; i < 100; i++) {
    arr2[10] //should report
}

const arr3: int[] = [1, 2, 3, 4];
for (let i = 0; i < arr3.length; i++) {
    arr3[10] //should report
}

const arr4: int[] = [1, 2, 3, 4];
let x: int = 3;
for (let i = 0; i < arr4.length; i++) {
    arr4[x]; //should report
}

const arr5: int[] = [1, 2, 3, 4];
for (let i = 0; i < 10; i++) {
    arr5[i]; //should report
}


const arr6: int[] = [1, 2, 3, 4];
if (arr6.length > 10) {
    arr6[10]
}

const arr7: int[] = [1, 2, 3, 4];
if (arr7.length > 10) {
    return;
}

arr7[10]

const arr8: int[] = [1, 2, 3, 4];
const index: int = 9;
if (arr8.length > 10 && index > 0) {
    return;
}

arr8[index];

const arr9: int[] = [1, 2, 3, 4];
if (arr9.length > 10 && index > 0) {
    arr9[index];
}

const arr10: int[] = [1, 2, 3, 4];
if (index > 0) {
    arr10[index];
}

const arr10: int[] = [1, 2, 3, 4];
let newIndex = 10;
if (arr10.length > newIndex) {
    return;
}

newIndex = 22;

arr10[newIndex];

let arr = [0, 1, 2, 3, 4, 5]
for(let i = 0; i < arr.length; i++) {
arr[i] = arr[i] + 1;
}
for(let i = 0; i < arr.length; i++) {
i = 10;
arr[i] = arr[i] + 1;
}

let arr = [0, 1, 2, 3, 4, 5]
let idx = 2;
if(idx > 0 && idx < arr.length) {
arr[idx] = arr[idx] + 1;
}
if(idx > 0 && idx < arr.length) {
idx = 10;
arr[idx] = arr[idx] + 1;
}

let arr = [0, 1, 2, 3, 4, 5]
let idx = 0;
while(idx > 0 && idx < arr.length) {
arr[idx] = arr[idx] + 1;
idx++;
idx = 10;
}
while(idx > 0 && idx < arr.length) {
idx = 10;
arr[idx] = arr[idx] + 1;
}

let arr = [0, 1, 2, 3, 4, 5]
let idx = 0;
arr[idx];
arr[10];
if (arr.length > 10) {
arr[10] = 10;
}

function foo():int{
return 1;
}

arr[44/3];
arr[foo()];
arr[()=>{return 1}];
if(arr.length > foo()) {
arr[foo()];
}
if(arr.length > 44/3) {
arr[4*4/3];
}

let arr1:number[] = [1, 1.5,45,2]

function foo(i:number):number{
  return i;
}

arr1[3*5] = 23;
arr1[parseInt("16")] = 23;
arr1[foo(16)] = 23

let arr1:number[] = [1, 1.5,45,2]

arr1[Number.MAX_VALUE] = 23;
arr1[Number.MAX_SAFE_INTEGER] = 23;

let arr1:number[] = [1, 1.5,45,2]
function foo(i:number):number{
  return i;
}
arr1[(24)] = 23;
arr1[+24] = 23;
enum TE{
  AA = 12
}
arr1[TE.AA] = 12;

let a: string[] = [];
let b: Array<string> = new Array(a.length);
for (let i = 0; i < a.length; i++) {
  b[i];
}

function test1(arr: object[]) {
  if (arr.length == 1 && arr[0] instanceof Array) {}
}

function test2(arr: object[]) {
  const str = arr.length < 2 ? arr[0] : arr[1];
}

function test3(arr: object[]) {
  let index = 1;
  if (index < arr.length) {
    const value = arr[index++];
  }

  if (index > arr.length) {
    const value = arr[++index];
  }
}

function test4(arr?: object[]) {
  if (arr && arr.length > 0) {
    const a = arr[0];
  }
}

function test5(arrA: object[], arrB: object[]) {
  const length = Math.max(arrA.length, arrB.length);
  for (let i = 0; i < length; i++) {
    let numA = arrA[i] || 0;
    let numB = arrB[i] || 0;
  }
}

function test6(arr: object[]) {
  if(arr.length < 2){
    arr[0]
  }
}

let arr:number[]=[];
if(arr.length > 1){
    arr[1]
}

let arr1 = [0, 1, 2, 3, 4, 5]
let arr2 = new Array<number>();
let arr3 = new Array<number>(10)

let index1 = 0
let index2 = 1
let index3 = 2


arr1[index1]

if (index1 >= 0 && index1 < arr1.length) {
    arr1[index1]
}

arr1[3]

if (arr1.length > 3) {
    arr1[3]
}

for(let i = 0; i < arr1.length; i++) {
    arr1[i];
}

let i = 0;
while(i < arr1.length) {
    arr1[i];
}

if (index1 >= 0 && index1 < arr1.length) {
    index1 = 100
    arr1[index1]
}

let a: string[] = [];
let b: Array<string> = new Array(a.length);
for (let i = 0; i < a.length; i++) {
  b[i];
}

for (let i = 0; i < a.length; i++) {
  b.push("abc");
  b[i];
}

for (let i = 0; i < a.length; i++) {
  b.pop();
  b[i];
}

let arr_len = 8;
let b: Array<string> = new Array(arr_len);
for (let i = 0; i < arr_len; i++) {
  b[i];
}

function test7(config: Record<string, object>, name: string) {
  if (config[name][getValue()]) {

  }
}

function getValue(): string {
  return '';
}

const arr: Record<string, string> = {};
let keys: string[] = Object.keys(arr);
let values: string[] = Object.values(arr);
for (let i = 0; i < keys.length; i++) {
  values[i];
}

const arr: Map<string, string> = new Map();
let keys: string[] = Object.keys(arr);
let values: string[] = Object.values(arr);
for (let i = 0; i < keys.length; i++) {
  values[i];
}

let concatArray: ConcatArray<number> = new Array<number>(1.0, 2.0, 3.0);

let tempNum: number = concatArray[5];


for (let i = 0; i < concatArray.length; i++) {
    concatArray[i]
};

if (concatArray.length > 10) {
    concatArray[10]
};

class DummyClass {
    public a1: Array<number> = new Array<number>();

    foo() {
        this.a1[123] = 123;
    }
}

let dc = new DummyClass();

dc.a1[123] = 1234;
