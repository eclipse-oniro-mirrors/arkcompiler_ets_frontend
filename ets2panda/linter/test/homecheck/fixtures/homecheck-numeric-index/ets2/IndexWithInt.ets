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
    const index: number = 10;       // 告警1，定义为int，arkts-numeric-semantic
    let item = arr[index];
    item = arr[0];

    const a = 20;                   // 告警2，定义为int，arkts-numeric-semantic
    const b = a;                    // 告警3，定义为int，arkts-numeric-semantic
    item = arr[b];

    let c = 1 + 2;                  // 告警4，定义为int，arkts-numeric-semantic
    item = arr[~c];

    let d = c;                      // 告警5，定义为int，arkts-numeric-semantic
    item = arr[-d];
}

function testArrayIndexWithNumber(): void {
    const index: number = 10.1;
    let item = arr[index];          // 告警6，仅告警，arkts-array-index-expr-type
    const a = 20.2;                 // 告警7，定义为number，arkts-array-index-expr-type
    const b = a;                    // 告警8，定义为number，arkts-array-index-expr-type
    item = arr[b];                  // 告警9，仅告警，arkts-array-index-expr-type

    let c = 2;                      // 告警10，定义为number，arkts-numeric-semantic
    arr[c];                         // 告警11，仅告警，arkts-array-index-expr-type
    c = 3.3;
}

function testNoInfiniteLoop(): void {
    let a = export2IndexInt;        // 告警12，定义为number，export变量始终为number，arkts-numeric-semantic
    let b = a + 2;                  // 告警13，定义为number，arkts-numeric-semantic
    a = b + 3;
    let item = arr[a];              // 告警14，仅告警，arkts-array-index-expr-type
}

function testOtherTypeIndex(): void {
    const index: any = export2IndexInt;
    let item = arr[index];          // 告警15，仅告警，arkts-array-index-expr-type

    const a: number | string = 2;
    item = arr[a];                  // 告警16，仅告警，arkts-array-index-expr-type
}

const globalIndexInt = 3;           // 告警17，定义为int，arkts-numeric-semantic
const globalIndexNumber = 3.3;      // 告警18，定义为number，arkts-array-index-expr-type

function testIndexWithReturnValue(): void {
    let item = arr[export2FunctionInt(globalIndexInt)];
    item = arr[export2FunctionNumber()];            // 告警19，仅告警，arkts-array-index-expr-type
}

function testIndexWithArrayItem(): void {
    let indexes = [0, 1, 2];
    let item = arr[indexes[0]];                    // 告警20，不支持index来自于另一个数组，仅告警，arkts-array-index-expr-type
}

class A {
    b: number = 1;
    static c = 2;                                   // 告警21，定义为number，arkts-numeric-semantic
}

function testIndexWithFieldRef(): void {
    const a = new A();
    let item = arr[a.b];                    // 告警22，仅告警，a.b是public属性，认为找不全，arkts-array-index-expr-type
    intValue = arr[A.c];                    // 告警23，仅告警，A.c是public属性，认为找不全，arkts-array-index-expr-type
}

