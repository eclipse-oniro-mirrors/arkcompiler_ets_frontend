/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

let arr: number[] = [1, 2, 3];
let arr2: int[] = [2, 4];
const combinedArr = arr.concat(/* @@ label */arr2 as Array<number>);

console.log(combinedArr)
console.log(combinedArr.length);
combinedArr[3]


/* @@@ label Warning Warning W0326: Attempting to cast type 'Array<Int>' to 'Array<Double>' can throw 'CastClassError' exception at runtime */
