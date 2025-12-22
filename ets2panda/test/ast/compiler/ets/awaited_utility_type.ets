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
 
type A = Awaited<string> // string
type B = Awaited<Promise<Promise<number|Promise<boolean>>>> // number | boolean
type C = Awaited<Promise<number> | boolean> // number | boolean
type D = Awaited<number | string> // number | string

type E = Awaited<Promise<Array<Promise<number>|Promise<boolean>>>> 
const results: E = [Promise.resolve(1.0)] // OK
let results_2 : E = [1.0]
let a: A = "a"
let valueB: B = true
let valueD: D = 123

 /* @@? 23:22 Error Semantic error ESE0227: Array element at index 0 with type 'Double' is not compatible with the target array element type 'Promise<Double>|Promise<Boolean>' */