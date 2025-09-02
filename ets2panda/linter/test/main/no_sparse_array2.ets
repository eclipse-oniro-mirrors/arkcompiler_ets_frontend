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

/*
 * Should report (arkts-no-sparse-array)
 */

let a = [1, , , 3];
let b = [];

let d = Promise.race<number>([]);
let e = Promise.all<number>([]);
let f = Promise.allSettled<number>([]);
let g = Promise.any<number>([]);
let h = Promise.resolve<number[]>([]);

let d1 = Promise.race([]);
let e1 = Promise.all([]);
let f1 = Promise.allSettled([]);
let g1 = Promise.any([]);
let h1 = Promise.resolve([]);

let bigInt64 = new BigInt64Array([]);
let bigUint64 = new BigUint64Array([]);
let floast64 = new Float64Array([]);
let float32 = new Float32Array([]);
let int32 = new Int32Array([]);
let int64 = new Int16Array([]);
let int8 = new Int8Array([]);
let uint32 = new Uint32Array([]);
let uint64 = new Uint16Array([]);
let uint8 = new Uint8Array([]);
let uint8Clamped = new Uint8ClampedArray([]);

let str1 = new String([]);
let bool1 = new Boolean([]);
let num1 = new Number([]);
let obj1 = new Object([]);

let set1 = new WeakSet<object>();
set1.has([]);
set1.add([]);
set1.delete([]);

let map1 = new WeakMap<object, string>();
map1.has([]);
map1.set([], "");
map1.delete([]);
map1.get([]);

let set2 = new Set<object>();
set2.has([]);
set2.add([]);
set2.delete([]);

let map2 = new Map<object, string>();
map2.has([]);
map2.set([], "");
map2.delete([]);
map2.get([]);