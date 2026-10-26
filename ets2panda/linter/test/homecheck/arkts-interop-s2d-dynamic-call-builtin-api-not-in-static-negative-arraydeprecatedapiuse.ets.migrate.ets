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

import { array1, array2, array3, foo1, str1 } from './fixtures/interop/s2d/builtin-api/exportEts2';
import { array11, array22, array33, str2 } from './fixtures/interop/s2d/builtin-api/exportEts1';

class A {
    constructor(public base: number) {
    }

    compare(value: number, index: number, arr: Array<number>) {
        return value < this.base;
    }
}

let a = new A(2);
const arrayGlobal1 = array3;
const arrayGlobal2 = arrayGlobal1;

function testArrayEveryAPI(param1: number[], param2: number[]): void {
    array1.every(a.compare, a);                         // 告警1，array1来自arkts1.2，Array Every API不接收第二个参数
    const arrayFromImport = array2;
    const array = arrayFromImport;
    array.every(a.compare, a);                          // 告警2
    arrayGlobal2.every(a.compare, a);                   // 告警3
    const arrayFromGlobal = arrayGlobal1;
    const res = arrayFromGlobal.every(a.compare, a);    // 告警4
    const arrayFromReturnValue = foo1();
    arrayFromReturnValue.every(a.compare, a);           // 告警5
    param1.every(a.compare, a);                         // 告警6
    param2.every(a.compare, a);                         // 告警7
}

function testArrayFlatAPI(): void {
    array1.flat(2);                                     // 告警8，flat接口arkts1.2中不存在
}

function testArrayFlatMapAPI(): void {
    arrayGlobal2.flatMap((value, index, array) => {     // 告警9，flatMap接口arkts1.2中不存在
        return value;
    });
}

function testArrayConcatAPI(): void {
    array1.concat(5, array2);                           // 告警10，array1来自arkts1.2，不接受单个元素的入参
    let newArray = array2.concat(5, array2);            // 告警11
}

class B {
    constructor(public base: number) {
    }

    addBase(value: number, index: number) {
        return value + this.base;
    }
}

let b = new B(2);
function testArrayStaticFromAPI(): void {
    let arr1 = Array.from(array1, b.addBase, b);        // 告警12，array1来自arkts1.2的ArrayLike类型数据
    let arr2 = Array.from(arrayGlobal2, b.addBase, b);  // 告警13，arrayGlobal2来自arkts1.2

    const res1 = Array.from(str1, changeChar, this);    // 告警14，str1来自arkts1.2的Iterable类型数据
}

function testArrayFilterAPI(): void {
    array1.filter(a.compare, a);                        // 告警15，array1来自arkts1.2，Array Filter API不接收第二个参数
}

function testArrayFindAPI(): void {
    const arrayFromReturnValue = foo1();
    arrayFromReturnValue.find(a.compare, a);            // 告警16
}

function testArrayFindIndexAPI(): void {
    const index = array2.findIndex(a.compare, a);       // 告警17
}

function testArrayForEachAPI(): void {
    array2.forEach(a.compare, a);                       // 告警18
}

function testArrayMapAPI(): void {
    array1.map(a.compare, a);                           // 告警19
}

function testArraySomeAPI(): void {
    array1.some(a.compare, a);                           // 告警20
}

class TestField {
    fieldA = Array.from(array1, b.addBase, b);               // 告警21
    static fieldB = array1.filter(a.compare, a);             // 告警22

    static fieldC = array1;
    fieldD = arrayGlobal2;

    static testStaticField() {
        this.fieldC.forEach(a.compare, a);                  // 告警23
        const newArray = Array.from(this.fieldC, b.addBase, b);   // 告警24
    }

    testField() {
        this.fieldD.forEach(a.compare, a);                  // 告警25
        const newArray = Array.from(this.fieldD, b.addBase, b);   // 告警26
    }
}

function changeChar(char: string, index: number): string {
    return `${char.toUpperCase()}(${index})`;
}

testArrayEveryAPI(arrayGlobal1, [1, 2, 3]);
testArrayEveryAPI([1, 2, 3], array1);

let arrayGlobal22 = array2;
arrayGlobal22 = array22;
function positiveArrayEveryAPI(): void {
    const array1 = [1, 2, 3];
    array1.every(a.compare, a);             // 不告警，array1是声明的local变量
    arrayGlobal22.every(a.compare, a);      // 不告警，arrayGlobal22最终由array22赋值，array22来自arkts1.1
    const array2 = array22;
    array2.every(a.compare, a);             // 不告警，array2由array22赋值，array22来自arkts1.1
    testArrayEveryAPI(array11, array22);    // 不告警，array11和array22来自arkts1.1
    arrayGlobal1.every(a.compare);          // 不告警，没有第二个参数
}

function positiveArrayFlatAPI(): {
    array11.flat(1);                        // 不告警，array11来自arkts1.1
}

function positiveArrayFlatMapAPI(): void {
    arrayGlobal22.flatMap((value, index, array) => {     // 不告警，arrayGlobal22最终由array22赋值，array22来自arkts1.1
        return value;
    });
}

function positiveArrayConcatAPI(): void {
    array1.concat(array11, array2);         // 不告警，参数均为数组
    array11.concat(array1, array2);         // 不告警，参数均为数组
}

function positiveArrayStaticFromAPI(): void {
    let arr1 = Array.from(array11, b.addBase, b);       // 不告警，array11来自arkts1.1
    let arr2 = Array.from(arrayGlobal2, b.addBase);     // 不告警，arrayGlobal2来自arkts1.2，没有第三个参数
    const res1 = Array.from(str1, changeChar);          // 不告警，str1来自arkts1.2的Iterable类型数据，没有第三个参数
    const res2 = Array.from(str11, changeChar, this);   // 不告警，str11来自arkts1.1的Iterable类型数据
}

function positiveArrayFilterAPI(): void {
    array11.filter(a.compare, a);                       // 不告警
    array1.filter(a.compare);                           // 不告警
}

function positiveArrayFindAPI(): void {
    const arrayFromReturnValue = foo1();
    arrayFromReturnValue.find(a.compare);               // 不告警
    array22.find(a.compare, a);                         // 不告警
}

function positiveArrayFindIndexAPI(): void {
    const index = array2.findIndex(a.compare);          // 不告警
    const index = array22.findIndex(a.compare, a);      // 不告警
}

function positiveArrayForEachAPI(): void {
    array2.forEach(a.compare);                          // 不告警
    array22.forEach(a.compare, a);                      // 不告警
}

function positiveArrayMapAPI(): void {
    array1.map(a.compare);                              // 不告警
    array11.map(a.compare, a);                          // 不告警
}

function positiveArraySomeAPI(): void {
    array1.some(a.compare);                             // 不告警
    array11.some(a.compare, a);                         // 不告警
}

class PositiveField {
    fieldA = Array.from(array11, b.addBase, b);              // 不告警
    static fieldB = array33.filter(a.compare, a);            // 不告警
    fieldC = array1.some(a.compare);                         // 不告警

    static fieldD = array33;
    fieldE = array3;

    static testStaticField() {
        this.fieldD.forEach(a.compare, a);                  // 不告警
        const newArray = Array.from(this.fieldD, b.addBase, b);   // 不告警
    }

    testField() {
        this.fieldE.forEach(a.compare);                  // 不告警
        const newArray = Array.from(this.fieldE, b.addBase);   // 不告警
    }
}
