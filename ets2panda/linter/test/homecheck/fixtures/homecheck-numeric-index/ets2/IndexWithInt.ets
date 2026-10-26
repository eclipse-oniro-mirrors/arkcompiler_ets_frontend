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

'use static'

import { export2IndexInt, export2IndexNumber, export2FunctionInt, export2FunctionNumber } from './ExportETS2';
import { export1IndexInt, export1IndexNumber, export1FunctionInt, export1FunctionNumber, Export1Class } from '../ets1/ExportETS1';

const arr = [1, 2, 3, 4, 5];

function testArrayIndexWithInt(): void {
    const index: number = 10;       // warning1, defined as int, arkts-numeric-semantic
    let item = arr[index];
    item = arr[0];

    const a = 20;                   // warning2, defined as int, arkts-numeric-semantic
    const b = a;                    // warning3, defined as int, arkts-numeric-semantic
    item = arr[b];

    let c = 1 + 2;                  // warning4, defined as int, arkts-numeric-semantic
    item = arr[~c];

    let d = c;                      // warning5, defined as int, arkts-numeric-semantic
    item = arr[-d];
}

function testArrayIndexWithNumber(): void {
    const index: number = 10.1;
    let item = arr[index];          // warning6, warning only, arkts-array-index-expr-type
    const a = 20.2;                 // warning7, defined as number, arkts-array-index-expr-type
    const b = a;                    // warning8, defined as number, arkts-array-index-expr-type
    item = arr[b];                  // warning9, warning only, arkts-array-index-expr-type

    let c = 2;                      // warning10, defined as number, arkts-numeric-semantic
    arr[c];                         // warning11, warning only, arkts-array-index-expr-type
    c = 3.3;
}

function testNoInfiniteLoop(): void {
    let a = export2IndexInt;        // warning12, defined as number, export variable always number, arkts-numeric-semantic
    let b = a + 2;                  // warning13, defined as number, arkts-numeric-semantic
    a = b + 3;
    let item = arr[a];              // warning14, warning only, arkts-array-index-expr-type
}

function testOtherTypeIndex(): void {
    const index: any = export2IndexInt;
    let item = arr[index];          // warning15, warning only, arkts-array-index-expr-type

    const a: number | string = 2;
    item = arr[a];                  // warning16, warning only, arkts-array-index-expr-type
}

const globalIndexInt = 3;           // warning17, defined as int, arkts-numeric-semantic
const globalIndexNumber = 3.3;      // warning18, defined as number, arkts-array-index-expr-type

function testIndexWithReturnValue(): void {
    let item = arr[export2FunctionInt(globalIndexInt)];
    item = arr[export2FunctionNumber()];            // warning19, warning only, arkts-array-index-expr-type
}

function testIndexWithArrayItem(): void {
    let indexes = [0, 1, 2];
    let item = arr[indexes[0]];                    // warning20, index from another array not supported, warning only, arkts-array-index-expr-type
}

class A {
    b: number = 1;
    static c = 2;                                   // warning21, defined as number, arkts-numeric-semantic
}

function testIndexWithFieldRef(): void {
    const a = new A();
    let item = arr[a.b];                    // warning22, warning only, a.b is public property, considered unresolvable, arkts-array-index-expr-type
    intValue = arr[A.c];                    // warning23, warning only, A.cis public property, considered unresolvable, arkts-array-index-expr-type
}

function indexWithParam(index: number): void {
    let item = arr[index];                  // warning24, warning only, cross-function depth exceeds limit 2, arkts-array-index-expr-type - deleted with param
}

function callOne(a: number): void {
    indexWithParam(a);
}

function callTwo(b: number): void {
    callOne(b);
}

function callThree(c: number): void {
    callTwo(c);
}

function outer1(a: number, b: string): void {        // warning25, defined as int, arkts-array-index-expr-type
    function inner1() {
        let item = arr[a];
    }
}

function outer2(a: number, b: string): void {
    function inner2() {
        let item = arr[a];                          // warning26, warning only, arkts-array-index-expr-type
    }
}

function callOuter2WithNumber(): void {
    outer2(globalIndexNumber, 'abc');
}

function testLocalInRightOp(): void {
    let a = 1;                                      // warning27, defined as int, arkts-numeric-semantic
    let item = arr[a++];

    let b = a * 1.1;                                // warning28, defined as number, arkts-array-index-expr-type
    arr[b];                                         // warning29, warning only, arkts-array-index-expr-type
}

function bar(a: number): number {                   // warning30, a type can be converted to int, arkts-array-index-expr-type
    return a * 1.2;
}

function testLocalInOtherStmt(): number {
    let a = 1;                                      // warning31, defined as int, arkts-numeric-semantic
    let item = arr[a];
    let b = bar(a);                                 // warning32, defined as number, arkts-array-index-expr-type
    item = arr[b];                                  // warning33, warning only, arkts-array-index-expr-type

    if (a > 0 && b < 0) {
        return -a;
    }
    return a;
}

function testImportFromNoETS2(): void {
    let index = export1IndexInt;                    // warning34, defined as number, arkts-array-index-expr-type
    let item = arr[index];                          // warning35, index from non-ets1.2 file, warning only, arkts-array-index-expr-type
    index = export1IndexNumber;
    item = arr[index];                              // warning36, index from non-ets1.2 file, warning only, arkts-array-index-expr-type

    index = export1FunctionInt();
    item = arr[index];                              // warning37, index from non-ets1.2 file, warning only, arkts-array-index-expr-type
    index = export1FunctionNumber();
    item = arr[index];                              // warning38, index from non-ets1.2 file, warning only, arkts-array-index-expr-type

    let a = new Export1Class();
    index = a.goo();
    arr[index];                                     // warning39, index from non-ets1.2 file, warning only, arkts-array-index-expr-type
    index = A.foo();
    arr[index];                                     // warning40, index from non-ets1.2 file, warning only, arkts-array-index-expr-type
}

