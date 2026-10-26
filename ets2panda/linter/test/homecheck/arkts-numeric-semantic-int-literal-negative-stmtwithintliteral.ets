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

class RightOpHasIntLiteral {
    assignWithIntLiteral(): void {
        let a = 1;                      // warning1, defined as int, arkts-numeric-semantic
        let b: number = a;              // warning2, defined as int, arkts-numeric-semantic
        let c: number = 2;              // warning3, defined as int, arkts-numeric-semantic
        a = c;
        let d = b;                      // warning4, defined as int, arkts-numeric-semantic
    }

    assignWithNumericLiteral(): void {
        let a = 1;                      // warning5, defined as number, arkts-numeric-semantic
        let b: number = a;
        let c: null | number = 2;       // c has correct type annotation, no warning needed
        c = 3.0
        a = c;
    }

    assignWithNegIntLiteral(): void {
        let a = -3;                     // warning6, defined as int, arkts-numeric-semantic
        let b: number | undefined = a;  // warning7, defined as int, arkts-numeric-semantic
        let c = b;                      // warning8, defined as int, arkts-numeric-semantic
    }

    assignWithNegNumericLiteral(): void {
        let a = -3;                     // warning9, defined as number
        let b = a;                      // a is number, no need to check left variable
        a = 3.1
    }

    assignWithDivision(): void {
        let a = 1;                      // warning10, defined as number, involved in division, arkts-numeric-semantic
        let b = 2/a;                    // warning11-12, b defined as number, 2 change to 2.0, arkts-numeric-semantic
        let c = 3;                      // warning13, defined as number, involved in division, arkts-numeric-semantic
        let d = c/4.1;                  // warning14, defined as number, 4.1 is float literal, not actively checked
        let e = b;                      // b is number, no need to check left variable
        let f = 1/2;                    // warning15-17, f defined as number, 1 and 2 change to 1.0 and 2.0, arkts-numeric-semantic
    }

    assignWithBinaryOperator(): void {
        let a = 1 + 2;                  // warning18, defined as int, arkts-numeric-semantic
        let b = 3 - a;                  // warning19, defined as int, arkts-numeric-semantic
        let c = 4 * 5.0;                // warning20, defined as number, arkts-numeric-semantic
        let d = 6 ** c;                 // warning21, defined as number, arkts-numeric-semantic
        let e = d ?? 7;                 // warning22, defined as number, arkts-numeric-semantic
    }
}

export let global1 = 1;                 // warning23, defined as number, export variable scope unknown, arkts-numeric-semantic
export let global2 = 2;                 // warning24, defined as number, export variable scope unknown, arkts-numeric-semantic

function testExportLocal(): void {
    let a = 3;                          // warning25, defined as number, assigned from export variable, arkts-numeric-semantic
    a = global1;
    let b = 4;                          // warning26, defined as int, int variable can be assigned to export number, arkts-numeric-semantic
    global2 = b;
}

export let global3 = 1;                 // warning27, defined as number, export variable scope unknown, arkts-numeric-semantic
let a3 = 1;                             // warning28, defined as number, assigned from export variable, arkts-numeric-semantic
a3 = global3;

let global4 = 2;                        // warning29, defined as int, arkts-numeric-semantic
let a4 = 4;                             // warning30, defined as int, arkts-numeric-semantic
a4 = global4;

enum TestEnum {
    A1 = 1,
    A2 = 2,
    A3 = 3
}

class TestEnumArrayTuple {
    accessEnum(): void {
        let a = 1;                      // warning31, defined as number, has enum assignment, must be number, arkts-numeric-semantic
        a = TestEnum.A1;
    }

    initArray(): void {                 // no warning, check for following scenarios done in another rule
        let arr: number[] = [1, 2, 3];
        arr = [2, 3, 4];
        arr = [1/2, 2/3];
        arr[0] = 2/3;
        let arr1 = [0, ...arr, 10];
    }

    static initArrayWithLocal(): void {
        let a = 1;                      // warning32, defined as int, int can be assigned to number array element
        let arr: number[] = [a];
        let b = 1/2;                    // warning33-35, defined as number,1 and 2 change to 1.0 and 2.0
        arr.push(b);
        let c = 1;                      // warning36, defined as number, involved in division, arkts-numeric-semantic
        let d = 2;                      // warning37, defined as number, involved in division, arkts-numeric-semantic
        let newArray = [1, c/d];
    }

