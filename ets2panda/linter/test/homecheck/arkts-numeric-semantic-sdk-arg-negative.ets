/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

import { parseInt, getArg, parseLong, parseIntOrLong, testAsyncNS } from '@ohos.int';

function testSdkWithIntParamIntArg(): void {
    const radix: number = 10; // sdk-api-num2int
    parseInt('123', radix);
    parseInt('123', 2);

    const a = 20; // sdk-api-num2int
    const b = a; // sdk-api-num2int
    parseInt('123', b);

    let c = 1 + 2; // sdk-api-num2int
    parseInt('123', ~c);

    let d = c; // sdk-api-num2int
    parseInt('123', -d);
}

function testSdkWithIntParamNumberArg(): void {
    const radix: number = 10.0;
    parseInt('123', radix); // sdk-api-num2int
    const a = 20.2;
    const b = a;
    parseInt('123', b); // sdk-api-num2int

    let c = 2; // arkts-numeric-semantic
    parseInt('123', c); // sdk-api-num2int
    c = 3.3;
}

function testNoInfiniteLoop(): void {
    let a = 10; // sdk-api-num2int
    let b = a + 2; // sdk-api-num2int
    a = b + 3;
    parseInt('123', a);
}

function paramOtherTypeArg(): void {
    const radix: any = 10.1;
    parseInt('123', radix); // sdk-api-num2int

    const a: number | string = 2;
    parseInt('123', a); // sdk-api-num2int
}

function foo1(): number {
    let d = 1; // arkts-numeric-semantic
    const a = 2; // arkts-numeric-semantic
    if (a > 0) {
        return d;
    } else {
        return -d;
    }
}

function foo2(): number {
    let d = 1; // arkts-numeric-semantic
    if (d > 0) {
        return d;
    } else {
        return d + 0.3;
    }
}

function testSdkWithReturnValue(): void {
    parseInt('123', foo1());
    parseInt('123', foo2()); // sdk-api-num2int
}

function testSdkWithArrayItem(): void {
    let arr = [1, 2, 3];
    parseInt('123', arr[0]); // sdk-api-num2int
}

class A {
    b: number = 1;
    static c = 2; // arkts-numeric-semantic
}

function testSdkWithFieldRef(): void {
    const a = new A();
    parseInt('123', a.b); // sdk-api-num2int
    parseInt('123', A.c); // sdk-api-num2int
}

function testSdkWithParamRefInt(a: number, b: string): void { // sdk-api-num2int
    parseInt('123', a);
    const c = a; // sdk-api-num2int
    parseInt('123', c);
}

function testSdkWithParamRefNumber(a: string, b: number): void {
    parseInt('123', b); // sdk-api-num2int
}

function testLocalInRightOp(): void {
    let a = 1; // sdk-api-num2int
    a++;
    parseInt('123', a);

    let b = a * 1.1;
    parseInt('123', b); // sdk-api-num2int
}

function bar(a: number): number { // sdk-api-num2int
    return a * 1.2;
}

function testLocalInOtherStmt(): number {
    let a = 1; // sdk-api-num2int
    let b = bar(a);
    parseInt('123', a);

    parseInt('123', b); // sdk-api-num2int

    if (a > 0 && b < 0) {
        return -a;
    }
    return a;
}

let globalArg: number | undefined = 3; // arkts-numeric-semantic

function testArgWithUnionType() {
    parseInt('123', globalArg);
    let arg1: number | null = 4; // sdk-api-num2int
    parseInt('123', arg1);
    const arg2: number | string = 5;
    parseInt('123', arg2); // sdk-api-num2int
}

function testArgWithSDKReturnValue() {
    let arg = getArg(); // sdk-api-num2int
    parseInt('123', arg);
}

const array = [1, 2, 3];

class TestFieldRef {
    private a = 1; // arkts-numeric-semantic
    public b = 4; // arkts-numeric-semantic
    private c: string | number = 5;
    readonly private d: number | null; // sdk-api-num2int
    protected e = 6; // arkts-numeric-semantic
    private f: number;
    private g: number | undefined = undefined; // sdk-api-num2int
    private h: number = 3; // arkts-numeric-semantic
    private i = 3; // arkts-numeric-semantic
    private j = 4; // arkts-numeric-semantic

    constructor() {
        this.d = 6;
    }

    set setF(f: number) {
        this.f = f;
    }

