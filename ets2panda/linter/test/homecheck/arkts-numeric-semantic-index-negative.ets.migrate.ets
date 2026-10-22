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

const arr = [1, 2, 3, 4, 5];

function testArrayIndexWithInt(): void {
    const index: number = 10; // arkts-numeric-semantic
    let item = arr[index];
    item = arr[0];

    const a = 20; // arkts-numeric-semantic
    const b = a; // arkts-numeric-semantic
    item = arr[b];

    let c = 1 + 2; // arkts-numeric-semantic
    item = arr[~c];

    let d = c; // arkts-numeric-semantic
    item = arr[-d];
}

function testArrayIndexWithNumber(): void {
    const index: number = 10.1;
    let item = arr[index]; // arkts-numeric-semantic
    const a = 20.2; // arkts-numeric-semantic
    const b = a; // arkts-numeric-semantic
    item = arr[b]; // arkts-numeric-semantic

    let c = 2; // arkts-numeric-semantic
    arr[c]; // arkts-numeric-semantic
    c = 3.3;
}

const globalIndexInt = 3; // arkts-numeric-semantic
const globalIndexNumber = 3.3; // arkts-numeric-semantic

class A {
    b: number = 1;
    static c = 2; // arkts-numeric-semantic
}

function testIndexWithFieldRef(): void {
    const a = new A();
    let item = arr[a.b]; // arkts-numeric-semantic
    let intValue = arr[A.c]; // arkts-numeric-semantic
}

function testLocalInRightOp(): void {
    let a = 1; // arkts-numeric-semantic
    let item = arr[a++];

    let b = a * 1.1; // arkts-numeric-semantic
    arr[b]; // arkts-numeric-semantic
}

function bar(a: number): number { // arkts-numeric-semantic
    return a * 1.2;
}

function testLocalInOtherStmt(): number {
    let a = 1; // arkts-numeric-semantic
    let item = arr[a];
    let b = bar(a); // arkts-numeric-semantic
    item = arr[b]; // arkts-numeric-semantic

    if (a > 0 && b < 0) {
        return -a;
    }
    return a;
}

const INDEX = 3; // arkts-numeric-semantic

class GlobalIndex {
    private useGlobalIndex(): void {
        const arr: number[] | undefined = []
        let item = arr[INDEX];
        let i = 1.1; // arkts-numeric-semantic
        item = arr[i]; // arkts-numeric-semantic
    }
}

class TestClosureIndex {
    private foo = (param: number): void => {
        let index = 0; // arkts-numeric-semantic
        let goo = (): void => {
            let item1 = arr[param]; // arkts-numeric-semantic
            let item2 = arr[index];
        }
    }

    private callFoo(): void {
        this.foo(2.5);
    }

    goo(): void {
        let index = 2; // arkts-numeric-semantic
        let funcA = (): void => {
            let item = arr[index];
        }
    }
}

class Assign2ArrayItem {
    foo(): void {
        const a = 1; // arkts-numeric-semantic
        arr[a] = 2;
        let b = 3; // arkts-numeric-semantic
        arr[b++] = 4;
        let c = 5; // arkts-numeric-semantic
        arr[c+1] = 6;
    }
}

class IndexWithConstant {
    foo(): void {
        let item = arr[1.0]; // arkts-numeric-semantic
        arr[2.00] = 3; // arkts-numeric-semantic
        let a = 1.0; // arkts-numeric-semantic
        item = arr[a]; // arkts-numeric-semantic
        arr[1.03]; // arkts-numeric-semantic
        arr[2.20] = 2; // arkts-numeric-semantic
    }
}

class NumberInName {
    foo(): void {
        let numberA: number = 1; // arkts-numeric-semantic
        let numberB = 2; // arkts-numeric-semantic
        let numberC = 3.1; // arkts-numeric-semantic
        arr[numberA];
        arr[numberB];
        arr[numberC]; // arkts-numeric-semantic
    }
}

class PositiveCase {
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

class ArrayLength {
    foo() {
        let arr = [0, 1, 2, 3];
        for (let i = arr.length - 1; i >= 0; i--) {
            let item = arr[i];
        }
    }
}
