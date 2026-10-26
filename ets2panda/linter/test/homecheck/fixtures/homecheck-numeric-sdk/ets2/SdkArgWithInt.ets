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

import { parseInt, getArg, parseLong, parseIntOrLong, testAsyncNS } from '@ohos.int';

function testSdkWithIntParamIntArg(): void {
    const radix: number = 10;       // warning1, defined as int, sdk-api-num2int
    parseInt('123', radix);
    parseInt('123', 2);

    const a = 20;                   // warning2, defined as int, sdk-api-num2int
    const b = a;                    // warning3, defined as int, sdk-api-num2int
    parseInt('123', b);

    let c = 1 + 2;                  // warning4, defined as int, sdk-api-num2int
    parseInt('123', ~c);

    let d = c;                      // warning5, defined as int, sdk-api-num2int
    parseInt('123', -d);
}

function testSdkWithIntParamNumberArg(): void {
    const radix: number = 10.0;
    parseInt('123', radix);         // warning6, needs cast, sdk-api-num2int
    const a = 20.2;
    const b = a;
    parseInt('123', b);             // warning7, needs cast, sdk-api-num2int

    let c = 2;                      // warning8, defined as number, arkts-numeric-semantic
    parseInt('123', c);             // warning9, needs cast, sdk-api-num2int
    c = 3.3;
}

function testNoInfiniteLoop(): void {
    let a = 10;                                 // warning10, defined as int, sdk-api-num2int
    let b = a + 2;                              // warning11, defined as int, sdk-api-num2int
    a = b + 3;
    parseInt('123', a);
}

function paramOtherTypeArg(): void {
    const radix: any = 10.1;
    parseInt('123', radix);                     // warning12, needs cast, sdk-api-num2int

    const a: number | string = 2;
    parseInt('123', a);                         // warning13, needs cast, sdk-api-num2int
}

function foo1(): number {                       // needs enhancement, change return type to int
    let d = 1;                                  // warning14, defined as int, arkts-numeric-semantic
    const a = 2;                                // warning15, defined as int, arkts-numeric-semantic
    if (a > 0) {
        return d;
    } else {
        return -d;
    }
}

function foo2(): number {
    let d = 1;                                  // warning16, defined as int, arkts-numeric-semantic
    if (d > 0) {
        return d;
    } else {
        return d + 0.3;
    }
}

function testSdkWithReturnValue(): void {
    parseInt('123', foo1());
    parseInt('123', foo2());                    // warning17, needs cast, sdk-api-num2int
}

function testSdkWithArrayItem(): void {
    let arr = [1, 2, 3];
    parseInt('123', arr[0]);                    // warning18, needs cast, not continuing to check arrayRef, sdk-api-num2int
}

class A {
    b: number = 1;
    static c = 2;                               // warning19, defined as number, arkts-numeric-semantic
}

function testSdkWithFieldRef(): void {
    const a = new A();
    parseInt('123', a.b);                       // warning20, , needs cast, not continuing to check public property, sdk-api-num2int
    parseInt('123', A.c);                       // warning21, , needs cast, not continuing to check public property, sdk-api-num2int
}

function goo1(): void {
    const a = 10;                               // warning22, defined as int, arkts-numeric-semantic
    testSdkWithParamRefInt(a, 'abc');
}

function goo2(): void {
    const b = 1;                                // warning23, defined as int, arkts-numeric-semantic
    testSdkWithParamRefInt(b, 'abc');
}

function testSdkWithParamRefInt(a: number, b: string): void { // warning24, defined as int, sdk-api-num2int
    parseInt('123', a);
    const c = a;                                // warning25, defined as int, sdk-api-num2int
    parseInt('123', c);
}

function goo3(): void {
    testSdkWithParamRefNumber('abc', 1.2);
}

function testSdkWithParamRefNumber(a: string, b: number): void {
    parseInt('123', b);                         // warning26, needs cast, sdk-api-num2int
}

function hoo(a: number): void {
    parseInt('123', a);                         // warning27, needs cast, cross-function depth exceeds limit 2, sdk-api-num2int
}

function callDepthOne(a: number): void {
    hoo(a);
}

function callDepthTwo(b: number): void {
    callDepthOne(b);
}

function callDepthThree(c: number): void {
    callDepthTwo(c);
}

function outer1(a: number, b: string): void {        // warning8, defined as int, sdk-api-num2int
    function inner1() {
        parseInt('123', a);
    }
}

