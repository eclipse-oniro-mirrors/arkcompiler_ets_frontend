/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

let n1:number =  123;
let n2:bigint = 456n;

n1 <= n2
n1 == n2
n1 >= n2

n1 != n2
n1 !== n2
n1 === n2
n1 > n2
n1 < n2

n1 = n2
n1 + n2
n1 - n2
n1 * n2
n1 / n2
n1 % n2

function parFunPar(params: boolean) {}
parFunPar(1n > 1.0);
parFunPar(1n < 1.0);
parFunPar(1n >= 1.0);
parFunPar(1n <= 1.0);
parFunPar(1n == 1.0);

function parFunReturn() : boolean {
  return 1n > 1.0;
}
function parFunReturn1() : boolean {
  return 1n < 1.0;
}
function parFunReturn2() : boolean {
  return 1n >= 1.0;
}
function parFunReturn3() : boolean {
  return 1n <= 1.0;
}
function parFunReturn4() : boolean {
  return 1n == 1.0;
}

if (1n > 1.0) {
    console.log('1n > 1.0');
}
if (1n < 1.0) {
    console.log('1n < 1.0');
}
if (1n >= 1.0) {
    console.log('1n >= 1.0');
}
if (1n <= 1.0) {
    console.log('1n <= 1.0');
}
if (1n == 1.0) {
    console.log('1n == 1.0');
}

let compareA6:boolean = true;
compareA6 = 2n > 1.0 && 1n > 1.5 || 2n >= 2.0;

const condition = true
compareA6 = ((1n > 1.0) ? (1n < 1.0) : (1n >= 1.0));
compareA6 = ((1n <= 1.0) ? (1n == 1.0) : (1n > 1.5));
function comparePar2() {
  return 5n > 5.0 || 5n < 5.0 || 5n >= 5.0 || 5n <= 5.0 || 5n == 5.0;
}