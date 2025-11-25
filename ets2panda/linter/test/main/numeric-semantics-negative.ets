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

let a : [number, number, boolean] = [1, 1, true]
a = [2, 2, false]
a = [2.0/3, 3/4.0, false]

let arr:number[] = [1, 2, 3] // number to int
arr = [2, 3, 4]
arr[0] = 2/3;// cannot assign double to int
arr = [1/3, 2/3, 4] // cannot assign double to int

let arrT:Array<number> = [1, 2, 3, 4]
let arrB:Array<number> = arrT??[0]// cannot assign double to int
let arr3:Array<int> = [1, 2, 3, 4]

let b:boolean = true
let arr1:int[] = [1, 2, 3]
let arr2:number[] = b? arr : [0, 1, 2] // cannot assign double to int

let arrE = [1, 2, 3, 4]
let arrA:Array<number> = arrE||[1, 2, 3, 4] // cannot assign double to int

let a: [number, number, boolean] = [1, 1, true]
let b = 2

a = [b/3, 1/b, false] // default number to double

let arr:number[] = [1, 2, 3]
let a = 2
arr[0] = a/3; // default number to double
arr = [1/a, 2/3, a/3] // default to number to double
