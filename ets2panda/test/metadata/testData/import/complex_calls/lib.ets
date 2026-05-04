/**
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

export function foo(x: string): void {
    console.log(x)
}

class B {}

export class A {
    fld: string = ""

    f1 (x: int | string): int | string {
        if (x instanceof Int) {
            return -1;
        }
        if (this.fld == "") {
            return -2;
        }
        return x;
    }

    f2 <T extends Object>(x: T, y: undefined): string | undefined {
        return x instanceof string ? x : y;
    }
}

export namespace NS {
    namespace INS {
        const v1: int = 33;
    }
    console.log("Test")
    export let a: A = new A();

    export function bar(x: A): void {
        console.log(x.f1("abc"))
    }
}

export class A22 {
    static bar3(x: A): void {
        console.log(x.f1("abc"))
    }
}

export function bar2(x: A): void {
    console.log(x.f1("abc"))
}

export let gv: A = NS.a