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

let foa:(c:string, ...args:FixedArray<number>)=>void = (c:number, ...args:FixedArray<number>)=>{}

let fob:(...args:FixedArray<number>)=>number = (...args:FixedArray<number>) =>{}

let foc:(c:string, ...args:FixedArray<number>)=>string = (c:number, ...args:FixedArray<string>):string=>{}

/* @@? 16:56 Error TypeError: Type '(c: Double, ...args: FixedArray<Double>) => void' cannot be assigned to type '(c: String, ...args: FixedArray<Double>) => void' */
/* @@? 18:48 Error TypeError: Type '(...args: FixedArray<Double>) => void' cannot be assigned to type '(...args: FixedArray<Double>) => Double' */
/* @@? 20:58 Error TypeError: Type '(c: Double, ...args: FixedArray<String>) => String' cannot be assigned to type '(c: String, ...args: FixedArray<Double>) => String' */
