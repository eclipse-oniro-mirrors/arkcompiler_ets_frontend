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
let element1 = an_array[-2]
an_array[-5]
an_array[+5]
an_array[~5]
an_array[!5]
class A {
    a: number = -11;
    A(){}
    aa():number{
        let ss: number[] = [-1,-5]
        ss[-2]
        return ss[0];
        }
}
function test() {
  return -1;
}

let arr: string[] = ['1', '2']
arr[-2]
arr[-0.13536]
arr[test()]
arr[test()*6]
const b: number = -9;
arr[b]
let a: A = new A()
arr[a.a]
arr[a.aa()]

arr[a.a+5]
arr[a.aa()*5]

arr[-1*6]
const m:number = Infinity;
arr[-Infinity]
arr[m]
arr[-m]
let n:number = Number.MIN_SAFE_INTEGER;
arr [-Number.MIN_SAFE_INTEGER]
arr[Number.MIN_SAFE_INTEGER]
arr[n]
arr[-n]
arr[-Number.NaN]
arr[Number.NaN]
const p:number = Number.MAX_SAFE_INTEGER;
arr [-Number.MAX_SAFE_INTEGER]
arr[Number.MAX_SAFE_INTEGER-5]
arr[-p]
arr[-p+7]
let q:number = Number.POSITIVE_INFINITY
arr[-Number.POSITIVE_INFINITY] 
arr[q]
arr[-q]
const c = Number.NEGATIVE_INFINITY
arr[-Number.NEGATIVE_INFINITY]
arr[c]
const y1 = 6+n
const y2 = -6-n
const y3 = -6*m 
const y4 = -6/2
const y5 = -6/b
const y6 = -6%5
arr[y1]
arr[y2]
arr[y3]
arr[y4]
arr[y5]
arr[y6]
const y7 = -(-6/b)
arr[y7]
arr[-1 as string]
arr[-1 as number]
arr[-1 as Number]
arr[NaN];
arr[-NaN];
enum E34 {
  BLUE = -1,
}
let a34 = arr[E34.BLUE];
let a35 = arr[-E34.BLUE];
const num = -2;
arr[(num + 1) as number];
arr[num as number];