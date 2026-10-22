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
    const radix: number = 10;       // 告警1，定义为int，sdk-api-num2int
    parseInt('123', radix);
    parseInt('123', 2);

    const a = 20;                   // 告警2，定义为int，sdk-api-num2int
    const b = a;                    // 告警3，定义为int，sdk-api-num2int
    parseInt('123', b);

    let c = 1 + 2;                  // 告警4，定义为int，sdk-api-num2int
    parseInt('123', ~c);

    let d = c;                      // 告警5，定义为int，sdk-api-num2int
    parseInt('123', -d);
}

function testSdkWithIntParamNumberArg(): void {
    const radix: number = 10.0;
    parseInt('123', radix);         // 告警6，需要强转，sdk-api-num2int
    const a = 20.2;
    const b = a;
    parseInt('123', b);             // 告警7，需要强转，sdk-api-num2int

    let c = 2;                      // 告警8，定义为number，arkts-numeric-semantic
    parseInt('123', c);             // 告警9，需要强转，sdk-api-num2int
    c = 3.3;
}

function testNoInfiniteLoop(): void {
    let a = 10;                                 // 告警10，定义为int，sdk-api-num2int
    let b = a + 2;                              // 告警11，定义为int，sdk-api-num2int
    a = b + 3;
    parseInt('123', a);
}

function paramOtherTypeArg(): void {
    const radix: any = 10.1;
    parseInt('123', radix);                     // 告警12，需要强转，sdk-api-num2int

    const a: number | string = 2;
    parseInt('123', a);                         // 告警13，需要强转，sdk-api-num2int
}

function foo1(): number {                       // 需要增强，将返回值改为int
    let d = 1;                                  // 告警14，定义为int，arkts-numeric-semantic
    const a = 2;                                // 告警15，定义为int，arkts-numeric-semantic
    if (a > 0) {
        return d;
    } else {
        return -d;
    }
}

function foo2(): number {
    let d = 1;                                  // 告警16，定义为int，arkts-numeric-semantic
    if (d > 0) {
        return d;
    } else {
        return d + 0.3;
    }
}

function testSdkWithReturnValue(): void {
    parseInt('123', foo1());
    parseInt('123', foo2());                    // 告警17，需要强转，sdk-api-num2int
}

function testSdkWithArrayItem(): void {
    let arr = [1, 2, 3];
    parseInt('123', arr[0]);                    // 告警18，需要强转，不继续检查arrayRef，sdk-api-num2int
}

class A {
    b: number = 1;
    static c = 2;                               // 告警19，定义为number，arkts-numeric-semantic
}

function testSdkWithFieldRef(): void {
    const a = new A();
    parseInt('123', a.b);                       // 告警20，，需要强转，不继续检查public属性，sdk-api-num2int
    parseInt('123', A.c);                       // 告警21，，需要强转，不继续检查public属性，sdk-api-num2int
}

function goo1(): void {
    const a = 10;                               // 告警22，定义为int，arkts-numeric-semantic
    testSdkWithParamRefInt(a, 'abc');
}

function goo2(): void {
    const b = 1;                                // 告警23，定义为int，arkts-numeric-semantic
    testSdkWithParamRefInt(b, 'abc');
}

function testSdkWithParamRefInt(a: number, b: string): void { // 告警24，定义为int，sdk-api-num2int
    parseInt('123', a);
    const c = a;                                // 告警25，定义为int，sdk-api-num2int
    parseInt('123', c);
}

function goo3(): void {
    testSdkWithParamRefNumber('abc', 1.2);
}

function testSdkWithParamRefNumber(a: string, b: number): void {
    parseInt('123', b);                         // 告警26，需要强转，sdk-api-num2int
}

