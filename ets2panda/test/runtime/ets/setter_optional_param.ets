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

class C1 {
    _xx: string | undefined
    set xx(x: string | undefined) {
        this._xx = x;
    }
    get xx(): string | undefined {
        return this._xx;
    }
}

class C2 {
    static _yy: string | undefined = "x"
    static set yy(y: string | undefined) {
        C2._yy = y;
    }
}

interface I1 {
    set zz(z: string | undefined)
    get zz(): string | undefined
}

class C3 implements I1 {
    private _zz: string | undefined = ""
    get zz(): string | undefined {
        return this._zz;
    }
    set zz(z: string | undefined) {
        this._zz = z;
    }
}

function main() {
    let c1 = new C1()
    c1.xx = "xyz"
    C2.yy = "xyz"
    let i1: I1 = new C3();
    i1.zz = "xyz";
    arktest.assertEQ(c1.xx, "xyz")
    arktest.assertEQ(C2._yy, "xyz")
    arktest.assertEQ(i1.zz, "xyz")
}