function outer2(a: number, b: string): void {
    function inner2() {
        parseInt('123', a);                         // warning29, needs cast, passes 1.2 when calling outer2, sdk-api-num2int
    }
}

function callOuter2WithNumber(): void {
    outer2(1.2, 'abc');
}

function testLocalInRightOp(): void {
    let a = 1;                                      // warning30, defined as int, sdk-api-num2int
    a++;
    parseInt('123', a);

    let b = a * 1.1;
    parseInt('123', b);                             // warning31, needs cast, sdk-api-num2int
}

function bar(a: number): number {                   // warning32, a type defined as int, sdk-api-num2int
    return a * 1.2;
}

function testLocalInOtherStmt(): number {
    let a = 1;                                      // warning33, defined as int, sdk-api-num2int
    let b = bar(a);
    parseInt('123', a);

    parseInt('123', b);                             // warning34, needs cast, sdk-api-num2int

    if (a > 0 && b < 0) {
        return -a;
    }
    return a;
}

let globalArg: number | undefined = 3;              // warning35, defined as int | undefined, arkts-numeric-semantic

function testArgWithUnionType() {
    parseInt('123', globalArg);
    let arg1: number | null = 4;                    // warning36, defined as int | null, sdk-api-num2int
    parseInt('123', arg1);
    const arg2: number | string = 5;
    parseInt('123', arg2);                          // warning37, needs cast, sdk-api-num2int
}

function testArgWithSDKReturnValue() {
    let arg = getArg();                             // warning38, defined as int, sdk-api-num2int
    parseInt('123', arg);
}

const array = [1, 2, 3];




class TestFieldRef {
    private a = 1;                                  // warning39, defined as int, arkts-numeric-semantic
    public b = 4;                                   // warning40, defined as number, arkts-numeric-semantic
    private c: string | number = 5;
    readonly private d: number | null;              // warning41, defined as int, sdk-api-num2int
    protected e = 6;                                // warning42, defined as number, arkts-numeric-semantic
    private f: number;
    private g: number | undefined = undefined;      // warning43, defined as int, sdk-api-num2int
    private h: number = 3;                          // warning44, defined as int, arkts-numeric-semantic
    private i = 3;                                  // warning45, defined as int, arkts-numeric-semantic
    private j = 4;                                  // warning46, defined as number, arkts-numeric-semantic

    constructor() {
        this.d = 6;
    }

    set setF(f: number) {
        this.f = f;
    }

    foo(): void {
        parseInt('123', this.a);                    // declared as int at definition
        parseInt('123', this.b);                    // warning47, b is public, cannot fully resolve, needs cast, sdk-api-num2int
        parseInt('123', array.length);              // warning48, interacts with builtin API, sdk-api-num2int
        parseInt('123', A.c);                       // warning49, field from another class, cannot fully resolve, needs cast, sdk-api-num2int
        parseInt('123', this.c);                    // warning50, not fully used as int, needs cast, sdk-api-num2int
        parseInt('123', this.d);                    // declared as int at definition
        parseInt('123', this.e);                    // warning51, e is protected, cannot fully resolve, needs cast, sdk-api-num2int
        parseInt('123', this.f);                    // warning52, f has setter, cannot fully resolve, needs cast, sdk-api-num2int
        parseInt('123', this.g);                    // declared as int at definition
        parseInt('123', this.h);                    // declared as int at definition
        parseInt('123', this.i);                    // declared as int at definition
        parseInt('123', this.j);                    // warning53, j used as float, needs cast, sdk-api-num2int
    }

    useField(): void {
        this.g = getArg();
        let a = this.h;
        let b = this.fieldAsParam(this.i);
        this.j = this.fieldAsReturn();
    }

    fieldAsParam(i: number): void {
        let a = i;
    }

    fieldAsReturn(): number {
        return 1.1;
    }
}

import { export2IndexInt, export2IndexNumber, export2FunctionInt, export2FunctionNumber } from './ExportETS2';
import { export1IndexInt, export1IndexNumber, export1FunctionInt, export1FunctionNumber, Export1Class } from '../ets1/ExportETS1';