    initArrayOtherCase(): void {
        let arr = [1, 2, 3];            // no warning, check for following scenarios done in another rule
        let arr1 = arr ?? [0/1];
        let condition = true;
        let arr2 = condition ? arr : [0, 1/2];
    }

    initTuple(): void {
        let a: [number, number, boolean] = [1, 2, true];    // no warning, check for following scenarios done in another rule
        a = [2, 3, false];
        a = [1/2, 2/3, false];
        a[0] = 2/3;
        let b: [string, number, number, boolean] = ['abc', ...a];
    }
}

class TestClassField {                  // class fields not actively checked, except for 2/3 scenarios, or used by variables involving integer literals
    private a: number = 1;              // warning38, defined as int, arkts-numeric-semantic
    b = 1.2;
    static c = 2/3 + 6;                 // warning39-41, 2 and 3 change to 2.0 and 3.0, arkts-numeric-semantic
    d: number;
    e: number = 4;
    private f: number = 5;

    constructor(param: number) {
        this.d = param;
    }

    static accessField(): void {
        this.a = 3;
        let aa = 3;                     // warning42, defined as int, arkts-numeric-semantic
        aa = this.a;
        let bb = 4;                     // warning43, defined as number, this.b is public, not continuing to check, arkts-numeric-semantic
        bb = this.b;
        let ff = 6;                     // warning44, defined as number, arkts-numeric-semantic
        ff = this.f;
    }

    accessFieldMore(): void {
        this.a = 4;
        let aa = 3.2;
        aa = this.a;                    // does not affect this.a as int
        this.f = 6.6;                   // affects this.f, can only be used as number
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
            bb: 2/3,                    // warning45-46, needs fixchange to 2.0 and 3.0, arkts-numeric-semantic
        }
        let a = obj.aa;
        let b = obj.bb;
    }

    objectUseLocal(): void {
        let a = 1;                      // warning47, defined as int, arkts-numeric-semantic
        let b = 2;                      // warning48, defined as number, arkts-numeric-semantic
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
        let a = condition ? 1 : 2/3;    // warning49-51, add number type, 2 and 3 change to 2.0 and 3.0, arkts-numeric-semantic
    }
}

class pass2ParamType {
    foo(param: number): number {        // no warning, only handles direct assignment of a, parameter passing not checked, int can be passed as parameter to number
        return param;
    }

    goo(): void {
        let a = 1;                      // warning52, defined as int, arkts-numeric-semantic
        this.foo(a);
    }
}

class MayCauseInfiniteLoop {
    foo(): void {
        let a = 10;                     // warning53, defined as int, arkts-numeric-semantic
        a++;
        let b = 11;                     // warning54, defined as int, arkts-numeric-semantic
        b = (b < 0) ? 0: b;
    }
}

class SameVarInDiffScopes {
    foo(): void {
        for (let i = 0; i < 10; i++) {  // warning55, defined as int, arkts-numeric-semantic
            console.log(i);
        }
        for (let i = 1; i < 20; i++) {  // warning56, defined as int, arkts-numeric-semantic
            console.log(i);
        }
        for (let i = 2; i < 30; i++) {  // warning57, defined as int, arkts-numeric-semantic
            console.log(i);
        }
    }

    goo(): void {
        if (true) {
            let i = 3;                  // warning58, defined as int, arkts-numeric-semantic
        } else {
            let i = 3.3;
        }
    }
}

class FieldNumberTest {
    a = 1;                              // warning59, defined as number, arkts-numeric-semantic
    static b = 2;                       // warning60, defined as number, arkts-numeric-semantic
    readonly c = 3;                     // warning61, defined as number, arkts-numeric-semantic
    private _d = 4;                     // warning62, defined as number, arkts-numeric-semantic

    get d() {
        return this._d;
    }
}

class TestFieldNumberType {
    foo(): void {
        let aa = new FieldNumberTest();
        let tt = aa.a/FieldNumberTest.b;    // warning63, defined as number, arkts-numeric-semantic
    }
}

enum MyEnum {
    A = 1,
    B = 2,
    C = -1,
}

import { har_val } from './fixtures/homecheck-numeric-int-literal/export2';

class TestDivision {
    test1(): void {
        let vo = 123;                       // warning64, defined as int
        let d4 = MyEnum.A;                  // warning65, defined as number
        let d5 = vo as number;              // warning66, defined as number
        let d6 = new FieldNumberTest().a;   // warning67, defined as number
        let d7 = FieldNumberTest.b;         // warning68, defined as number
        let d9 = har_val;                   // warning69, defined as number
        let d10 = d4/2;                     // warning70-71, defined as number, 2 change to 2.0
        d10 = d5/3;                         // warning72, 3 change to 3.0
        d10 = d6/4;                         // warning73, 4 change to 4.0
        d10 = d7/5;                         // warning74, 5 change to 5.0
        d10 = d9/6.0;
    }

