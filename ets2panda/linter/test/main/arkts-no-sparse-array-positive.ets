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

let c: number[] = [];

let b = [1, undefined, undefined, 2];

let i = Promise.race<number>([1.0]);
let j = Promise.all<number>([2.0]);
let k = Promise.allSettled<number>([3.0]);
let l = Promise.any<number>([4.0]);
let m = Promise.resolve<number[]>([5.0]);

let i1 = Promise.race([1]);
let j1 = Promise.all([2]);
let k1 = Promise.allSettled([3]);
let l1 = Promise.any([4]);
let m1 = Promise.resolve([5]);

let bigInt64 = new BigInt64Array([1n]);
let bigUint64 = new BigUint64Array([1n]);
let floast64 = new Float64Array([1.0]);
let float32 = new Float32Array([1.0]);
let int32 = new Int32Array([1.0]);
let int64 = new Int16Array([1.0]);
let int8 = new Int8Array([1.0]);
let uint32 = new Uint32Array([1.0]);
let uint64 = new Uint16Array([1.0]);
let uint8 = new Uint8Array([1.0]);
let uint8Clamped = new Uint8ClampedArray([1.0]);

let str2: Array<string> = new Array<string>();
let bool1 = new Boolean(['1']);
let num1 = new Number(10);
let obj1 = new Object();

function foo(arr: number[]) {
    return;
}
foo([]);

function foo1(arr: Array<number>) {
    return;
}
foo1([]);

function foo2<T>(a: T) {
    return;
}
foo2<number[]>([]);

function foo3<T>(a: T[]) {
    return;
}
foo3<number>([]);

function foo4<T extends number[]>(a: T) {
    return;
}
foo4<number[]>([]);

function foo5<T extends number>(a: T[]) {
    return;
}
foo5<number>([]);

let set1 = new WeakSet<object>();
set1.has(['1']);
set1.add([] as string[])
set1.add(['1']);
set1.delete(['1']);

let map1 = new WeakMap<object, string>();
map1.has(['1']);
map1.set([] as string[], "")
map1.set(['1'], "");
map1.delete(['1']);
map1.get(['1']);

let set2 = new Set<object>();
set2.has(['1']);
set2.add(['1']);
set1.add([] as string[])
set2.delete(['1']);

let map2 = new Map<object, string>();
map2.has(['1']);
map2.set(['1'], "");
map2.set([] as string[], "")
map2.delete(['1']);
map2.get(['1']);

let set3 = new Set<number[]>();
set1.add([1, 2, 3])