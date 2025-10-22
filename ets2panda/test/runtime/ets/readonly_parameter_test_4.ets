/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

function foo1 (tuple1: [number, string, boolean], arr1: number[]) {
    let arr2 = arr1 as readonly number[]
    let arr3 = arr1 as Readonly<number[]>
    
    let tuple2 = tuple1 as readonly [number, string, boolean]
}

function main() : void {
    let arr1 : int[] = [1, 2]
    let arr2 = arr1 as readonly int[]
    let arr3 = arr1 as Readonly<int[]>
    
    let tuple1 : [boolean, int] = [true, 2]
    let tuple2 = tuple1 as readonly [boolean, int]
}