    test2(): void {
        let e1 = (1);                       // warning75, defined as number
        let e3 = parseInt('42');            // warning76, defined as number
        let e7 = e1/2;                      // warning77-78, defined as number, 2 change to 2.0
        e7 = e3/2;                          // warning79, 2 change to 2.0
    }

    test3(): void {
        for (let i = 0; i < 10; i++) {      // warning80, defined as number
            let b = i/2;                    // warning81-82, defined as number, 2 change to 2.0
        }
    }

    test4(): void {
        let k4 = new FieldNumberTest();
        let k5 = k4.a/2                     // warning83-84, defined as number, 2 change to 2.0
        let k6 = k4.d/3;                    // warning85-86, defined as number, 3 change to 3.0
    }

    test5(): void {
        const arr = [0, 1, 2];
        for (const i of arr) {              // i cannot have type annotation here, would cause compile error
            let b = i/2;                    // warning87-88, defined as number, 2 change to 2.0
        }
    }
}

let k1 = new FieldNumberTest();
let k2 = k1.d/3;                            // warning89-90, defined as number, 3 change to 3.0

class Other {
    floatWithoutDot(): void {
        const a = 100.;                     // number type, no warning
        const b = .99;                      // number type, no warning
        const c = 123e4;                    // number type, no warning
        const d = 3.14e2;                   // number type, no warning
        const e = 5e-3;                     // number type, no warning
        const f = 100.e+4;                  // number type, no warning
    }
}

class TestOptionalProperty {
  private a?: number = 1;                    //warning91, defined as int
  private b?: number = 1.0;
  public c: number;

  constructor(c?: number) {                 //warning92, defined as int
    this.c = c === undefined ? 2 : c;
  }

  test(a?: number, b?: number): void {      //warning93, defined as int
    a = 1.0;
    b = 2;
  }
}

class EnumDivision {
    foo(): void {
        let a = MyEnum.A / MyEnum.B;        // warning94-95, defined as number, MyEnum.Aadd valueOf().toDouble()
        let b = 2 / MyEnum.B;               // warning96-98, defined as number, 2 change to 2.0, MyEnum.Badd valueOf().toDouble()
        a = b / MyEnum.C;                   // warning99, MyEnum.Cadd valueOf().toDouble()
    }
}

let a5 = 1;  //closure capture, warning100, a5: number
let b5 = 1;  //closure capture, warning101, b5: number
let tt = ()=>{
    let c5 = a5 / b5 //warning102, c5: number
}

class TestTypeLiteral {
    foo(): void {
        let test: { a: number; } = { a: 2 };
    }
}

import { parseInt } from '@ohos.int';

class PositiveCase {
    numericLiteral(): void {
        let a = 1.0;            // only checks integer literals, not float literals
        let b: number = a;
    }

    otherUnaryOperator(): void {
        let a = ^1;
        let b = !3;
        let c = a;
        let d = b;
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
        this.numberParam(2);        // integer literal argument can be assigned to number type, no warning needed
    }

    annotationInt(): void {
        let a: int = 1;
        let b: int = a;
        let c: int = 2;
        a = c;
        let d: int = b;
    }
}

let l : number = 1.2e4;
let m : number = 12E-3;

class A {
    private a : number = 12e3;
    private b : number = 1.3E-3;
}

async function foo(){
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

// Additional cases for branch coverage

// Return statement with int literal (no invoke)
function returnIntLiteral(): number {
  return 42;
}

// If statement with int literal (no invoke)
function ifWithIntLiteral(x: number): void {
  if (x === 42) {
    console.log('found');
  }
}

// Assignment to array element with int literal
function arrayElementAssign(): void {
  let arr: number[] = [0, 0, 0];
  arr[0] = 42;
}

// Right side is array access with int literal as index
function arrayAccessWithIntIndex(): number {
  let arr: number[] = [10, 20, 30];
  return arr[1];
}

// Int literal in condition expression (ternary)
function ternaryWithIntLiteral(): number {
  return true ? 1 : 2;
}

// Int literal in cast expression
function castWithIntLiteral(): number {
  let x = (42 as number);
  return x;
}
