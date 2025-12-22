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
let b = an_array[1.23]            // Report error for using double as index.
let c = an_array[12.]            // Report error for using double as index.
let d = an_array[12.0]            // Report error for using double as index.
let f = an_array[Number.MAX_VALUE]            // Report error for using double as index.
let g = an_array[Number.MIN_VALUE]            // Report error for using double as index.
let h = an_array[Number.MAX_SAFE_INTEGER]            // Report error for using double as index.
let j = an_array[3e0]            // Report error for using double via exponential notation as index.
let k = an_array[Math.floor(1.5)]            // Report error for using double as index.

const index_1: number = 1.3;
let index_2: number = 1.3;
an_array[index_1];            // Report error for using const double as index.
an_array[index_2];            // Report error for using double as index.

function getIndex(): number {
  return Math.random() * 10;
}
an_array[getIndex()];            // Report error for using the Math output without casting it to int as index.

let arr:number[] = [1, 2, 3]
enum TE{
  AA = 1.12
}
arr[TE.AA];            // Report error for using double as index.
arr[1.1 as number];            // Report error for using literal casted to double as index.

arr[true?1.3:1.2]            // Report error for using the double inside of a condition as index.

let test = 1;
arr[1 as number];            // Report error for using double as index.
arr[test as number];            // Report error for using double as index.
an_array[arr[1]];            // Report error for using the element of an array<array> as index.

let element = arr[1];
an_array[element];            // Report error for using the element of an array<array> assigned to a variable, as index.