function testImportFromNoETS2(): void {
    let arg1 = export1IndexInt;
    parseInt('123', arg1);              // warning54, index from non-ets1.2 file, needs cast, sdk-api-num2int
    let arg2 = export1IndexNumber;
    parseInt('123', arg2);              // warning55, index from non-ets1.2 file, needs cast, sdk-api-num2int

    let arg3 = export1FunctionInt();
    parseInt('123', arg3);              // warning56, index from non-ets1.2 file, needs cast, sdk-api-num2int
    let arg4 = export1FunctionNumber();
    parseInt('123', arg4);              // warning57, index from non-ets1.2 file, needs cast, sdk-api-num2int

    let a = new Export1Class();
    let arg5 = a.goo();
    parseInt('123', arg5);              // warning58, index from non-ets1.2 file, needs cast, sdk-api-num2int
    let arg6 = Export1Class.foo();
    parseInt('123', arg6);              // warning59, index from non-ets1.2 file, needs cast, sdk-api-num2int
}

export function calledByEts2(a: number): void {     // warning60, defined as int, sdk-api-num2int
    parseInt('123', a);
}

export function calledByEts1(b: number): void {
    parseInt('123', b);                 // warning61, index from non-ets1.2 file, needs cast, sdk-api-num2int
}

function testImportFromETS2(): void {
    let arg1 = export2IndexInt;
    parseInt('123', arg1);                          // warning62, assigned from export var, must be number, needs cast, sdk-api-num2int
    let arg2 = export2IndexNumber;
    parseInt('123', arg2);                          // warning63, assigned from number type, needs cast, sdk-api-num2int

    let arg3 = export2FunctionInt(123);             // warning64, defined as int, sdk-api-num2int
    parseInt('123', arg3);
    let arg4 = export2FunctionNumber();
    parseInt('123', arg4);                          // warning65, assigned from number type, needs cast, sdk-api-num2int
}

function testDivision(): void {
    const a = 2;                                    // warning66, involved in division, defined as number, arkts-numeric-semantic
    const b: number = 3;
    parseInt('123', a/b);                           // warning67, involved in division, needs cast, sdk-api-num2int

    const c = 2;                                    // warning68, involved in division, defined as number, arkts-numeric-semantic
    let d = 3/c;                                    // warning69-70, involved in division, d defined as number, 3 change to 3.0, arkts-numeric-semantic
    parseInt('123', d);                             // warning71, involved in division, needs cast, sdk-api-num2int
}

function testFixWithLineBoundary(): void {
    let a;                                          // warning72, defined as int, sdk-api-num2int
    a = 1;
    parseInt('123', a);
    let b;
    b = 2;
    parseInt('123', b);                             // warning73, defined as int, sdk-api-num2int, warning at b definition, to test autofix line-end boundary, comment placed here
    let c
    c = 3;
    parseInt('123', c);                             // warning74, defined as int, sdk-api-num2int, warning at c definition, to test autofix line-end boundary, comment placed here
}

function addNumber(a: string, b: string): number {
    return 1.0;
}

function testArgCrossMultiLines(): void {           // warning75, cast to int, sdk-api-num2int, warning at next line argument, to test autofix line-end boundary, comment placed here
    parseInt('123', addNumber(
        'abc',
        'xyz'));
                                                    // warning76, cast to int, sdk-api-num2int, warning at next line argument, to test autofix line-end boundary, comment placed here
    parseInt('123', addNumber(
        'abc',
        'xyz')
    );
}

class TestSDKWithLongType {
    testSimpleSdk(): void {
        let a = 1;                                  // warning77, defined as long, sdk-api-num2int
        parseLong(a);
        let b: number = 2;                          // warning78, defined as long, sdk-api-num2int
        parseLong(b);
    }

    testBothIntAndLongArg(): void {
        let c = 3;                                  // warning79, defined as long, sdk-api-num2int
        parseInt('abc', c);
        parseLong(c);
        let d: number = 4;                          // warning80, defined as long, sdk-api-num2int
        parseLong(d);
        parseInt('abc', d);
    }

    testBothIntAndLongSdk(): void {
        let e = 5;                                  // warning81, SDK has both int and long interfaces, upward compatible, defined as long, sdk-api-num2int
        parseIntOrLong(e);
    }
}

class OtherCase {
    testLiteralAsArg(): void {
        parseLong(1);
        parseLong(1.0);                             // warning82, cast to long, sdk-api-num2int
        parseInt('abc', 1);
        parseInt('abc', 1.1);                       // warning83, cast to int, sdk-api-num2int
    }

    testInt2Number(): void {
        let a = 1;                                  // warning84, defined as int, sdk-api-num2int
        let b = a;                                  // warning85, defined as number, int can be assigned to number, arkts-numeric-semantic
        parseInt('abc', a);
        b = 2.2;
        let c = 2;                                  // warning86, defined as long, sdk-api-num2int
        let d = c;                                  // warning87, defined as number, long can be assigned to number, arkts-numeric-semantic
        parseLong(c);
        d = 3.3;
    }
}