export function calledByEts2(a: number): void {     // warning41, can be converted to int - deleted with param
    let item = arr[a];
}

export function calledByEts1(b: number): void {
    let item = arr[b];                              // warning42, index from non-ets1.2 file - deleted with param
}

function otherCases(): void {
    let targetArray = ['a', 'b', 'c'];
    let index = targetArray.indexOf('a');           // warning43, defined as number, arkts-array-index-expr-type
    let item = arr[index];                          // warning44, indexfrom builtin, non-ets1.2 file, arkts-array-index-expr-type
    let len = targetArray.length;                   // warning45, defined as number, arkts-array-index-expr-type
    item = arr[len];                                // warning46, lenfrom builtin, non-ets1.2 file, arkts-array-index-expr-type
    item = arr[arr.length - 1];                     // warning47, warning only, arkts-array-index-expr-type
    let item = 'a/b/c'.split('/')[0];               // should warn, but split not parsed, base type unknown
}

const INDEX = 3;                                    // warning48, defined as int, arkts-numeric-semantic

class GlobalIndex {
    private useGlobalIndex(): void {
        const arr: number[] | undefined = []
        let item = arr[INDEX];
        let i = 1.1;                                // warning49, defined as number, arkts-array-index-expr-type
        item = arr[i];                              // warning50, warning only, arkts-array-index-expr-type
    }
}

class ArrayClass {
    public array = [1, 2, 3];
}

class TestParam {
    private a: ArrayClass | undefined = undefined;

    private createA(): ArrayClass {
        if (!this.a) {
            this.a = new ArrayClass();
        }
        return this.a;
    }

    private foo(): void {
        for (let i = 0; i< 10; i++) {               // warning51, defined as int, arkts-numeric-semantic
            this.goo(i);
        }
    }

    private goo(index: number) {                    // warning52, defined as int, arkts-array-index-expr-type - deleted with param
        let item = this.createA().array[index];
    }
}

class TestClosureIndex {
    private foo = (param: number): void => {
        let index = 0;                              // warning53, defined as int, arkts-numeric-semantic
        let goo = (): void => {
            let item1 = arr[param];                 // warning54, warning only, arkts-array-index-expr-type
            let item1 = arr[index];
        }
    }

    private callFoo(): void {
        this.foo(2.5);
    }

    goo(): void {
        let index = 2;                              // warning55, defined as int, arkts-numeric-semantic
        let funcA = (): void => {
            let item = arr[index];
        }
    }
}

class Assign2ArrayItem {
    foo(): void {
        const a = 1;                                // warning56, defined as int, arkts-numeric-semantic
        arr[a] = 2;
        let b = 3;                                  // warning57, defined as int, arkts-numeric-semantic
        arr[b++] = 4;
        let c = 5;                                  // warning58, defined as int, arkts-numeric-semantic
        arr[c+1] = 6;
    }
}

class IndexWithConstant {
    foo(): void {
        let item = arr[1.0];                // warning59, fix to 1, arkts-array-index-expr-type
        arr[2.00] = 3;                      // warning60, fix to 2, arkts-array-index-expr-type
        let a = 1.0;                        // warning61, defined as number, arkts-array-index-expr-type
        item = arr[a];                      // warning62, warning only, arkts-array-index-expr-type
        arr[1.03];                          // warning63, warning only, arkts-array-index-expr-type
        arr[2.20] = 2;                      // warning64, warning only, arkts-array-index-expr-type
    }
}

class NumberInName {
    foo(): void {
        let numberA: number = 1;            // warning65, defined as int, arkts-numeric-semantic
        let numberB = 2;                    // warning66, defined as int, arkts-numeric-semantic
        let numberC = 3.1;                  // warning67, defined as number, arkts-array-index-expr-type
        arr[numberA];
        arr[numberB];
        arr[numberC];                       // warning68, warning only, arkts-array-index-expr-type
    }
}

interface Inter {
    name: string;
}

class PositiveCase {
    static foo: Function = (param: Inter): void => {
        let a: Inter = { name: param.name };   // no warning, IR previously misinterpreted param.name as ArkArrayRef
    }

    testArrayIndexWithInt(): void {
        const index: int = 10;
        let item = arr[index];
        const a: int = 20;
        const b: int = a;
        item = arr[b];
        let c: int = 1 + 2;
        item = arr[~c];
        let d: int = c;
        item = arr[-d];
    }
}

class ParameterAsIndex {
    foo(a: any, b: any) {}
    bar(buzz: Array<any> = []): void {
        buzz.forEach((icon, index) => {
            this.foo(icon, buzz[index]);        // deleted with param
        });
    }
}

class ArrayLength {
    foo(){
        let arr = [0, 1, 2, 3];
        for(let i = arr.length - 1; i >= 0; i--) {
            let item = arr[i];
        }
        let myArr: MyArray = [0, 1, 2];
        for(let i = myArr.length - 1; i >= 0; i--) {
            let item1 = myArr[i];
        }
    }
}

class MyArray<T> extends Array<T> {

}