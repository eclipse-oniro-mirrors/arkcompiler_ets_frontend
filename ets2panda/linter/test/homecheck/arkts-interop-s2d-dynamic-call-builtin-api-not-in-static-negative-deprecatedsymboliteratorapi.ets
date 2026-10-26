/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { array1, array2, set1, map1, foo1, foo2, foo3 } from './fixtures/interop/s2d/builtin-api/exportEts2';
import { array11, array22, set11, map11 } from './fixtures/interop/s2d/builtin-api/exportEts1';
import { Animal, Dog } from './fixtures/interop/s2d/builtin-api/BaseClass';

const arrayGlobal1 = array2;
const arrayGlobal2 = arrayGlobal1;

function testArraySymbolIterator(param1: number[], param2: number[]): void {
    array1[Symbol.iterator]();                         // 告警1
    const arrayFromImport = array2;
    const array = arrayFromImport;
    array[Symbol.iterator]();                          // 告警2
    arrayGlobal2[Symbol.iterator]();                   // 告警3
    const arrayFromGlobal = arrayGlobal1;
    const res = arrayFromGlobal[Symbol.iterator]();    // 告警4
    const arrayFromReturnValue = foo1();
    arrayFromReturnValue[Symbol.iterator]();           // 告警5
    param1[Symbol.iterator]();                         // 告警6
    param2[Symbol.iterator]();
}

function testReflectArraySymbolIterator(param1: number[], param2: number[]): void {
    let iter = Reflect.get(array1, Symbol.iterator);     // 告警7
    const arrayFromImport = array2;
    const array = arrayFromImport;
    Reflect.get(array, Symbol.iterator);                          // 告警8
    Reflect.get(arrayGlobal2, Symbol.iterator);                   // 告警9
    const arrayFromGlobal = arrayGlobal1;
    const res = Reflect.get(arrayFromGlobal, Symbol.iterator);    // 告警10
    const arrayFromReturnValue = foo1();
    Reflect.get(arrayFromReturnValue, Symbol.iterator);           // 告警11
    Reflect.get(param1, Symbol.iterator);
    Reflect.get(param2, Symbol.iterator);                         // 告警12
}

testArraySymbolIterator(array1, [1, 2, 3]);

testReflectArraySymbolIterator([1, 2, 3], arrayGlobal2);

function testSetSymbolIterator(): void {
    let iterator = set1[Symbol.iterator]();             // 告警13
    let iter = Reflect.get(set1, Symbol.iterator);      // 告警14
}

function testMapSymbolIterator(): void {
    let iterator = map1[Symbol.iterator]();             // 告警15
    let iter = Reflect.get(map1, Symbol.iterator);      // 告警16
}

let arrayGlobal22 = array2;
arrayGlobal22 = array22;
function positiveArrayEveryAPI(): void {
    const array1 = [1, 2, 3];
    array1[Symbol.iterator]();             // 不告警
    arrayGlobal22[Symbol.iterator]();      // 不告警
    const array2 = array22;
    array2[Symbol.iterator]();             // 不告警

    Reflect.get(array1, Symbol.iterator);             // 不告警
    Reflect.get(arrayGlobal22, Symbol.iterator);      // 不告警
    Reflect.get(array2, Symbol.iterator);             // 不告警
}

let globalSet = new Set<[1, 2, 3]>;
function positiveSetSymbolIterator(): void {
    globalSet[Symbol.iterator]();          // 不告警
    let set = new Set<[1, 2, 3]>;
    set[Symbol.iterator]();                // 不告警

    Reflect.get(globalSet, Symbol.iterator);          // 不告警
    Reflect.get(set, Symbol.iterator);                // 不告警
}

function positiveMapSymbolIterator(): void {
    map11[Symbol.iterator]();              // 不告警
    Reflect.get(map11, Symbol.iterator);              // 不告警
}