    foo(): void {
        parseInt('123', this.a);
        parseInt('123', this.b); // sdk-api-num2int
        parseInt('123', array.length); // sdk-api-num2int
        parseInt('123', A.c); // sdk-api-num2int
        parseInt('123', this.c); // sdk-api-num2int
        parseInt('123', this.d);
        parseInt('123', this.e); // sdk-api-num2int
        parseInt('123', this.f); // sdk-api-num2int
        parseInt('123', this.g);
        parseInt('123', this.h);
        parseInt('123', this.i);
        parseInt('123', this.j); // sdk-api-num2int
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

class TestSDKWithLongType {
    testSimpleSdk(): void {
        let a = 1; // sdk-api-num2int
        parseLong(a);
        let b: number = 2; // sdk-api-num2int
        parseLong(b);
    }

    testBothIntAndLongArg(): void {
        let c = 3; // sdk-api-num2int
        parseInt('abc', c);
        parseLong(c);
        let d: number = 4; // sdk-api-num2int
        parseLong(d);
        parseInt('abc', d);
    }

    testBothIntAndLongSdk(): void {
        let e = 5; // sdk-api-num2int
        parseIntOrLong(e);
    }
}

class OtherCase {
    testLiteralAsArg(): void {
        parseLong(1);
        parseLong(1.0); // sdk-api-num2int
        parseInt('abc', 1);
        parseInt('abc', 1.1); // sdk-api-num2int
    }

    testInt2Number(): void {
        let a = 1; // sdk-api-num2int
        let b = a; // arkts-numeric-semantic
        parseInt('abc', a);
        b = 2.2;
        let c = 2; // sdk-api-num2int
        let d = c; // arkts-numeric-semantic
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
        parseInt('abc', TestEnum.A1); // sdk-api-num2int
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
    private a = 1; // arkts-numeric-semantic
    private b = 2; // arkts-numeric-semantic
    private c = 3; // arkts-numeric-semantic
    private d = 4; // arkts-numeric-semantic

    foo(): void {
        parseInt('123', this.a);
        let bb = this.b // sdk-api-num2int
        parseInt('123', bb);
    }

    fieldUsedInCondition(): void {
        if (this.c > 0) {
            parseInt('123', this.c);
        }
    }

    fieldUsedInBinaryExpr(): void {
        parseInt('123', this.d);
        console.log('abc' + this.d);
        if (this.c > 0 || this.d > 0) {
            return;
        }
    }
}

class TestAwaitExpr {
    async foo(): Promise<number> {
        return 3; // arkts-numeric-semantic
    }

    async callAsyncMethod(): Promise<void> {
        let a = await this.foo();
        parseInt('123', a); // sdk-api-num2int
    }

    async callAsyncSDKReturnInt(): Promise<void> {
        let b = await testAsyncNS.TestAsync.getArgAsyncInt();
        parseInt('123', b); // sdk-api-num2int
    }

    async callAsyncSDKReturnNumber(): Promise<void> {
        let c = await testAsyncNS.TestAsync.getArgAsyncNumber();
        parseInt('123', c); // sdk-api-num2int
    }
}

class TestCastExpr {
    foo(): number {
        let a: number | null = null; // arkts-numeric-semantic
        a = 3;
        return a as number;
    }

    castInReturn(): void {
        let a = this.foo(); // sdk-api-num2int
        parseInt('123', a);
    }

    castInAssign(): void {
        let a = 1; // sdk-api-num2int
        let b: number = 2; // sdk-api-num2int
        let c = a + b as number; // sdk-api-num2int
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

        parseInt('123', obj.bb); // sdk-api-num2int
    }

    objectUseLocal(): void {
        let a = 1; // arkts-numeric-semantic
        let obj: Obj = {
            aa: parseInt('123', a),
            bb: 2,
        };

        function inner(): void {
            let b = 3; // arkts-numeric-semantic
            let obj: Obj = {
                aa: 4,
                bb: parseInt('123', b),
            };
        }
    }
}

function testFixWithLineBoundary(): void {
    let a; // sdk-api-num2int
    a = 1;
    parseInt('123', a);
    let b; // sdk-api-num2int
    b = 2;
    parseInt('123', b);
    let c // sdk-api-num2int
    c = 3;
    parseInt('123', c);
}

function addNumber(a: string, b: string): number {
    return 1.0;
}

function testArgCrossMultiLines(): void { // sdk-api-num2int
    parseInt('123', addNumber(
        'abc',
        'xyz'));
                                                // sdk-api-num2int
    parseInt('123', addNumber(
        'abc',
        'xyz')
    );
}