function indexWithParam(index: number): void {
    let item = arr[index];                  // 告警24，仅告警，因为跨函数层数超过limit 2层限制，arkts-array-index-expr-type - deleted with param
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

function outer1(a: number, b: string): void {        // 告警25，定义为int，arkts-array-index-expr-type
    function inner1() {
        let item = arr[a];
    }
}

function outer2(a: number, b: string): void {
    function inner2() {
        let item = arr[a];                          // 告警26，仅告警，arkts-array-index-expr-type
    }
}

function callOuter2WithNumber(): void {
    outer2(globalIndexNumber, 'abc');
}

function testLocalInRightOp(): void {
    let a = 1;                                      // 告警27，定义为int，arkts-numeric-semantic
    let item = arr[a++];

    let b = a * 1.1;                                // 告警28，定义为number，arkts-array-index-expr-type
    arr[b];                                         // 告警29，仅告警，arkts-array-index-expr-type
}

function bar(a: number): number {                   // 告警30，a类型可以转成int，arkts-array-index-expr-type
    return a * 1.2;
}

function testLocalInOtherStmt(): number {
    let a = 1;                                      // 告警31，定义为int，arkts-numeric-semantic
    let item = arr[a];
    let b = bar(a);                                 // 告警32，定义为number，arkts-array-index-expr-type
    item = arr[b];                                  // 告警33，仅告警，arkts-array-index-expr-type

    if (a > 0 && b < 0) {
        return -a;
    }
    return a;
}

function testImportFromNoETS2(): void {
    let index = export1IndexInt;                    // 告警34，定义为number，arkts-array-index-expr-type
    let item = arr[index];                          // 告警35，index来源于非ets1.2文件，仅告警，arkts-array-index-expr-type
    index = export1IndexNumber;
    item = arr[index];                              // 告警36，index来源于非ets1.2文件，仅告警，arkts-array-index-expr-type

    index = export1FunctionInt();
    item = arr[index];                              // 告警37，index来源于非ets1.2文件，仅告警，arkts-array-index-expr-type
    index = export1FunctionNumber();
    item = arr[index];                              // 告警38，index来源于非ets1.2文件，仅告警，arkts-array-index-expr-type

    let a = new Export1Class();
    index = a.goo();
    arr[index];                                     // 告警39，index来源于非ets1.2文件，仅告警，arkts-array-index-expr-type
    index = A.foo();
    arr[index];                                     // 告警40，index来源于非ets1.2文件，仅告警，arkts-array-index-expr-type
}

export function calledByEts2(a: number): void {     // 告警41，可以转成int - deleted with param
    let item = arr[a];
}

export function calledByEts1(b: number): void {
    let item = arr[b];                              // 告警42，index来源于非ets1.2文件 - deleted with param
}

function otherCases(): void {
    let targetArray = ['a', 'b', 'c'];
    let index = targetArray.indexOf('a');           // 告警43，定义为number，arkts-array-index-expr-type
    let item = arr[index];                          // 告警44，index来源于builtIn,非ets1.2文件，arkts-array-index-expr-type
    let len = targetArray.length;                   // 告警45，定义为number，arkts-array-index-expr-type
    item = arr[len];                                // 告警46，len来源于builtIn,非ets1.2文件，arkts-array-index-expr-type
    item = arr[arr.length - 1];                     // 告警47，仅告警，arkts-array-index-expr-type
    let item = 'a/b/c'.split('/')[0];               // 应该告警，但split未正确解析，不知道base类型为arrayType
}

const INDEX = 3;                                    // 告警48，定义为int，arkts-numeric-semantic

class GlobalIndex {
    private useGlobalIndex(): void {
        const arr: number[] | undefined = []
        let item = arr[INDEX];
        let i = 1.1;                                // 告警49，定义为number，arkts-array-index-expr-type
        item = arr[i];                              // 告警50，仅告警，arkts-array-index-expr-type
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
        for (let i = 0; i< 10; i++) {               // 告警51，定义为int，arkts-numeric-semantic
            this.goo(i);
        }
    }

    private goo(index: number) {                    // 告警52，定义为int，arkts-array-index-expr-type - deleted with param
        let item = this.createA().array[index];
    }
}

class TestClosureIndex {
    private foo = (param: number): void => {
        let index = 0;                              // 告警53，定义为int，arkts-numeric-semantic
        let goo = (): void => {
            let item1 = arr[param];                 // 告警54，仅告警，arkts-array-index-expr-type
            let item1 = arr[index];
        }
    }

    private callFoo(): void {
        this.foo(2.5);
    }

    goo(): void {
        let index = 2;                              // 告警55，定义为int，arkts-numeric-semantic
        let funcA = (): void => {
            let item = arr[index];
        }
    }
}

class Assign2ArrayItem {
    foo(): void {
        const a = 1;                                // 告警56，定义为int，arkts-numeric-semantic
        arr[a] = 2;
        let b = 3;                                  // 告警57，定义为int，arkts-numeric-semantic
        arr[b++] = 4;
        let c = 5;                                  // 告警58，定义为int，arkts-numeric-semantic
        arr[c+1] = 6;
    }
}

class IndexWithConstant {
    foo(): void {
        let item = arr[1.0];                // 告警59，修复成1，arkts-array-index-expr-type
        arr[2.00] = 3;                      // 告警60，修复成2，arkts-array-index-expr-type
        let a = 1.0;                        // 告警61，定义成number，arkts-array-index-expr-type
        item = arr[a];                      // 告警62，仅告警，arkts-array-index-expr-type
        arr[1.03];                          // 告警63，仅告警，arkts-array-index-expr-type
        arr[2.20] = 2;                      // 告警64，仅告警，arkts-array-index-expr-type
    }
}

class NumberInName {
    foo(): void {
        let numberA: number = 1;            // 告警65，定义为int，arkts-numeric-semantic
        let numberB = 2;                    // 告警66，定义为int，arkts-numeric-semantic
        let numberC = 3.1;                  // 告警67，定义成number，arkts-array-index-expr-type
        arr[numberA];
        arr[numberB];
        arr[numberC];                       // 告警68，仅告警，arkts-array-index-expr-type
    }
}

interface Inter {
    name: string;
}

class PositiveCase {
    static foo: Function = (param: Inter): void => {
        let a: Inter = { name: param.name };   // 无告警，原先IR误将param.name表示成ArkArrayRef
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