/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

class A {
    readonly indices: number[][] | null;
    constructor() {
        this.indices = new number[2][2];
    }

    public equals(other: Object | null | undefined): boolean {
        for (let i = 0; i < this.indices!.length; ++i) {
            let num: number[] = this.indices![i];
            num[0];
        }
        return false;
    }
}

function foo1(value: number, opt?: string) {
    return "" + value + opt;
}
function foo2(value: number, opt: string | undefined = undefined) {
    return "" + value + opt;
}
function testfoo() {
    arktest.assertEQ(foo1(12 as number), "12undefined") // inference blocked
    arktest.assertEQ(foo1(12, "s"), "12s")
    arktest.assertEQ(foo1(12, undefined), "12undefined")

    arktest.assertEQ(foo2(12 as number), "12undefined") // inference blocked
    arktest.assertEQ(foo2(12, "s"), "12s")
    arktest.assertEQ(foo2(12, undefined), "12undefined")
}

class DNode<T> {
    readonly parent: DNode<T> | undefined = undefined;
}

class DTree<T> {
    private current: DNode<T> = new DNode<T>();
    up() {
        let parent = this.current.parent
        this.current = parent!
    }
}

class B {
    public b: boolean = true;
};


function dummy(): B | undefined {
    return  new B();
}

function testb(): void {
    let b: B | undefined = dummy();
    if (b?.b == false) {
        b!.b = true;
    }
}

class Test {
    n: number | undefined = undefined;
}

type int32 = int;

function dummy1(): int32 | null {
    return null;
}

function testf(a: int32 | undefined): int32 {
    let x: int32 | null = dummy1();
    return a! + x!;
}

function testcond() {
    let _a: null | undefined = null, options_x: null | undefined = null;
    return (_a = options_x) !== null && _a != undefined ? _a : 1;
}

type float32 = float;
function foo(param?: float32) { }

export class Matrix33 {
    constructor() { this.arr = new Float32Array(8) }
    constructor(arr: Float32Array) { this.arr = arr }
    private arr: Float32Array | null = null;

    rotate(degrees: float32, pivotX?: float32, pivotY?: float32): Matrix33 {
        let rads = degrees * Math.PI / 180
        let cos = Math.cos(rads)
        let sin = Math.sin(rads)
        let newarr = new Float32Array(8)
        newarr.set([cos, -sin, 0, cos, 0, 0, 0, 1]);
        return new Matrix33(newarr);
    }
}

function main() {
    new A().equals(null);
    testfoo();
    try {
        new DTree<string | number>().up();
        arktest.assertTrue(false)
    } catch (e) {
        if (e not instanceof NullPointerError) {
            throw e;
        }
     }
    testb();
    try {
        testf(123);
        arktest.assertTrue(false)
    } catch (e) { }
    arktest.assertTrue((testcond() instanceof Int) && (testcond() == 1))

    foo(123);
    foo(undefined);
    new Matrix33().rotate(1, 2, 3)
}
