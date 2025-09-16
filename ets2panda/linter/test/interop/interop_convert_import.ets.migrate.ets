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
 'use static'

 import {foo, foo2, foo3, foo4, array_val, null_val, undefined_val} from "./interop_convert_import_js.js"

 let a: number = foo.getProperty("num").toNumber()
 let a1: boolean = foo2.getProperty("bool").toBoolean()
 let a2: string = foo3.getProperty("str").toString()
 let a3: bigint = foo4.getProperty("big").toBigInt()

test_helper.test(() => {
return (array_val as Array).toString() === new Array(1, 2, 3).toString();// 扫描出 arkts-interop-js2s-convert-js-type - no pass
}, "array_val as Array === [1, 2, 3]");

// convert type - Array
test_helper.test(() => {
return (array_val as number[]).toString() === [1,2,3].toString();// 扫描出 arkts-interop-js2s-convert-js-type - no pass
}, "array_val as Array === [1, 2, 3]");

// convert type - null
test_helper.test(() => {
return null_val as null === null;// 扫描出 arkts-interop-js2s-convert-js-type - no pass
}, "null_val as null === null");

// convert type - undefined
test_helper.test(() => {
return undefined_val as undefined === undefined; // 扫描出 arkts-interop-js2s-convert-js-type - no pass
}, "undefined_val as undefined === undefined");