enum TestEnum {
    A1 = 1,
    A2 = 2,
    A3 = 3
}

class TestEnumArrayTuple {
    accessEnum(): void {
        parseInt('abc', TestEnum.A1);               // warning88, cast to int, enum cannot be redefined as int, sdk-api-num2int
    }
}

type int = number;
type long = number;

class PositiveCase {
    testDefineWithInt(): void {
        const radix: int = 10;
        let intValue = parseInt('123', radix);
        const a: int = 20;
        const b: int = a;
        intValue = parseInt('123', b);
    }

    testDefineWithLong(): void {
        let a: long = 1;
        parseLong(a);
    }
}

class TestFieldRefMore {
    private a = 1;                                  // warning89, defined as int, arkts-numeric-semantic
    private b = 2;                                  // warning90, defined as int, arkts-numeric-semantic
    private c = 3;                                  // warning91, defined as int, arkts-numeric-semantic
    private d = 4;                                  // warning92, defined as int, arkts-numeric-semantic

    foo(): void {
        parseInt('123', this.a);     // declared as int at definition
        let bb = this.b                             // warning93, defined as int, sdk-api-num2int
        parseInt('123', bb);
    }

    fieldUsedInCondition(): void {
        if (this.c > 0) {
            parseInt('123', this.c); // declared as int at definition
        }
    }

    fieldUsedInBinaryExpr(): void {
        parseInt('123', this.d);     // declared as int at definition
        console.log('abc' + this.d);
        if (this.c > 0 || this.d > 0) {
            return;
        }
    }
}

class TestAwaitExpr {
    async foo(): Promise<number> {
        return 3;                   // warning, 3 change to 3.0, arkts-numeric-semantic
    }

    async callAsyncMethod(): Promise<void> {
        let a = await this.foo();
        parseInt('123', a);         // warning94, needs cast, sdk-api-num2int
    }

    async callAsyncSDKReturnInt(): Promise<void> {
        let b = await testAsyncNS.TestAsync.getArgAsyncInt();
        parseInt('123', b);         // warning95, needs cast, sdk-api-num2int
    }

    async callAsyncSDKReturnNumber(): Promise<void> {
        let c = await testAsyncNS.TestAsync.getArgAsyncNumber();
        parseInt('123', c);         // warning96, needs cast, sdk-api-num2int
    }
}

class TestCastExpr {
    foo(): number {
        let a: number | null = null;                // warning97, defined as int, arkts-numeric-semantic
        a = 3;
        return a as number;
    }

    castInReturn(): void {
        let a = this.foo();                         // warning98, defined as int, sdk-api-num2int
        parseInt('123', a);
    }

    castInAssign(): void {
        let a = 1;                                  // warning99, defined as int, sdk-api-num2int
        let b: number = 2;                          // warning100, defined as int, sdk-api-num2int
        let c = a + b as number;                    // warning101, defined as int, sdk-api-num2int
        parseInt('123', c);
    }
}

interface Obj {
    aa: number;
    bb: number;
}

class TestObj {
    createInstance(): void {
        let obj: Obj = {
            aa: 1,
            bb: 2,
        }

        parseInt('123', obj.bb);                    // warning102, needs cast, sdk-api-num2int
    }

    objectUseLocal(): void {
        let a = 1;                                  // warning103, defined as int, arkts-numeric-semantic
        let obj: Obj = {
            aa: parseInt('123', a),
            bb: 2,
        };

        function inner(): void {
            let b = 3;                              // warning104, defined as int, arkts-numeric-semantic
            let obj: Obj = {
                aa: 4,
                bb: parseInt('123', b),
            };
        }
    }

    multipleObjUseLocal(): void {
        let outerA = 1;                             // warning105, defined as int, arkts-numeric-semantic
        let multiObj: OuterObj = {
            outer: 'outer',
            obj: {
                aa: parseInt('123', outerA),
                bb: 2,
            }
        }
    }
}

interface OuterObj {
    outer: string;
    obj: Obj;
}

class Positive {
    testSdkWithIntParamIntArg(): void {
        const radix: int = 10;
        parseInt('123', radix);
        const a: int = 20;
        const b: int = a;
        parseInt('123', b);

        let c: int = 1 + 2;
        parseInt('123', ~c);

        let d: int = c;
        parseInt('123', -d);
    }
}