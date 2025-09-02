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

namespace NumericSemanticsReport {
    const a: number = 11 // NOT OK
    const b = 12 // NOT OK
    const c = 13.0 // NOT OK
    const d: number = 14.0
    const e: number = 15 // NOT OK
    console.log('' + 1/2) // NOT OK
}

namespace NumericSemanticsDone {
    const a: number = 11.0
    const b: number = 12.0
    const c: number = 13.0
    const d: number = 14.0
    const e: number = 15.0
    console.log('' + 1.0/2.0)
}

namespace NoNumericSemantics {
    interface X1 { name: number, __2: number}
    interface X2 { name: number, _2: number}
    let x: X1 = {name: 20.0, __2: 30.0} // OK
    console.log(x.__2); // OK

    let x_fix: X2 = {name: 20.0, _2: 20.0};

    let x_non = {name: 20.0, 2: 20.0}; // OK

    const arr = []
    console.log(arr[2]);

    // Number bases
    let c5: number = 0x123; // Hexadecimal
    let c6: number = 0o123; // Octal
    let c7: number = 0b101; // Binary

    let e1: number = 1e0;
    let e2: number = 1E1;
    let e3: number = 1e+1;
    let e4: number = 1E-1;
    let e5: number = +1e2;
    let e6: number = -1E2;

    let h: number = arr[12. as int]

    enum E {
        A = 1.0,
        B = 2.0
    }
}

namespace NumericSemanticsOther {
    let e7: number = 1e4 + 11;
}

namespace BeCareful {
    `${1/2}`

}

namespace NoDiffInArk1_1To1_2 {
    const a1 = `${1/2}` // NOT OK
    const a2 = `${1.0/2.0}`
    const b1 = `20${20 | 21 | 22 | 23}` // NOT OK
    const b2 = `20${20.0 | 21.0 | 22.0 | 23.0}`
    const c1 = `20 + ${20}` // NOT OK
    const c2 = `20 + ${20.0}`
    console.log(a1,a2,b1,b2,c1,c2)

    // Automatically delete decimal parts during bitwise operations
    let e = 15 // NOT OK
    let e1 = e & 3; // NOT OK
    let e2 = e | 3; // NOT OK
}

namespace GenericTypeCase {
    function ReturnGenericNumber<T>(a: T): T {
        return a
    }

    function ReturnGenericArry<T>(a: T): T[] {
        return [a]
    }

    ReturnGenericNumber(1) // NOT OK, generic type is
    ReturnGenericNumber(true ? 1 : 2) // NOT OK
    ReturnGenericArry(1) // NOT OK

    function TestReturnGenericNumber<T>(a: T[]): T[] {
        return a.map(item => item) // OK, not report arkts-numeric-semantic
    }

    function MapCase(a: number[]): number {
        let groupNum: number = new Set(a.map(item => item)).size; // OK, not report arkts-numeric-semantic
        return groupNum;
    }

    function foo<T>(v:T):T{return v}
    foo(12)/24 // NOT OK
    foo(12.0)/24 // NOT OK

    function foo1<T,U>(v:T, u:U, b:boolean):T|U{
        return b ? v: u
    }
    foo1(12.0, 8, true)/24 // NOT OK
    foo1(12.0, 8, false)/24 // NOT OK

    console.log(foo1(12/24, 8, true)) // NOT OK
    console.log(foo1<number,number>(12/24, 8, true)) // NOT OK
    console.log(foo1<number,number>(12.0/24.0, 8, true))
}
