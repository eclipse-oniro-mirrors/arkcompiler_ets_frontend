/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS' BASIS,",
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.",
 * See the License for the specific language governing permissions and",
 * limitations under the License."
 */

import { parseInt } from '@ohos.int';

let har_val = 2;

class RightOpHasIntLiteral {
    assignWithIntLiteral(): void {
        let a = 1; // arkts-numeric-semantic
        let b: number = a; // arkts-numeric-semantic
        let c: number = 2; // arkts-numeric-semantic
        a = c;
        let d = b; // arkts-numeric-semantic
    }

    assignWithNumericLiteral(): void {
        let a = 1; // arkts-numeric-semantic
        let b: number = a;
        let c: null | number = 2;
        c = 3.0
        a = c;
    }

    assignWithNegIntLiteral(): void {
        let a = -3; // arkts-numeric-semantic
        let b: number | undefined = a; // arkts-numeric-semantic
        let c = b; // arkts-numeric-semantic
    }

    assignWithNegNumericLiteral(): void {
        let a = -3; // arkts-numeric-semantic
        let b = a;
        a = 3.1
    }

    assignWithDivision(): void {
        let a = 1; // arkts-numeric-semantic
        let b = 2/a; // arkts-numeric-semantic
        let c = 3; // arkts-numeric-semantic
        let d = c/4.1; // arkts-numeric-semantic
        let e = b;
        let f = 1/2; // arkts-numeric-semantic
    }

    assignWithBinaryOperator(): void {
        let a = 1 + 2; // arkts-numeric-semantic
        let b = 3 - a; // arkts-numeric-semantic
        let c = 4 * 5.0; // arkts-numeric-semantic
        let d = 6 ** c; // arkts-numeric-semantic
        let e = d ?? 7; // arkts-numeric-semantic
    }
}

export let global1 = 1; // arkts-numeric-semantic
export let global2 = 2; // arkts-numeric-semantic

function testExportLocal(): void {
    let a = 3; // arkts-numeric-semantic
    a = global1;
    let b = 4; // arkts-numeric-semantic
    global2 = b;
}

export let global3 = 1; // arkts-numeric-semantic
let a3 = 1; // arkts-numeric-semantic
a3 = global3;

let global4 = 2; // arkts-numeric-semantic
let a4 = 4; // arkts-numeric-semantic
a4 = global4;

enum TestEnum {
    A1 = 1,
    A2 = 2,
    A3 = 3
}

class TestEnumArrayTuple {
    accessEnum(): void {
        let a = 1; // arkts-numeric-semantic
        a = TestEnum.A1;
    }

    static initArrayWithLocal(): void {
        let a = 1; // arkts-numeric-semantic
        let arr: number[] = [a];
        let b = 1/2; // arkts-numeric-semantic
        arr.push(b);
        let c = 1; // arkts-numeric-semantic
        let d = 2; // arkts-numeric-semantic
        let newArray = [1, c/d];
    }
}

class TestClassField {
    private a: number = 1; // arkts-numeric-semantic
    b = 1.2;
    static c = 2/3 + 6; // arkts-numeric-semantic
    d: number;
    e: number = 4;
    private f: number = 5;

    constructor(param: number) {
        this.d = param;
    }

    static accessField(): void {
        this.a = 3;
        let aa = 3; // arkts-numeric-semantic
        aa = this.a;
        let bb = 4; // arkts-numeric-semantic
        bb = this.b;
        let ff = 6; // arkts-numeric-semantic
        ff = this.f;
    }
}

interface Obj {
    aa: number;
    bb: number;
}

class TestObj {
    accessObjField(): void {
        let obj: Obj = {
            aa: 1,
            bb: 2/3, // arkts-numeric-semantic
        }
        let a = obj.aa;
        let b = obj.bb;
    }

    objectUseLocal(): void {
        let a = 1; // arkts-numeric-semantic
        let b = 2; // arkts-numeric-semantic
        let obj: Obj = {
            aa: a > 0 ? a : 1,
            bb: b < 0 ? 1 : b,
        };
        b = obj.aa;
    }
}

class TestPosition {
    foo(): void {
        let condition = true;
        let a = condition ? 1 : 2/3; // arkts-numeric-semantic
    }
}

class pass2ParamType {
    foo(param: number): number {
        return param;
    }

    goo(): void {
        let a = 1; // arkts-numeric-semantic
        this.foo(a);
    }
}

class MayCauseInfiniteLoop {
    foo(): void {
        let a = 10; // arkts-numeric-semantic
        a++;
        let b = 11; // arkts-numeric-semantic
        b = (b < 0) ? 0: b;
    }
}

