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

function foo(index:number){
  let an_array = [1,2,3]
  let a = an_array[index]
  let a1 = an_array[index + 1]
  let a2 = an_array[index + 1.1]
  let b = an_array[1.23]
  let c = an_array[true]
  let d = an_array['index']
  let e = an_array[undefined]
  let f = an_array[null]
  let g = an_array[]
  let h = an_array[12.]
  let i = an_array[12.0]
  let j = an_array[0]
  let k = an_array[Number.MAX_VALUE]
  let l = an_array[Number.MIN_VALUE]
  let m = an_array[Number.MAX_SAFE_INTEGER]
}

let array = [1,2,3]
const index_1: number = 1.3;
let index_2: number = 1.3;
let index_3: number = 1;
array[index_1];
array[index_2];
array[index_3];
let index_4: int = 2
array[index_4];
const index_5: number = 1.0;
array[index_5];

function getIndex(): number {
  return Math.random() * 10;
}
let array1 = [1, 2, 3];
array1[getIndex()];

let array2 = [1, 2, 3];
for (let i: number = 0; i < array2.length; i++) {
  console.log(array2[i]);
}

for (let i: int = 0; i < array2.length; i++) {
  console.log(array2[i]);
}

let arr1:number[] = [1, 2, 3]
enum TE{
  AA = 1.12
  BB = 0
}
arr1[TE.AA];
arr1[TE.BB];
arr1[+0];
arr1[-0];
arr1[+1];
arr1[-1];
arr1[1.1 as number];

let a:short = 1;
let b:byte = 1;
let c:int = 1;
let d:long = 1;

let arr:number[] = [1,2,3]
arr[true?1.3:1.2]
arr[a] = 1;
arr[b] = 1;
arr[c] = 1;
arr[d] = 1;

let test = 1;
arr[1 as number];
arr[test as number];

@Component
struct Test {
  @Link testIndex: number;

  async cateMode(testIndex: number) {
    emitter.on(innerEvent, (eventData) => {
      if (this.testIndex == 0) {
        let array = [1,2,3];
        array[this.testIndex];
      }
    })
  }
}
