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
 
let an_array = [1,2,3]
let element = an_array[0]
let element1 = an_array[-2]         // Report error for using negative integer as index.
an_array[-5]         // Report error for using negative integer as index.
an_array[+5]
an_array[~5]         // Report error for using negative integer as index. (~5 is -6)
class A {
  a: number = -11;
  A(){}
  aa():number{
    let ss: number[] = [-1,-5]
    ss[-2]         // Report error for using negative integer as index.
    return ss[0];
  }
}
function test(): number {
  return -1;
}

let arr: string[] = ['1', '2']
arr[-2]         // Report error for using negative integer as index.
arr[-0.13536]         // Report error for using negative double as index.
arr[test()]         // Report error for using negative integer as index.
arr[test()*6]         // Report error for using negative integer as index.
const b: number = -9;
arr[b]         // Report error for using const negative integer as index.
let a: A = new A()
arr[a.a]         // Report error for using class member negative integer as index.
arr[a.aa()]         // Report error for using negative integer as index.

arr[a.a+5]         // Report error for using negative integer as index.
arr[a.aa()*5]         // Report error for using negative integer as index.

arr[-1*6]         // Report error for using negative integer as index.
const m:number = Infinity;
arr[-Infinity]         // Report error for using negative double (Infinity) as index.
arr[m]         // Report error for using double (Infinity) as index.
arr[-m]         // Report error for using negative double (Infinity) as index.
let n:number = Number.MIN_SAFE_INTEGER;
arr [-Number.MIN_SAFE_INTEGER]         // Report error for using negative double as index.
arr[Number.MIN_SAFE_INTEGER]         // Report error for using double as index.
arr[n]         // Report error for using double as index.
arr[-n]         // Report error for using negative double as index.
arr[-Number.NaN]         // Report error for using negative double (NaN) as index.
arr[Number.NaN]         // Report error for using double (NaN) as index.
const p:number = Number.MAX_SAFE_INTEGER;
arr [-Number.MAX_SAFE_INTEGER]         // Report error for using negative double as index.
arr[Number.MAX_SAFE_INTEGER-5]
arr[-p]         // Report error for using negative double as index.
arr[-p+7]         // Report error for using negative double as index.
let q:number = Number.POSITIVE_INFINITY
arr[-Number.POSITIVE_INFINITY]         // Report error for using negative double (Infinity) as index.
arr[q]         // Report error for using double (Infinity) as index.
arr[-q]         // Report error for using negative double (Infinity) as index.
const c = Number.NEGATIVE_INFINITY
arr[-Number.NEGATIVE_INFINITY]         // Report error for using double (Infinity) as index.
arr[c]         // Report error for using negative double (Infinity) as index.
const y1 = 6+n
const y2 = -6-n
const y3 = -6*m
const y4 = -6/2
const y5 = -6/b
const y6 = -6%5
arr[y1]         // Report error for using double as index.
arr[y2]         // Report error for using negative double as index.
arr[y3]         // Report error for using negative double (Infinity) as index.
arr[y4]         // Report error for using negative integer as index.
arr[y5]         // Report error for using double as index.
arr[y6]         // Report error for using negative integer as index.
const y7 = -(-6/b)
arr[y7]         // Report error for using double as index.
arr[-1 as number]         // Report error for using literal casted to negative integer as index.
arr[NaN];         // Report error for using double (NaN) as index.
arr[-NaN];         // Report error for using negative double (NaN) as index.
enum E34 {
  BLUE = -1,
}
let a34 = arr[E34.BLUE];         // Report error for using enum negative integer as index.
let a35 = arr[-E34.BLUE];
const num = -2;
arr[(num + 1) as number];         // Report error for using literal casted to negative integer as index.
arr[num as number];         // Report error for using const casted to negative integer as index.