class SameVarInDiffScopes {
    foo(): void {
        for (let i = 0; i < 10; i++) { // arkts-numeric-semantic
            console.log(i);
        }
        for (let i = 1; i < 20; i++) { // arkts-numeric-semantic
            console.log(i);
        }
        for (let i = 2; i < 30; i++) { // arkts-numeric-semantic
            console.log(i);
        }
    }

    goo(): void {
        if (true) {
            let i = 3; // arkts-numeric-semantic
        } else {
            let i = 3.3;
        }
    }
}

class FieldNumberTest {
    a = 1; // arkts-numeric-semantic
    static b = 2; // arkts-numeric-semantic
    readonly c = 3; // arkts-numeric-semantic
    private _d = 4; // arkts-numeric-semantic

    get d() {
        return this._d;
    }
}

class TestFieldNumberType {
    foo(): void {
        let aa = new FieldNumberTest();
        let tt = aa.a/FieldNumberTest.b; // arkts-numeric-semantic
    }
}

enum MyEnum {
    A = 1,
    B = 2,
    C = -1,
}

class TestDivision {
    test1(): void {
        let vo = 123; // arkts-numeric-semantic
        let d4 = MyEnum.A; // arkts-numeric-semantic
        let d5 = vo as number; // arkts-numeric-semantic
        let d6 = new FieldNumberTest().a; // arkts-numeric-semantic
        let d7 = FieldNumberTest.b; // arkts-numeric-semantic
        let importVal = har_val; // arkts-numeric-semantic
        let d10 = d4/2; // arkts-numeric-semantic
        d10 = d5/3; // arkts-numeric-semantic
        d10 = d6/4; // arkts-numeric-semantic
        d10 = d7/5; // arkts-numeric-semantic
        d10 = importVal/6.0;
    }

    test2(): void {
        let e1 = (1); // arkts-numeric-semantic
        let e3 = parseInt('42'); // arkts-numeric-semantic
        let e7 = e1/2; // arkts-numeric-semantic
        e7 = e3/2; // arkts-numeric-semantic
    }

    test3(): void {
        for (let i = 0; i < 10; i++) { // arkts-numeric-semantic
            let b = i/2; // arkts-numeric-semantic
        }
    }

    test4(): void {
        let k4 = new FieldNumberTest();
        let k5 = k4.a/2 // arkts-numeric-semantic
        let k6 = k4.d/3; // arkts-numeric-semantic
    }
}

let k1 = new FieldNumberTest();
let k2 = k1.d/3; // arkts-numeric-semantic

class TestOptionalProperty {
    private a?: number = 1; // arkts-numeric-semantic
    private b?: number = 1.0;
    public c: number;

    constructor(c?: number) { // arkts-numeric-semantic
        this.c = c === undefined ? 2 : c;
    }

    test(a?: number, b?: number): void { // arkts-numeric-semantic
        a = 1.0;
        b = 2;
    }
}

class EnumDivision {
    foo(): void {
        let a = MyEnum.A / MyEnum.B; // arkts-numeric-semantic
        let b = 2 / MyEnum.B; // arkts-numeric-semantic
        a = b / MyEnum.C; // arkts-numeric-semantic
    }
}

let a5 = 1; // arkts-numeric-semantic
let b5 = 1; // arkts-numeric-semantic
let tt = () => {
    let c5 = a5 / b5 // arkts-numeric-semantic
}

class PositiveCase {
    numericLiteral(): void {
        let a = 1.0;
        let b: number = a;
    }

    otherBinaryOperator(): void {
        let a = 1 || 2;
        let b = a && 3;
        let c = b & 4;
        let d = c | 5;
        let e = 6 ^ d;
        let f = e << 7;
        let g = f >> 8;
        let h = 9 >>> 10;
        let i = a % b;
    }

    numberParam(param: number): void {
        console.log(param);
    }

    callMethod(): void {
        this.numberParam(2);
    }

    annotationInt(): void {
        let a: int = 1;
        let b: int = a;
        let c: int = 2;
        a = c;
        let d: int = b;
    }
}

let l: number = 1.2e4;
let m: number = 12E-3;

class A {
    private a: number = 12e3;
    private b: number = 1.3E-3;
}

async function foo() {
    let finalValue: number = 0;
    await Promise.resolve<number>(1)
        .then((v: number) => new Promise<number>((resolve) => {
            setTimeout(() => resolve(v * 2), 10);
        }))
        .then((v: number) => {
            return new Promise<number>((resolve) => {
                setTimeout(() => {
                    Promise.resolve<number>(v + 3)
                        .then((v2: number) => {
                            let b = finalValue;
                            finalValue = v2 * 4;
                            resolve(v2);
                        });
                }, 20)
            })
        })
}

export let har_val = 2;
