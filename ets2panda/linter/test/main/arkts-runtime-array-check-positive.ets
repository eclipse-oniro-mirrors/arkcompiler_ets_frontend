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

const arr: int[] = [1, 2, 3, 4];

for (let i = 0; i < arr.length; i++) {
    arr[i]; //legal
}

for (let i = 0; i < 100; i++) {
    console.log(i); //legal
}

const arr6: int[] = [1, 2, 3, 4];
if (arr6.length > 10) {
    arr6[10] //legal
}

const arr7: int[] = [1, 2, 3, 4];
if (arr7.length > 10) {
    return;
}

arr7[10] //legal

let arr = [0, 1, 2, 3, 4, 5]
for(let i = 0; i < arr.length; i++) {
arr[i] = arr[i] + 1; //legal
}

let arr = [0, 1, 2, 3, 4, 5]
let idx = 2;
if(idx > 0 && idx < arr.length) {
arr[idx] = arr[idx] + 1;
}

function test1(arr: object[]) {
  if (arr.length == 1 && arr[0] instanceof Array) {} //legal
}

function test2(arr: object[]) {
  const str = arr.length < 2 ? arr[0] : arr[1]; //legal
}

function test4(arr?: object[]) {
  if (arr && arr.length > 0) {
    const a = arr[0]; //legal
  }
}

if (arr.length > 10) {
arr[10] = 10; // legal
}

let arr:number[]=[];
if(arr.length > 1){
    arr[1] //legal
}

let arr1 = [0, 1, 2, 3, 4, 5]
if (arr1.length > 3) {
    arr1[3] //legal
}

for(let i = 0; i < arr1.length; i++) {
    arr1[i]; //legal
}

let i = 0;
while(i < arr1.length) {
    arr1[i]; //legal
}

let aa: number[] = [];
aa.length = 101;

if (100 < aa.length) {
  aa[99] = 1;
}

if (aa.length > 100) {
  aa[99] = 2;
}

if (aa.length == 100) {
  aa[99] = 3;
}

if (aa.length === 100) {
  aa[99] = 4;
}

let indexa = 100;

if (indexa < aa.length) {
  aa[indexa] = 5;
}

if (aa.length > indexa) {
  aa[indexa] = 6;
}

if (aa.length == indexa) {
  aa[indexa] = 7;
}

if (aa.length === indexa) {
  aa[indexa] = 8;
}