function hoo(a: number): void {
    parseInt('123', a);                         // 告警27，需要强转，因为跨函数层数超过limit 2层限制，sdk-api-num2int
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

function outer1(a: number, b: string): void {        // 告警8，定义为int，sdk-api-num2int
    function inner1() {
        parseInt('123', a);
    }
}

function outer2(a: number, b: string): void {
    function inner2() {
        parseInt('123', a);                         // 告警29，需要强转，调用outer2时传入1.2，sdk-api-num2int
    }
}

function callOuter2WithNumber(): void {
    outer2(1.2, 'abc');
}

function testLocalInRightOp(): void {
    let a = 1;                                      // 告警30，定义为int，sdk-api-num2int
    a++;
    parseInt('123', a);

    let b = a * 1.1;
    parseInt('123', b);                             // 告警31，需要强转，sdk-api-num2int
}

function bar(a: number): number {                   // 告警32，a类型定义为int，sdk-api-num2int
    return a * 1.2;
}

function testLocalInOtherStmt(): number {
    let a = 1;                                      // 告警33，定义为int，sdk-api-num2int
    let b = bar(a);
    parseInt('123', a);

    parseInt('123', b);                             // 告警34，需要强转，sdk-api-num2int

    if (a > 0 && b < 0) {
        return -a;
    }
    return a;
}

let globalArg: number | undefined = 3;              // 告警35，定义为int | undefined，arkts-numeric-semantic

function testArgWithUnionType() {
    parseInt('123', globalArg);
    let arg1: number | null = 4;                    // 告警36，定义为int | null，sdk-api-num2int
    parseInt('123', arg1);
    const arg2: number | string = 5;
    parseInt('123', arg2);                          // 告警37，需要强转，sdk-api-num2int
}

function testArgWithSDKReturnValue() {
    let arg = getArg();                             // 告警38，定义为int，sdk-api-num2int
    parseInt('123', arg);
}

const array = [1, 2, 3];




class TestFieldRef {
    private a = 1;                                  // 告警39，定义为int，arkts-numeric-semantic
    public b = 4;                                   // 告警40，定义为number，arkts-numeric-semantic
    private c: string | number = 5;
    readonly private d: number | null;              // 告警41，定义为int，sdk-api-num2int
    protected e = 6;                                // 告警42，定义为number，arkts-numeric-semantic
    private f: number;
    private g: number | undefined = undefined;      // 告警43，定义为int，sdk-api-num2int
    private h: number = 3;                          // 告警44，定义为int，arkts-numeric-semantic
    private i = 3;                                  // 告警45，定义为int，arkts-numeric-semantic
    private j = 4;                                  // 告警46，定义为number，arkts-numeric-semantic

    constructor() {
        this.d = 6;
    }

    set setF(f: number) {
        this.f = f;
    }

    foo(): void {
        parseInt('123', this.a);                    // 在定义处声明int
        parseInt('123', this.b);                    // 告警47，b为public，无法找全，需要强转，sdk-api-num2int
        parseInt('123', array.length);              // 告警48，与builtIn接口交互，sdk-api-num2int
        parseInt('123', A.c);                       // 告警49，其他class的field，无法找全，需要强转，sdk-api-num2int
        parseInt('123', this.c);                    // 告警50，不全作为int使用，需要强转，sdk-api-num2int
        parseInt('123', this.d);                    // 在定义处声明int
        parseInt('123', this.e);                    // 告警51，e为protected，无法找全，需要强转，sdk-api-num2int
        parseInt('123', this.f);                    // 告警52，f有setter，无法找全，需要强转，sdk-api-num2int
        parseInt('123', this.g);                    // 在定义处声明int
        parseInt('123', this.h);                    // 在定义处声明int
        parseInt('123', this.i);                    // 在定义处声明int
        parseInt('123', this.j);                    // 告警53，j有作为浮点使用，需要强转，sdk-api-num2int
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
    parseInt('123', arg1);              // 告警54，index来源于非ets1.2文件，需要强转，sdk-api-num2int
    let arg2 = export1IndexNumber;
    parseInt('123', arg2);              // 告警55，index来源于非ets1.2文件，需要强转，sdk-api-num2int

    let arg3 = export1FunctionInt();
    parseInt('123', arg3);              // 告警56，index来源于非ets1.2文件，需要强转，sdk-api-num2int
    let arg4 = export1FunctionNumber();
    parseInt('123', arg4);              // 告警57，index来源于非ets1.2文件，需要强转，sdk-api-num2int

    let a = new Export1Class();
    let arg5 = a.goo();
    parseInt('123', arg5);              // 告警58，index来源于非ets1.2文件，需要强转，sdk-api-num2int
    let arg6 = Export1Class.foo();
    parseInt('123', arg6);              // 告警59，index来源于非ets1.2文件，需要强转，sdk-api-num2int
}

export function calledByEts2(a: number): void {     // 告警60，定义为int，sdk-api-num2int
    parseInt('123', a);
}

export function calledByEts1(b: number): void {
    parseInt('123', b);                 // 告警61，index来源于非ets1.2文件，需要强转，sdk-api-num2int
}

function testImportFromETS2(): void {
    let arg1 = export2IndexInt;
    parseInt('123', arg1);                          // 告警62，由export的变量赋值只能是number，需要强转，sdk-api-num2int
    let arg2 = export2IndexNumber;
    parseInt('123', arg2);                          // 告警63，由number类型赋值，需要强转，sdk-api-num2int

    let arg3 = export2FunctionInt(123);             // 告警64，定义为int，sdk-api-num2int
    parseInt('123', arg3);
    let arg4 = export2FunctionNumber();
    parseInt('123', arg4);                          // 告警65，由number类型赋值，需要强转，sdk-api-num2int
}

function testDivision(): void {
    const a = 2;                                    // 告警66，有参与除法运算，定义为number，arkts-numeric-semantic
    const b: number = 3;
    parseInt('123', a/b);                           // 告警67，有参与除法运算，需要强转，sdk-api-num2int

    const c = 2;                                    // 告警68，有参与除法运算，定义为number，arkts-numeric-semantic
    let d = 3/c;                                    // 告警69-70，有参与除法运算，d定义为number，3改为3.0，arkts-numeric-semantic
    parseInt('123', d);                             // 告警71，有参与除法运算，需要强转，sdk-api-num2int
}

function testFixWithLineBoundary(): void {
    let a;                                          // 告警72，定义为int，sdk-api-num2int
    a = 1;
    parseInt('123', a);
    let b;
    b = 2;
    parseInt('123', b);                             // 告警73，定义为int，sdk-api-num2int，告警实际在b的定义处，为了测试自动修复的行尾边界情况，将注释加在这里
    let c
    c = 3;
    parseInt('123', c);                             // 告警74，定义为int，sdk-api-num2int，告警实际在c的定义处，为了测试自动修复的行尾边界情况，将注释加在这里
}

function addNumber(a: string, b: string): number {
    return 1.0;
}

function testArgCrossMultiLines(): void {           // 告警75，强转int，sdk-api-num2int，告警实际在下一行入参处，为了测试自动修复的行尾边界情况，将注释加在这里
    parseInt('123', addNumber(
        'abc',
        'xyz'));
                                                    // 告警76，强转int，sdk-api-num2int，告警实际在下一行入参处，为了测试自动修复的行尾边界情况，将注释加在这里
    parseInt('123', addNumber(
        'abc',
        'xyz')
    );
}

class TestSDKWithLongType {
    testSimpleSdk(): void {
        let a = 1;                                  // 告警77，定义为long，sdk-api-num2int
        parseLong(a);
        let b: number = 2;                          // 告警78，定义为long，sdk-api-num2int
        parseLong(b);
    }

    testBothIntAndLongArg(): void {
        let c = 3;                                  // 告警79，定义为long，sdk-api-num2int
        parseInt('abc', c);
        parseLong(c);
        let d: number = 4;                          // 告警80，定义为long，sdk-api-num2int
        parseLong(d);
        parseInt('abc', d);
    }

    testBothIntAndLongSdk(): void {
        let e = 5;                                  // 告警81，该SDK同时存在int与long的接口，向上兼容定义为long，sdk-api-num2int
        parseIntOrLong(e);
    }
}

class OtherCase {
    testLiteralAsArg(): void {
        parseLong(1);
        parseLong(1.0);                             // 告警82，强转long，sdk-api-num2int
        parseInt('abc', 1);
        parseInt('abc', 1.1);                       // 告警83，强转int，sdk-api-num2int
    }

    testInt2Number(): void {
        let a = 1;                                  // 告警84，定义为int，sdk-api-num2int
        let b = a;                                  // 告警85，定义为number，int可以赋值给number，arkts-numeric-semantic
        parseInt('abc', a);
        b = 2.2;
        let c = 2;                                  // 告警86，定义为long，sdk-api-num2int
        let d = c;                                  // 告警87，定义为number，long可以赋值给number，arkts-numeric-semantic
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
        parseInt('abc', TestEnum.A1);               // 告警88，强转int，枚举类型不能修改定义为int，sdk-api-num2int
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
    private a = 1;                                  // 告警89，定义为int，arkts-numeric-semantic
    private b = 2;                                  // 告警90，定义为int，arkts-numeric-semantic
    private c = 3;                                  // 告警91，定义为int，arkts-numeric-semantic
    private d = 4;                                  // 告警92，定义为int，arkts-numeric-semantic

    foo(): void {
        parseInt('123', this.a);     // 在定义处声明int
        let bb = this.b                             // 告警93，定义为int，sdk-api-num2int
        parseInt('123', bb);
    }

    fieldUsedInCondition(): void {
        if (this.c > 0) {
            parseInt('123', this.c); // 在定义处声明int
        }
    }

    fieldUsedInBinaryExpr(): void {
        parseInt('123', this.d);     // 在定义处声明int
        console.log('abc' + this.d);
        if (this.c > 0 || this.d > 0) {
            return;
        }
    }
}

class TestAwaitExpr {
    async foo(): Promise<number> {
        return 3;                   // 告警，3改为3.0，arkts-numeric-semantic
    }

    async callAsyncMethod(): Promise<void> {
        let a = await this.foo();
        parseInt('123', a);         // 告警94，需要强转，sdk-api-num2int
    }

    async callAsyncSDKReturnInt(): Promise<void> {
        let b = await testAsyncNS.TestAsync.getArgAsyncInt();
        parseInt('123', b);         // 告警95，需要强转，sdk-api-num2int
    }

    async callAsyncSDKReturnNumber(): Promise<void> {
        let c = await testAsyncNS.TestAsync.getArgAsyncNumber();
        parseInt('123', c);         // 告警96，需要强转，sdk-api-num2int
    }
}

class TestCastExpr {
    foo(): number {
        let a: number | null = null;                // 告警97，定义为int，arkts-numeric-semantic
        a = 3;
        return a as number;
    }

    castInReturn(): void {
        let a = this.foo();                         // 告警98，定义为int，sdk-api-num2int
        parseInt('123', a);
    }

    castInAssign(): void {
        let a = 1;                                  // 告警99，定义为int，sdk-api-num2int
        let b: number = 2;                          // 告警100，定义为int，sdk-api-num2int
        let c = a + b as number;                    // 告警101，定义为int，sdk-api-num2int
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

        parseInt('123', obj.bb);                    // 告警102，需要强转，sdk-api-num2int
    }

    objectUseLocal(): void {
        let a = 1;                                  // 告警103，定义为int，arkts-numeric-semantic
        let obj: Obj = {
            aa: parseInt('123', a),
            bb: 2,
        };

        function inner(): void {
            let b = 3;                              // 告警104，定义为int，arkts-numeric-semantic
            let obj: Obj = {
                aa: 4,
                bb: parseInt('123', b),
            };
        }
    }

    multipleObjUseLocal(): void {
        let outerA = 1;                             // 告警105，定义为int，arkts-numeric-semantic
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